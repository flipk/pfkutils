
Based on the examination of the core dump
(core.dtlsq_stress_te.2164116) and the source code, here is the
detailed crash analysis.

──────────────────────────────────────────────────────────────────────────────

1. Executive Summary

- Process / Command: ./obj/dtlsq_stress_test 1
- Fatal Signal: SIGSEGV (Signal 11 — Segmentation fault)
- Fault Address: 0x000000000000002c (NULL pointer dereference at offset +0x2c)
- Crashing Thread: Thread 1 (LWP 2164123) in
  ProtoSSL::ProtoSslDtlsQueue::recv_thread() at instruction
  0x000000000041aee7 (protossldtlsqueuing.cc:364).
- Primary Cause: A race condition between
  ProtoSslDtlsQueue::shutdown() and ProtoSslDtlsQueue::recv_thread().
  shutdown() sets member pointer client = NULL before stopping and
  joining worker threads. When recv_thread() wakes from select() due
  to incoming network traffic on the socket, it calls client->get_fd()
  where client is NULL, crashing on client->netctx.fd (offset 0x2c).

──────────────────────────────────────────────────────────────────────────────

2. Core Dump & Thread Inspection

Thread Overview

Thread 1 (LWP 2164123) [CRASHED]:
  #0 0x000000000041aee7 in ProtoSSL::ProtoSslDtlsQueue::recv_thread()
  #1 0x000000000041b0e8 in ProtoSSL::ProtoSslDtlsQueue::_start_thread(void*)

Thread 4 (LWP 2164116 - Main / Test Runner Thread):
  #0 0x00007f829f3947e9 in __futex_abstimed_wait_common ()
  #1 0x00007f829f399eb3 in pthread_join ()
  #2 0x000000000041417a in ProtoSSL::ProtoSslDtlsQueue::shutdown()
  #3 0x00000000004053aa in TestSession::cleanup()
  #4 0x0000000000408d9f in TestSession::run()
  #5 0x0000000000403886 in main ()

Thread 2 (LWP 2164122 - Tick Thread):
  #0 0x00007f829f419997 in select ()
  #1 0x0000000000414ae5 in ProtoSSL::ProtoSslDtlsQueue::tick_thread()

Thread 3 (LWP 2164117 - Proxy Thread):
  #0 0x00007f829f419997 in select ()
  #1 0x0000000000406446 in UdpFaultProxy::proxy_thread()

Thread 5 (LWP 2164124 - Send Thread):
  #0 0x00007f829f397239 in pthread_cond_wait ()
  #1 0x0000000000419b42 in ProtoSSL::ProtoSslDtlsQueue::send_thread()

Register & Memory State in Thread 1 (recv_thread)

- r15 = 0x1e9280c0 (this pointer of ProtoSslDtlsQueue)
- rdi = 0x00000000 (loaded from 0xa0(%r15), which corresponds to
  member variable this->client)
- Faulting instruction:
  0x41aee0: mov  0xa0(%r15), %rdi    ; rdi = this->client (loaded NULL!)
  0x41aee7: mov  0x2c(%rdi), %ecx    ; ecx = client->get_fd() -> SIGSEGV at (0x0 + 0x2c)
- Stack read bitset for sel.rfds:
  %rsp contained 0x0000000000000200 (1 << 9), meaning FD 9 (the DTLS
   UDP client socket) had pending data.
- FD 12 (closer_pipe.readEnd) was not set in this return from select().

State in Thread 4 (main thread in shutdown())

- Frame 2 shows shutdown() had executed:
  WaitUtil::Lock lock(&dtls_lock);
  if (client == NULL) return;
  ProtoSSLConnClient * l_client = client;
  client = NULL;             // <--- Nulls shared member variable here!
  lock.unlock();
  ...
  write(closer_pipe.writeEnd, &dummy, 1);
  ...
  pthread_join(thread_ids[0], ...);  // <--- Blocked waiting for tick_thread (Thread 2)
- Local variable l_client in Thread 4 was 0x7f8290000b70.
  Inspecting 0x7f8290000b70 + 0x2c confirms netctx.fd == 9.

──────────────────────────────────────────────────────────────────────────────

3. Root Cause Analysis

