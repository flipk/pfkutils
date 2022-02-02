
#include "libprotossl.h"
#include PROTODTLSQUEUING_HDR
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#define PRINT_SEQNOS 0
#define PRINTF(x...) // fprintf(stderr,x)


#if GOOGLE_PROTOBUF_VERSION >= 3004001
#define BYTE_SIZE_FUNC ByteSizeLong
#else
#define BYTE_SIZE_FUNC ByteSize
#endif

using namespace ProtoSSL;


ProtoSslDtlsQueue :: dtls_fragment :: dtls_fragment(void)
{
    pkthdr = new ProtoSSL :: DTLS :: DTLS_PacketHeader_m;
    init();
}

ProtoSslDtlsQueue :: dtls_fragment :: ~dtls_fragment(void)
{
    delete pkthdr;
}

void
ProtoSslDtlsQueue :: dtls_fragment :: init()
{
    fragment.resize(0);
    pkthdr->Clear();
    seqno = 0;
    delivered = false;
    age = 0;
    refcount = 0;
}

std::string
ProtoSslDtlsQueue :: dtls_fragment :: print(void) const
{
    std::ostringstream ret;
    ret << "age " << age
        << ", fragment " << fragment.size()
        << ", hdr:\n" << pkthdr->DebugString();
    return ret.str();
}

ProtoSslDtlsQueue :: ProtoSslDtlsQueue(const ProtoSslDtlsQueueConfig &_config,
                                       ProtoSSLConnClient * _client)
    : config(_config), // implicit copy constructor (shallow copy)
      client(_client)
{
    send_msg_pool.add(pool_size);
    send_other_pool.add(other_pool_size);
    read_pool.add(pool_size);

    queues[0] = &send_q;
    num_queues = 1;

    for (uint32_t ind = 0; ind < config.num_queues; ind++)
    {
        queues[num_queues] = new dtls_thread_queue_t;
        queue_sizes[num_queues] = 0;
        num_queues++;
    }
    // NOTE at this point, this->num_queues = config.num_queues+1
    // because the send_q is always queue[0].
    // but ALSO note that ProtoSslDtlsQueueConfig::add_queue always
    // returns index into queues[] (but not into queue_config[]).

    _ok = true;
    link_up = true;
    link_changed = false;

    tick = 0;

    // don't bother with recv_window sizing; that'll happen
    // on the first msg from the sender.
    send_window.resize(config.window_size);
    send_seqno = 0;
    recv_seqno = 0;
    ticks_without_recv = 0;
    ticks_without_send = 0;

    // initial value, will change
    retransmit_age = config.ticks_per_second;
    rtd_rsp_timestamp = 0;
    last_rtd_req_time = 0;

    for (uint32_t which = 0; which < num_threads; which++)
    {
        thread_started[which] = false;
        if (start_thread(which, thread_ids + which) == false)
        {
            _ok = false;
            break;
        }
    }
}

ProtoSslDtlsQueue :: ~ProtoSslDtlsQueue(void)
{
    // internally, shutdown() protects itself from being
    // called twice.
    shutdown();

    // clean out the handle_read queue()
    dtls_read_event * dre;
    while ((dre = recv_q.dequeue(0)) != NULL)
        read_pool.release(dre);

    // clean out all inbound queues.
    dtls_send_event * dte;
    for (uint32_t ind = 0; ind < num_queues; ind++)
    {
        while ((dte = queues[ind]->dequeue(0)) != NULL)
            send_event_release(dte);
        // don't delete queues[0], since that is &send_q
        if (ind != 0)
            delete queues[ind];
    }
}

inline ProtoSslDtlsQueue::dtls_fragment *
ProtoSslDtlsQueue :: fragpool_t :: alloc(void)
{
    // note thread_slinger_pool requires a buffer
    // released to a pool actually came from the pool
    // via alloc (it tracks in a buffer what pool it
    // came from)
    if (fragpool.empty())
        fragpool.add(10);
    dtls_fragment * frag = fragpool.alloc(0);
    frag->init();
    frag->refcount = 1;
    return frag;
}

inline void
ProtoSslDtlsQueue :: fragpool_t :: deref(dtls_fragment *frag)
{
    if (--frag->refcount == 0)
    {
        frag->init();
        fragpool.release(frag);
    }
}

void
ProtoSslDtlsQueue :: shutdown(void)
{
    // prevent a possible race condition:
    //   we send a DIE packet to send_thread.
    //   it sends GOT_DISCONNECT to recv_q and dies.
    //   two things happen in parallel in different threads:
    //   1.a. handle_read gets the message and returns it to caller.
    //     b. caller deletes ProtoSslDtlsQueue which calls shutdown again.
    //     c. shutdown tries to delete client.
    //   2.a. this thread wakes up from pthread_join
    //     b. this thread deletes client (double free!)
    // how do we prevent this?
    // use dtls_lock, and atomically read and NULL the client pointer.
    // then unlock the lock, and do the cleanup, deleting the client
    // (which is now a thread-local private variable)
    // error-free.

    WaitUtil::Lock lock(&dtls_lock);
    if (client == NULL)
        // already been shutdown()
        return;

    ProtoSSLConnClient * l_client = client;
    // signal that we should not call shutdown twice.
    client = NULL;
    lock.unlock();

    char dummy = 0;
    void * ptr = NULL;

    // both recv thread and tick thread select
    // on closer_pipe, but neither read from it,
    // so both should wake up.
    if (::write(closer_pipe.writeEnd, &dummy, 1) < 0)
    { /* keep compiler happy by pretending to care */ }

    // also close sender thread. sender thread is smart
    // enough to know a DIE packet didn't come from the pool.
    // we don't want to alloc from the pool in case it's empty
    // and we don't want to deadlock here.
    dtls_send_event die_evt;
    die_evt.type = dtls_send_event::DIE;
    send_q.enqueue(&die_evt);

    for (uint32_t which = 0; which < num_threads; which++)
        if (thread_started[which])
            pthread_join(thread_ids[which], &ptr);

    delete l_client;
}

bool
ProtoSslDtlsQueue :: start_thread(uint32_t which, pthread_t *pId)
{
    WaitUtil::Waiter waiter(&startThreadSync);
    waiter.lock();
    startWhich = which;
    int e = pthread_create(pId, NULL, &_start_thread, (void*) this);
    if (e != 0)
    {
        char err[100];
        fprintf(stderr,
                "ProtoSslDtlsQueue: starting thread %u: %d (%s)\n",
                which, e, strerror_r(e, err, sizeof(err)));
        return false;
    }
    pxfe_timespec expire(1,0);
    expire += pxfe_timespec().getNow();
    while (thread_started[which] == false)
    {
        if (waiter.wait(expire()) == false)
        {
            fprintf(stderr,
                    "ProtoSslDtlsQueue: timeout waiting for "
                    "thread %u to start!\n",
                    which);
            return false;
        }
    }
    return true;
}

// this function exists because it's painful to write an entire
// thread inside a static class method.
/*static*/ void *
ProtoSslDtlsQueue :: _start_thread(void *arg)
{
    ProtoSslDtlsQueue * obj = (ProtoSslDtlsQueue *) arg;
    // read the variable before waking up starter.

    WaitUtil::Lock lock(&obj->startThreadSync);
    uint32_t which = obj->startWhich;
    obj->thread_started[which] = true;
    lock.unlock();
    obj->startThreadSync.waiterSignal();

    switch (which)
    {
    case 0:  obj->tick_thread();  break;
    case 1:  obj->recv_thread();  break;
    case 2:  obj->send_thread();  break;
    }
    return NULL;
}

void
ProtoSslDtlsQueue :: tick_thread(void)
{
    // no lock required here because send_pool and send_q internally safe.
    while (1)
    {
        pxfe_select sel;
        sel.rfds.set(closer_pipe.readEnd);
        sel.tv.set(0, 1000000 / config.ticks_per_second);
        sel.select();
        if (sel.rfds.is_set(closer_pipe.readEnd))
            // time to exit.
            break;
        tick++;
        dtls_send_event * evt = send_other_pool.alloc(-1);
        evt->type = dtls_send_event::TICK;
        send_q.enqueue(evt);
    }
}

std::string
ProtoSslDtlsQueueStatistics ::Format(void)
{
    std::ostringstream  ostr;
    ostr << "bs " << bytes_sent
         << " br " << bytes_received
         << " fs " << frags_sent
         << " fr " << frags_received
         << " frs " << frags_resent
         << " mfd " << missing_frags_detected;
    return ostr.str();
}

bool
ProtoSslDtlsQueue :: get_peer_info(ProtoSSLPeerInfo &info)
{
    if (client == NULL)
        return false;
    return client->get_peer_info(info);
}

void
ProtoSslDtlsQueue :: get_stats(ProtoSslDtlsQueueStatistics *pstats)
{
    *pstats = stats;
}

ProtoSslDtlsQueue::read_return_t
ProtoSslDtlsQueue :: handle_read(MESSAGE &msg)
{
    // this object has been shutdown()
    if (client == NULL)
        return GOT_DISCONNECT;

    // no lock required here because recv_q and read_pool internally safe.
    dtls_read_event *dre = recv_q.dequeue(-1);
    read_return_t ret = dre->retcode;
    if (ret == GOT_MESSAGE)
    {
        if (msg.ParseFromString(dre->encoded_msg) == false)
        {
            fprintf(stderr, "ProtoSslDtlsQueue :: handle_read : "
                    "parse failure\n");
            // is this the best way to handle this? not really,
            // but how much will the apps have to change at this point,
            // since we haven't been handling it this far? this is
            // congruent with the behavior of ConnClient.
            msg.Clear();
            ret = READ_MORE;
        }
        stats.bytes_received += dre->encoded_msg.length();
        PRINTF("handle_read recv_q to user:\n%s\n", msg.DebugString().c_str());
    }
    read_pool.release(dre);
    return ret;
}