In protossldtlsqueuing.cc, ProtoSslDtlsQueue::shutdown() and
ProtoSslDtlsQueue::recv_thread() interact with client without safe
synchronization:

1. Premature Pointer Invalidation:
   In ProtoSslDtlsQueue::shutdown() (lines 181–189):
   WaitUtil::Lock lock(&dtls_lock);
   if (client == NULL)
       return;

   ProtoSSLConnClient * l_client = client;
   client = NULL; // Attempting to guard against double-shutdown
   lock.unlock();
   this->client is set to NULL prior to stopping and joining the
   background worker threads (recv_thread, tick_thread, send_thread).

2. Unsynchronized and Repeated Access in recv_thread:
   In recv_thread() (lines 351–367):
   while (1)
   {
       pxfe_select  sel;
       sel.rfds.set(closer_pipe.readEnd);
       if (alive)
           sel.rfds.set(client->get_fd());
       sel.tv.set(1,0);
       sel.select();

       if (sel.rfds.is_set(closer_pipe.readEnd))
           break;

       if (alive && sel.rfds.is_set(client->get_fd())) // Line 364: CRASH HERE
       {
           ProtoSSLConnClient::read_return_t rr =
               client->handle_read_raw(frag_recv_buffer);
   - recv_thread() holds no lock when reading client.
   - On line 364, client->get_fd() is evaluated unconditionally
     whenever alive is true.
   - If incoming traffic arrives on the UDP socket (or if select()
     returns due to traffic/timeout) right as shutdown() sets client =
     NULL, sel.rfds.is_set(closer_pipe.readEnd) is false.
   - recv_thread() evaluates client->get_fd(), reads client == NULL,
     and immediately dereferences address 0x2c.

──────────────────────────────────────────────────────────────────────────────

4. Secondary & Related Failure Modes

1. Top-of-loop race in recv_thread:
   At line 356: sel.rfds.set(client->get_fd()) will also dereference
   NULL if client is set to NULL while recv_thread is beginning an
   iteration of while(1).
2. Missing NULL check in send_frag:
   In send_frag() (protossldtlsqueuing.cc:1297):
   if (client->send_raw(frag_send_buffer) == false)
   send_frag() directly calls client->send_raw(). If a queued tick
   event triggers handle_tick() in send_thread after shutdown() has
   nulled client, send_thread will also crash with a NULL pointer
   dereference.
3. Flawed Re-entrancy Protection:
   The comment in shutdown() states that nulling client was intended
   to prevent double-free if the user calls delete dtlsq from another
   thread. However, nulling client while worker threads are running
   causes a crash, and if a destructor were actually called
   concurrently, destroying member objects while pthread_join is in
   progress would result in memory corruption.

──────────────────────────────────────────────────────────────────────────────

5. Summary of Possible Causes

┌───────────────────────┬─────────────────────────────────────┬────────────────────────────────────────────────┐
│ Component             │ Trigger / Condition                 │ Mechanism                                      │
├───────────────────────┼─────────────────────────────────────┼────────────────────────────────────────────────┤
│ recv_thread vs        │ Server or proxy sends UDP packets   │ shutdown() sets client = NULL before threads   │
│ shutdown race         │ during test cleanup while shutdown( │ are stopped. recv_thread wakes on the UDP      │
│                       │ ) is called                         │ packet and executes client->get_fd(), crashing │
│                       │                                     │ at 0x2c.                                       │
├───────────────────────┼─────────────────────────────────────┼────────────────────────────────────────────────┤
│ recv_thread select    │ 1-second select timeout expires     │ closer_pipe bit is not set; recv_thread        │
│ timeout race          │ between client = NULL and write(    │ proceeds to evaluate client->get_fd(),         │
│                       │ closer_pipe)                        │ dereferencing NULL.                            │
├───────────────────────┼─────────────────────────────────────┼────────────────────────────────────────────────┤
│ send_thread tick race │ TICK event processed before DIE     │ handle_tick() calls send_frag(), which calls   │
│                       │ event in send_q during shutdown     │ client->send_raw() without verifying client != │
│                       │                                     │ NULL.                                          │
└───────────────────────┴─────────────────────────────────────┴────────────────────────────────────────────────┘