void
ProtoSslDtlsQueue :: recv_thread(void)
{
    bool alive = true;
    // no lock here; see called functions.
    while (1)
    {
        pxfe_select  sel;
        sel.rfds.set(closer_pipe.readEnd);
        if (alive)
            sel.rfds.set(client->get_fd());
        sel.tv.set(1,0);
        sel.select();

        if (sel.rfds.is_set(closer_pipe.readEnd))
            // time to exit.
            break;

        if (alive && sel.rfds.is_set(client->get_fd()))
        {
            ProtoSSLConnClient::read_return_t rr =
                client->handle_read_raw(frag_recv_buffer);

            switch (rr)
            {
            case ProtoSSLConnClient::GOT_DISCONNECT:
            {
                // any remaining fragments in the window
                // will be lost at this point, since their
                // missing frags will never arrive.
                dtls_read_event * dre = read_pool.alloc(-1);
                dre->retcode = GOT_DISCONNECT;
                recv_q.enqueue(dre);
                // if we GOT_DISCONNECT from ConnClient, it has closed
                // its socket, and it's no longer valid to call
                // get_fd() on it. the only thing we can do now is
                // wait calmly for the sweet embrace of a graceful
                // exit.
                alive = false;
                break;
            }
            case ProtoSSLConnClient::READ_MORE:
                // means same as client's meaning. go around again,
                // and send nothing to user.
                break;

            case ProtoSSLConnClient::GOT_TIMEOUT:
                // what do we do with this? not much, that i know
                // of right now.
                break;

            case ProtoSSLConnClient::GOT_MESSAGE:
                handle_got_frag();
                break;
            }
        }
    }
}

void
ProtoSslDtlsQueue :: handle_got_frag(void)
{
    // need lock because we access recv_window and ondeck_ack/nack;
    // it is locked in the appropriate places below.
    WaitUtil::Lock   lock(&dtls_lock, false);

    dtls_fragment * frag = NULL;
    google::protobuf::io::CodedInputStream::Limit l;
    uint32_t seqno = 0;
    uint32_t wpos = 0;
    bool reliable = true;
    google::protobuf::io::CodedInputStream  instr(
        (const uint8_t *) frag_recv_buffer.c_str(),
        (int) frag_recv_buffer.size());

    uint32_t header_size;
    if (instr.ReadVarint32(&header_size) == false)
    {
        // PRINT SOMETHING
        fprintf(stderr, "ProtoSslDtlsQueue :: handle_got_frag : "
                "failed to read header_size\n");
        goto fail1;
    }

    if ((uint32_t) instr.BytesUntilLimit() < header_size)
    {
        // PRINT SOMETHING
        fprintf(stderr, "ProtoSslDtlsQueue :: handle_got_frag : "
                "packet does not contain complete PacketHeader\n");
        goto fail1;
    }

    l = instr.PushLimit(header_size);
    frag = fragpool.alloc();
    if (frag->pkthdr->ParseFromCodedStream(&instr) == false)
    {
        // PRINT SOMETHING
        fprintf(stderr, "ProtoSslDtlsQueue :: handle_got_frag : "
                "failure parsing PacketHeader\n");
        goto fail2;
    }
    instr.PopLimit(l);

    PRINTF("received fragment:\n%s\n", frag->pkthdr->DebugString().c_str());

    reliable = frag->pkthdr->has_sequence_no();
    seqno = reliable ? frag->pkthdr->sequence_no() : 0;
    frag->seqno = seqno;

    stats.frags_received ++;

#if PRINT_SEQNOS
    {
        std::ostringstream str;
        bool printit = false;
        str << "<--- t:" << tick << " received ";
        if (frag->pkthdr->has_sequence_no())
        {
            str << "seqno " << frag->pkthdr->sequence_no() << " ";
            printit = true;
        }
        else
            str << "         ";
        if (frag->pkthdr->ack_seq_no_size() > 0)
        {
            printit = true;
            str << "acks ";
            for (int ind = 0; ind < frag->pkthdr->ack_seq_no_size(); ind++)
            {
                str << frag->pkthdr->ack_seq_no(ind) << " ";
            }
        }
        if (frag->pkthdr->nack_seq_no_size() > 0)
        {
            printit = true;
            str << "nacks ";
            for (int ind = 0; ind < frag->pkthdr->nack_seq_no_size(); ind++)
            {
                str << frag->pkthdr->nack_seq_no(ind) << " ";
            }
        }
        if (printit)
            fprintf(stderr,"%s\n", str.str().c_str());
    }
#endif

    if (frag->pkthdr->has_window_size() == false)
    {
        // PRINT SOMETHING
        fprintf(stderr, "ProtoSslDtlsQueue :: handle_got_frag : "
                "incomplete PacketHeader (%d%d %u:%u)\n",
                frag->pkthdr->has_sequence_no(),
                frag->pkthdr->has_window_size(),
                frag->pkthdr->has_sequence_no() ?
                frag->pkthdr->sequence_no() : 0,
                frag->pkthdr->has_window_size() ?
                frag->pkthdr->window_size() : 0);
        goto fail2;
    }

    // if we received a rtd_req, immediately send an rtd_rsp.
    if (frag->pkthdr->has_rtd_req_timestamp())
    {
        rtd_rsp_timestamp = frag->pkthdr->rtd_req_timestamp();
        send_heartbeat();
    }

    // if we received a rtd_rsp, recalculate the rtd and update
    // the max age for retransmissions.
    if (frag->pkthdr->has_rtd_rsp_timestamp())
    {
        pxfe_timeval tv;
        tv.getNow();
        uint64_t diff = tv.usecs() - frag->pkthdr->rtd_rsp_timestamp();
        retransmit_age = (diff / (1000000 / config.ticks_per_second)) * 2;
        PRINTF("retransmit_age set to %u because rtd %" PRIu64 "\n",
               retransmit_age, diff);
        if (retransmit_age < 2)
            retransmit_age = 2;
    }

    // below this point, accessing resources shared in the class
    // which are not already internally safe.
    lock.lock();

    ticks_without_recv = 0;
    if (link_up == false)
    {
        fprintf(stderr, "ProtoSslDtlsQueue :: handle_got_frag : "
                "LINK_UP!\n");
        link_up = true;
        link_changed = true;
    }

    if (recv_window.size() < frag->pkthdr->window_size())
    {
        PRINTF("ProtoSslDtlsQueue :: handle_got_frag : "
                "receive window size set to %u\n",
                frag->pkthdr->window_size());
        recv_window.resize(frag->pkthdr->window_size());
    }

    for (int ind = 0;
         ind < frag->pkthdr->ack_seq_no_size();
         ind++)
    {
        PRINTF("processing ack %u\n", frag->pkthdr->ack_seq_no(ind));
        handle_recv_ack(frag->pkthdr->ack_seq_no(ind));
    }
    for (int ind = 0;
         ind < frag->pkthdr->nack_seq_no_size();
         ind++)
    {
        PRINTF("processing nack %u\n", frag->pkthdr->nack_seq_no(ind));
        handle_recv_nack(frag->pkthdr->nack_seq_no(ind));
    }

    if (frag->pkthdr->has_message_body_size())
    {
        uint32_t body_size = frag->pkthdr->message_body_size();
        if ((uint32_t) instr.BytesUntilLimit() < body_size)
        {
            // PRINT SOMETHING
            fprintf(stderr, "ProtoSslDtlsQueue :: handle_got_frag : "
                    "packet too short for message body (%d:%u)\n",
                    instr.BytesUntilLimit(), body_size);
            goto fail2;
        }
        l = instr.PushLimit(body_size);
        frag->fragment.resize(body_size);
        if (instr.ReadRaw((void*) frag->fragment.c_str(),
                          body_size) == false)
        {
            // PRINT SOMETHING
            fprintf(stderr, "ProtoSslDtlsQueue :: handle_got_frag : "
                    "readRaw of message body failed\n");
            goto fail2;
        }
        instr.PopLimit(l);

        if (reliable)
        {
            push_ondeck_acks(seqno);

// when we receive a fragment, we put it both in the reassembly map
// and in the recv_window, and we send an ack. reference count = 2.
// when a slot in recv_window gets overwritten by a new seqno,
// the old frag is removed from that slot, refcount --.
// when a complete message can be reassembled from reassembly map,
// they are marked as delivered but left in the map for a while.
// eventually seqnos more than 2*recv_window.size() are removed, refcount--.
// when refcount == 0, released back to frag pool.

            wpos = seqno % recv_window.size();

            ReassemblyMap::iterator rmi = recv_reassembly.find(seqno);
            if (rmi != recv_reassembly.end())
            {
                // we got the same fragment twice.
                // our last ack must not have gotten through.
                // so send an ack.
                fragpool.deref(frag);
            }
            else
            {
                if (recv_window[wpos] != NULL)
                    fragpool.deref(recv_window[wpos]);
                recv_window[wpos] = frag; // this counts as refcount 1.
                recv_reassembly[seqno] = frag;
                frag->refcount ++; // this counts as refcount 2.
            }

            uint32_t seqno_started = recv_seqno;
            while (1)
            {
                // now attempt reassembly of the next message
                // due to be delivered to the app.
                uint32_t consumed = recv_reassemble_deliver(recv_seqno);
                if (consumed == 0)
                    break;
                recv_seqno += consumed;
            }

            // clean the map of excessively old seqnos by trailing
            // two window sizes behind the ones we just consumed.
            for (seqno = seqno_started; seqno != recv_seqno; seqno++)
            {
                rmi = recv_reassembly.find(seqno - (2*recv_window.size()));
                if (rmi != recv_reassembly.end())
                {
                    fragpool.deref(rmi->second);
                    recv_reassembly.erase(rmi);
                }
            }
        }
        else // reliable = false
        {
            // deliver now, no questions.

            dtls_read_event * dre = read_pool.alloc(-1);
            dre->retcode = GOT_MESSAGE;
            dre->encoded_msg = frag->fragment;
            fragpool.deref(frag);
            recv_q.enqueue(dre);
        }
    }
    else
    {
        // done processing this frag, nothing to keep.
        fragpool.deref(frag);
    }

    if (0)
    {
fail2:
        fragpool.deref(frag);
    }

fail1:
    return;
}

uint32_t
ProtoSslDtlsQueue :: recv_reassemble_deliver(uint32_t seqno)
{
    ReassemblyMap::iterator rmi = recv_reassembly.find(seqno);
    if (rmi == recv_reassembly.end())
        // not present.
        return 0;

    dtls_fragment * f, * first_f = rmi->second;

    recv_frag_list.clear();
    // if this frag has no fragno/num_frags, it must be a
    // single-frag message, so skip the part where we scan
    // for remaining pieces and jump straight to delivery.
    recv_frag_list.push_back(first_f);
    PRINTF("reassembly list pushed first frag: %s\n",
           first_f->print().c_str());

    if (first_f->pkthdr->has_frag_no() && first_f->pkthdr->has_num_frags())
    {
        // not supposed to happen
        if (first_f->pkthdr->frag_no() != 0)
        {
            // bogus? what do we do with this? the connection is
            // probably not recoverable at this point.
            fprintf(stderr, "got BOGUS first f frag no %u! bug?\n",
                    first_f->pkthdr->frag_no());
            return 0;
        }

        for (uint32_t ind = 1;
             ind < first_f->pkthdr->num_frags();
             ind++)
        {
            rmi = recv_reassembly.find(seqno + ind);
            if (rmi == recv_reassembly.end())
            {
                stats.missing_frags_detected ++;
                // not present.
                return 0;
            }
            f = rmi->second;

            // not supposed to happen.
            if (f->seqno != (seqno + ind))
            {
                fprintf(stderr, "BOGUS cannot reassemble seqno %u "
                        "with seqno %u\n", seqno, f->seqno);
                return 0;
            }

            // not supposed to happen.
            if (f->pkthdr->frag_no() != ind)
            {
                // bogus? what do we do with this? the connection is
                // probably not recoverable at this point.
                fprintf(stderr, "got BOGUS first frag no %u != %u! bug?\n",
                        f->pkthdr->frag_no(), ind);
                return 0;
            }

            recv_frag_list.push_back(f);
            PRINTF("reassembly list pushed next frag: %s\n",
                   f->print().c_str());
        }
    }

    // now deliver what we got.
    dtls_read_event * dre = read_pool.alloc(-1);
    dre->retcode = GOT_MESSAGE;
    dre->encoded_msg.clear();

    PRINTF("reassembling message from frags:\n");
    for (uint32_t ind = 0; ind < recv_frag_list.size(); ind++)
    {
        dtls_fragment * f = recv_frag_list[ind];
        PRINTF("fragment %u : %s\n", ind, f->print().c_str());
        dre->encoded_msg.append(f->fragment);
        f->delivered = true;
    }
    recv_q.enqueue(dre);

    return recv_frag_list.size();
}

void
ProtoSslDtlsQueue :: push_ondeck_acks(uint32_t seqno)
{
    ondeck_ack_seq_nos.push_back(seqno);
    // if ondeck is >highwater, send now.
    if (ondeck_ack_seq_nos.size() >= config.max_outstanding_acks)
        send_heartbeat();
}

void
ProtoSslDtlsQueue :: handle_recv_ack(uint32_t recv_seq_no)
{
    PRINTF("got ACK for seqno %u\n", recv_seq_no);

    WaitUtil::Lock   lock(&dtls_lock);
    uint32_t seqno = recv_seq_no;
    uint32_t wpos = seqno % config.window_size;

    dtls_fragment * frag = send_window[wpos];
    if (frag == NULL || frag->seqno != seqno)
    {
        // this happens whenever message containing a list of ACKs is
        // lost -- the sender may have retransmitted several packets,
        // because one packet containing many ACKs was lost, even
        // though some of those packets actually made it.
        // that's okay.
        return;
    }
    send_window[wpos] = NULL;
    fragpool.deref(frag);
    lock.unlock();

    // tickle send thread, if it's in full-window condition,
    // it might be able to transmit now. this is an optimization,
    // which speeds things up. if we didn't do this, the tick thread
    // would eventually wake it up anyway.
    // dont use blocking alloc here, because if send_pool is empty,
    // that would be a deadlock, since we're the thread that releases
    // to the pool when acks come in.
    dtls_send_event * dse = send_other_pool.alloc(0);
    if (dse)
    {
        dse->type = dtls_send_event::ACK;
        send_q.enqueue(dse);
    }
}

void
ProtoSslDtlsQueue :: handle_recv_nack(uint32_t recv_seq_no)
{
    // recv thread does not process acks, the send thread does.
    dtls_send_event * dse = send_other_pool.alloc(-1);
    dse->type = dtls_send_event::NACK;
    dse->seqno = recv_seq_no;
    send_q.enqueue(dse);
}

ProtoSslDtlsQueue::send_return_t
ProtoSslDtlsQueue :: send_message(uint32_t queue_number,
                                  const MESSAGE &msg)
{
    PRINTF("SENDING message :\n%s\n", msg.DebugString().c_str());

    if (client == NULL)
        return CONN_SHUTDOWN;

    // locked only when needed. queues are internally safe.
    WaitUtil::Lock   lock(&dtls_lock, false);

    if (queue_number >= num_queues)
    {
        fprintf(stderr, "ProtoSslDtlsQueue :: send_message : "
                "bogus queue_number (%d >= %d)\n",
                queue_number, num_queues);
        return BOGUS_QUEUE_NUMBER;
    }

    if (!msg.IsInitialized())
    {
        // PRINT SOMETHING
        fprintf(stderr, "ProtoSslDtlsQueue :: send_message : "
                "your message is not initialized!\n");
        return MSG_NOT_INITIALIZED;
    }

    uint32_t msg_size = (uint32_t) msg.BYTE_SIZE_FUNC();

    stats.bytes_sent += msg_size;

    // if the message can't fit in the entire window, then we
    // can't send it.
    if (msg_size >= ((config.window_size-1) * config.fragment_size))
    {
        fprintf(stderr, "ProtoSslDtlsQueue :: send_message : "
                "message size %u is bigger than the max message size "
                "we can fragment (window size * fragment_size)!\n",
                (uint32_t) msg.BYTE_SIZE_FUNC());
        return MESSAGE_TOO_BIG;
    }

    const ProtoSslDtlsQueueConfig::queue_config_t &cfg =
        config.get_queue_config(queue_number);
    bool enqueue_to_head = (cfg.qt == ProtoSslDtlsQueueConfig::STACK);

    // for an "unreliable", the message needs to fit in one fragment,
    // be cause we don't do reassembly on unreliables.
    if (cfg.reliable == false && msg_size > config.fragment_size)
    {
        fprintf(stderr, "ProtoSslDtlsQueue :: send_message : "
                "message size %u is bigger than a fragment size on "
                "unreliable queue\n", msg_size);
        return MESSAGE_TOO_BIG;
    }

    switch (cfg.qt)
    {
    case ProtoSslDtlsQueueConfig::STACK:
        // always enqueue to head, but drop from tail when
        // over the limit.
        switch (cfg.lt)
        {
        case ProtoSslDtlsQueueConfig::MESSAGES:
            while ((uint32_t) queues[queue_number]->get_count() >=
                   cfg.limit)
            {
                dtls_send_event * dte =
                    queues[queue_number]->dequeue_tail(0);
                queue_sizes[queue_number] -= dte->encoded_msg.size();
                send_event_release(dte);
            }
            break;

        case ProtoSslDtlsQueueConfig::BYTES:
            lock.lock();
            while (queue_sizes[queue_number] >= cfg.limit)
            {
                dtls_send_event * dte =
                    queues[queue_number]->dequeue_tail(0);
                queue_sizes[queue_number] -= dte->encoded_msg.size();
                send_event_release(dte);
            }
            lock.unlock();
            break;

        case ProtoSslDtlsQueueConfig::NONE:
            // no check.
            break;
        }
        break;

    case ProtoSslDtlsQueueConfig::FIFO:
        // don't enqueue if over the limit.
        switch (cfg.lt)
        {
        case ProtoSslDtlsQueueConfig::MESSAGES:
            if ((uint32_t) queues[queue_number]->get_count() >= cfg.limit)
                return QOS_DROP;
            break;

        case ProtoSslDtlsQueueConfig::BYTES:
            lock.lock();
            if (queue_sizes[queue_number] >= cfg.limit)
                return QOS_DROP;
            lock.unlock();
            break;

        case ProtoSslDtlsQueueConfig::NONE:
            // no check.
            break;
        }
        break;
    }

    // dtls_lock must not be locked here, in case this
    // blocks -- would cause a deadlock.
    dtls_send_event * dte = send_msg_pool.alloc(-1);
    dte->type = dtls_send_event::MSG;
    dte->reliable = cfg.reliable;
    dte->seqno = 0; // not used in MSG, set to 0 to avoid confusion

    // do i need error checking here?
    msg.SerializeToString(&dte->encoded_msg);
    lock.lock();
    queue_sizes[queue_number] += dte->encoded_msg.size();
    lock.unlock();

    if (enqueue_to_head)
        queues[queue_number]->enqueue_head(dte);
    else
        queues[queue_number]->enqueue(dte);

    return SEND_SUCCESS;
}

void
ProtoSslDtlsQueue :: send_thread(void)
{
    // no locks at this level, see called functions.
    dtls_send_event * dte;
    bool done = false;
    dtls_send_event * sender_on_deck = NULL;

    while (!done)
    {
        int queue_number = 0;

        if (sender_on_deck == NULL)
        {
            PRINTF("SEND enter multi dequeue\n");
            dte = dtls_thread_queue_t::dequeue(
                queues, num_queues, /*forever*/-1, &queue_number);
        }
        else
        {
            PRINTF("SEND enter single dequeue\n");
            // send window is full, we can only handle
            // acks and ticks right now. note also
            // queue_number is preinitialized to 0 which
            // matches the fact that send_q is always
            // queues[0].
            dte = send_q.dequeue(/*forever*/-1);
        }

        if (dte == NULL)
            continue;

        switch (dte->type)
        {
        case dtls_send_event::TICK:
            handle_tick();
            break;

        case dtls_send_event::MSG:
            PRINTF("SEND got MSG of size %u\n",
                   (uint32_t) dte->encoded_msg.size());
            if (handle_send_msg(dte, queue_number) == false)
            {
                sender_on_deck = dte;
                // we're keeping this one for a while,
                // don't release it!
                dte = NULL;
            }
            break;

        case dtls_send_event::ACK:
            // the send_window part was already handled
            // for expediency.
            send_other_pool.release(dte);
            // the purpose of this message was just to
            // tickle us to check our ondeck.

            // reuse dte variable.
            dte = sender_on_deck;
            if (dte != NULL)
            {
                // reattempt sending. if we still can't
                // send it, leave it on deck.
                if (handle_send_msg(dte))
                    // sent! not saving anymore,
                    // okay to release it.
                    sender_on_deck = NULL;
                else
                    // still keeping it,
                    // still don't release it.
                    dte = NULL;
            }
            break;

        case dtls_send_event::NACK:
            handle_nack(dte);
            break;

        case dtls_send_event::DIE:
            dte = NULL; // DIE event doesn't come from the pool.
            done = true;
            break;
        }
        send_event_release(dte);
    }

    // if we're going down, we're taking handle_read down with us!
    // this covers the case where the user deletes the object
    // while still having a thread sitting in handle_read.
    // the user's thread will get the disconnect, and finish
    // on it's own.
    dtls_read_event * dre = read_pool.alloc(250000);
    if (dre)
    {
        dre->retcode = GOT_DISCONNECT;
        recv_q.enqueue(dre);
    }
}

void
ProtoSslDtlsQueue :: handle_tick(void)
{
    WaitUtil::Lock   lock(&dtls_lock);

    ticks_without_recv++;
    if (link_up)
    {
        if (ticks_without_recv >=
            (config.max_missed_heartbeats * config.hearbeat_interval))
        {
            fprintf(stderr, "ProtoSslDtlsQueue :: handle_tick : "
                    "LINK_DOWN!\n");
            link_up = false;
            link_changed = true;
        }
    }

    if (link_up)
    {
        for (uint32_t ind = 0;
             ind < send_window.size();
             ind++)
        {
            dtls_fragment * frag = send_window[ind];
            if (frag == NULL)
                continue;
            frag->age++;
            if (frag->age > retransmit_age)
            {
                frag->age = 0;
                stats.frags_resent ++;
                send_frag(frag, "age");
            }
        }
    }

    ticks_without_send++;
    if (ondeck_ack_seq_nos.size() > 0   ||
        ondeck_nack_seq_nos.size() > 0  ||
        ticks_without_send >= config.hearbeat_interval)
    {
        send_heartbeat();
    }

    if (link_changed)
    {
        link_changed = false;
        dtls_read_event * dre = read_pool.alloc(-1);
        dre->retcode = link_up ? LINK_UP : LINK_DOWN;
        recv_q.enqueue(dre);
    }
}

// false return means couldn't send because window full.
bool
ProtoSslDtlsQueue :: handle_send_msg(dtls_send_event *dte,
                                     int queue_number)
{
    WaitUtil::Lock   lock(&dtls_lock);

    if (client == NULL)
        // dont attempt anything! we must be in a race.
        // this means shutdown() is partway through, but we
        // probably have a buildup of messages in send_q
        // and we haven't gotten to the DIE message yet.
        return false;

    const std::string &msg = dte->encoded_msg;

    if (dte->reliable == false)
    {
        // just send it and forget it.
        dtls_fragment * frag = fragpool.alloc();
        frag->fragment = msg;
        send_frag(frag);
        fragpool.deref(frag);
        return true;
    }

    PRINTF("SEND attempting to send dbg seqno %x\n",
           dte->seqno);

    // note queue_number=-1 means we're trying again on an ondeck
    // message, so we don't have to account for it again in queue_sizes.
    if (queue_number >= 0)
        queue_sizes[queue_number] -= dte->encoded_msg.size();

    // round up.
    uint32_t frags_needed =
        (msg.size() + config.fragment_size - 1) / config.fragment_size;

    // first check if there's enough free sequence numbers
    // in the send window to send this message.
    for (uint32_t ind = 0; ind < frags_needed; ind++)
    {
        uint32_t wpos = (send_seqno + ind) % config.window_size;
        if (send_window[wpos] != NULL)
        {
            PRINTF("SEND putting dbg seqno %x on deck because window full\n",
                   dte->seqno);
            // the window is essentially "full".
            // go into flow control state.
            return false;
        }
    }

    // now we can fragment the message, allocate sequence
    // numbers, and send the fragments.
    for (uint32_t ind = 0; ind < frags_needed; ind++)
    {
        uint32_t wpos = send_seqno % config.window_size;
        dtls_fragment * frag = fragpool.alloc();

        PRINTF("SEND MSG queuing fragment %u of message "
               "with dbg seqno %x link seqno %u to wpos %u\n", ind,
               dte->seqno, send_seqno, wpos);

        frag->fragment = msg.substr(ind * config.fragment_size,
                                    config.fragment_size);

        // don't wrap or modulo. the window position is a modulo
        // of this but the actual value is preserved. this is used
        // by the receiver to determine if an ack was lost (i.e. if
        // a seqno was already delivered to the receiver application,
        // it should not be delivered again, instead only the ack
        // should be returned).
        frag->pkthdr->set_sequence_no(send_seqno);
        frag->seqno = send_seqno;
        send_seqno++;

        if (frags_needed > 1)
        {
            frag->pkthdr->set_frag_no( ind );
            frag->pkthdr->set_num_frags( frags_needed );
        }

        send_window[wpos] = frag;
        send_frag(frag);
    }
    // consumed!
    return true;
}

void
ProtoSslDtlsQueue :: handle_nack(dtls_send_event *dte)
{
    WaitUtil::Lock   lock(&dtls_lock);
    uint32_t seqno = dte->seqno;
    uint32_t wpos = seqno % config.window_size;

    dtls_fragment * frag = send_window[wpos];
    if (frag == NULL)
    {
        fprintf(stderr, "ProtoSslDtlsQueue :: handle_nack : "
                "got NACK for slot already freed (%u)!\n", seqno);
        return;
    }

    // retransmit.
    stats.frags_resent ++;
    send_frag(frag, "NACK");
}

// invoke with NULL to just send a heartbeat and maybe
// some acknacks. this should be invoked with dtls_lock locked.
void
ProtoSslDtlsQueue :: send_frag(dtls_fragment *frag, const char *reason)
{
    bool temp = false;
    if (frag == NULL)
    {
        temp = true;
        frag = fragpool.alloc();
    }

    frag->pkthdr->set_window_size(config.window_size);

    if (link_up)
    {
        // we don't send acknacks if we know the other side
        // doesn't seem to be there.
        while (ondeck_ack_seq_nos.size() > 0)
        {
            frag->pkthdr->add_ack_seq_no(ondeck_ack_seq_nos.front());
            ondeck_ack_seq_nos.pop_front();
        }
        while (ondeck_nack_seq_nos.size() > 0)
        {
            frag->pkthdr->add_nack_seq_no(ondeck_nack_seq_nos.front());
            ondeck_nack_seq_nos.pop_front();
        }
    }

    if (frag->fragment.size() > 0)
        frag->pkthdr->set_message_body_size(frag->fragment.size());

    frag_send_buffer.clear();

    // every 5 seconds, populate rtd_req_timestamp
    // to solicit a round trip delay measurement.
    time_t now = time(NULL);
    if ((now - last_rtd_req_time) >= 5)
    {
        pxfe_timeval tv;
        tv.getNow();
        frag->pkthdr->set_rtd_req_timestamp(tv.usecs());
        last_rtd_req_time = now;
    }

    // if we have received a rtd_req, populate rtd_rsp
    // and we'll send a reply. this will be processed in got_frag.
    if (rtd_rsp_timestamp != 0)
    {
        frag->pkthdr->set_rtd_rsp_timestamp(rtd_rsp_timestamp);
        rtd_rsp_timestamp = 0;
    }

    {
        // set up short-lived stack contexts for two objects that
        // don't need overhead of new/delete but do need
        // to be destructed in proper order.
        google::protobuf::io::StringOutputStream zos(&frag_send_buffer);
        {
            google::protobuf::io::CodedOutputStream cos(&zos);
            cos.WriteVarint32((uint32_t) frag->pkthdr->BYTE_SIZE_FUNC());
            frag->pkthdr->SerializeToCodedStream(&cos);
            if (frag->fragment.size() > 0)
                cos.WriteRaw(frag->fragment.c_str(),
                             frag->fragment.size());

        } // cos is destroyed here

    } // zos is destroyed here

    stats.frags_sent ++;

    PRINTF("transmitting fragment: %s\n", frag->print().c_str());

#if PRINT_SEQNOS
    {
        std::ostringstream str;
        bool printit = false;
        str << "                          "
            << "    ---> transmitted t:" << tick << " ";
        if (frag->pkthdr->has_sequence_no())
        {
            str << "seqno " << frag->pkthdr->sequence_no() << " ";
            printit = true;
        }
        else
            str << "         ";
        if (frag->pkthdr->ack_seq_no_size() > 0)
        {
            printit = true;
            str << "acks ";
            for (int ind = 0; ind < frag->pkthdr->ack_seq_no_size(); ind++)
            {
                str << frag->pkthdr->ack_seq_no(ind) << " ";
            }
        }
        if (frag->pkthdr->nack_seq_no_size() > 0)
        {
            printit = true;
            str << "nacks ";
            for (int ind = 0; ind < frag->pkthdr->nack_seq_no_size(); ind++)
            {
                str << frag->pkthdr->nack_seq_no(ind) << " ";
            }
        }
        if (reason)
        {
            printit = true;
            str << "due to " << reason;
        }
        if (printit)
            fprintf(stderr,"%s\n", str.str().c_str());
    }
#endif

    if (client->send_raw(frag_send_buffer) == false)
    {
        fprintf(stderr, "ProtoSslDtlsQueue :: send_frag : "
                "failure during ProtoSSLConnClient::send_raw?\n");
        link_up = false;
        link_changed = true;
    }

    if (temp)
        fragpool.deref(frag);

    ticks_without_send = 0;
}
