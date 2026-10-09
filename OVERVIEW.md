# PFKUtils Codebase Overview

This document provides a comprehensive, highly technical architectural reference for the core components of the `pfkutils` repository. It covers system purpose, runtime architectures, execution flows, persistence mechanisms, protocols, and architectural invariants across all specified modules.

---

## Architecture & Multi-Call Dispatcher

### Repository Build Architecture
The build system is managed via GNU Make using a modular inclusion pattern anchored by `Makefile`, `Makefile.inc`, and `config/rules`:
- **Configuration Resolution**: `Makefile` inspects `$(HOME)/pfk/etc/pfkutils.config.$(NODENAME)` or `$(HOME)/pfk/etc/pfkutils.config` to define target features (`PFK_BUILD_*`), architecture paths, and tool locations.
- **Build Core**: `Makefile.inc` standardizes object placement (`OBJDIR`), automatic dependency generation (`.d`), Lex/Bison workflows (`.l`/`.y`, `.ll`/`.yy`), and Protocol Buffer compilation (`.proto` compiled via `protoc`).
- **Feature Switches**: Defined in `config/rules` (e.g., `PFK_BUILD_pfkbak`, `PFK_BUILD_sudo`, `PFK_BUILD_t2t2`, `PFK_BUILD_protossl`, `PFK_BUILD_uuz`, `PFK_BUILD_i3`, etc.), generating `pfkutils_config.h` at build time.

### Multi-Call Dispatcher (`main/main.cc`)
The majority of standalone command-line tools are compiled into a unified multi-call binary `$(OBJDIR)/pfkutils` located in `main/`:
- **Program Table Generation**: `main/programs.h.in` defines entries via the macro `PROG(priv, name, help)`. During preprocessing, `programs.h` is generated based on active configuration switches.
- **Symbol Binding**: `main/main.cc` expands `PROG` into function declarations `extern "C" int name##_main(int argc, char **argv)` and populates `static struct prog_table prog_table[]`.
- **Dispatch Logic**:
  1. If `argv[0]` matches a known tool name (e.g., via symlink), `main.cc` executes the corresponding `name##_main(argc, argv)`.
  2. If invoked as `pfkutils <subcommand> [args...]`, it shifts `argc`/`argv` and invokes `name##_main`.
  3. Built-in maintenance operations:
     - `pfkutils -makelinks`: Creates symbolic links in the current directory pointing to `pfkutils` for each configured utility.
     - `pfkutils -dellinks`: Unlinks all symlinks matching known program names.
     - `pfkutils -config`: Dumps the embedded build-time configuration string `pfkutils_config`.

---

## Intrusive Containers & Memory Management (libpfkdll2 & libpfkutil/dll3)

The codebase strictly avoids dynamic node allocation in performance-critical data structures, utilizing intrusive linked lists, hash tables, and B-trees.

### `libpfkdll2` (`dll2.h`, `dll2_btree.h`)
An intrusive container library with self-validation and memory corruption detection:
- **Intrusive Node Embedding**: Structures embed `LListLinks<T, strict_walking, use_checksum>` directly as member variables. Insertion and removal do not allocate heap memory (`O(1)` operations).
- **Integrity Validation**: Each link stores magic checksums (`LLIST_MAGIC`, checksum of previous/next pointers). Operations assert that elements are not already on another list, preventing double-insertion and cross-linking bugs.
- **Core Containers**:
  - `LList<T>`: Standard doubly linked list with head/tail enqueue and dequeue.
  - `LListHash<T>`: Self-balancing intrusive hash table. Dynamic bucket array automatically rehashes when load factor thresholds are crossed without requiring node reallocations.
  - `LListOrderedQ<T>`: Priority queue ordered by comparison operators. Optimized for timer wheels and scheduling queues with $O(\ln N)$ insertion and $O(1)$ head dequeue.
  - `LListLRU<T>`: Least-recently-used cache list supporting $O(1)$ promotion of visited nodes to the head.
  - `LListHashLRU<T>`: Compound container combining hash lookup with LRU eviction ordering using dual link members.
  - `LListBTREE<T, ORDER>` (`dll2_btree.h`): In-memory B-tree template parameterized on node order and key types.

### `libpfkutil/dll3` (`dll3.h`, `dll3.tcc`, `dll3_btree.h`)
The next-generation evolution of the intrusive container model:
- Parameterized with template unique IDs and compile-time validation flags (`lockWarn`, `validate`).
- Provides `DLL3::List`, `DLL3::Queue`, and `DLL3::BTree` templates with built-in concurrency access assertions and strict node state validation.

---

## POSIX Abstraction & System Foundation (libpfkutil)

`libpfkutil` provides common platform primitives, RAII wrappers, IPC, and thread synchronization.

### POSIX Front-End (`posix_fe.h`)
Encapsulates POSIX APIs in RAII C++ classes with structured error propagation via `pxfe_errno`:
- **Networking**:
  - `pxfe_socket`: Generic socket wrapper.
  - `pxfe_tcp_stream_socket`: TCP client/server with IPv4/IPv6 support, non-blocking connect, backlog management, and SO_REUSEADDR.
  - `pxfe_udp_datagram_socket`: UDP socket with unicast/multicast send/receive.
  - `pxfe_unix_stream_socket` & `pxfe_unix_dgram_socket`: UNIX domain sockets for local IPC.
- **Event Loops**:
  - `pxfe_select`: RAII wrapper around `fd_set`, read/write/error sets, and timeval timeouts.
  - `pxfe_ticker`: Monotonic periodic interval timer delivering file descriptor readability for timer integration in `select()` loops.
- **Threading & Synchronization**:
  - `pxfe_pthread`: Thread lifecycle management with attributes and join mechanics.
  - `pxfe_pthread_mutex` & `pxfe_pthread_cond`: Mutexes supporting robust error checking and process-shared (`PTHREAD_PROCESS_SHARED`) attributes.
  - `pxfe_pthread_rwlock` & `pxfe_barrier`: Reader-writer locks and thread rendezvous barriers.
- **Process Management**:
  - `pxfe_pipe`: Unidirectional POSIX pipe wrapper.
  - `pxfe_sigaction` & `pxfe_sigset`: Signal mask manipulation and signal handler installation.

### High-Performance Shared Memory Pipe (`shmempipe.h`, `shmempipe.cc`)
A bidirectional, lock-assisted inter-process communication ring buffer:
- Implemented over POSIX shared memory (`shm_open`) and memory mapping (`mmap`).
- Synchronized using process-shared robust pthread mutexes (`PTHREAD_MUTEX_ROBUST`) and condition variables.
- Survives unexpected peer termination; detects abandoned locks via `EOWNERDEAD` and recovers state cleanly.

### Thread Dispatching & Supervision
- **Thread Slinger (`thread_slinger.h`, `thread_slinger.tcc`)**: Asynchronous worker thread queue implementation. Producers sling typed messages into thread queues; worker loops process them asynchronously without contention.
- **Thread States (`thread_states.h`, `thread_states.cc`)**: Shared-memory, lock-free thread activity monitor. Tracks per-thread execution state strings, timestamps, and progress indicators for debugging deadlocks or hung tasks.
- **Child Process Manager (`childprocessmanager.h`, `childprocessmanager.cc`)**: Manages child process lifecycles non-blockingly, intercepting `SIGCHLD`, capturing stdout/stderr pipes, and tracking process exit codes.
- **Signal Backtrace (`signal_backtrace.h`, `signal_backtrace.cc`)**: Installs fault handlers for `SIGSEGV`, `SIGBUS`, `SIGILL`, and `SIGFPE`, dumping demangled stack backtraces to stderr via `backtrace()` and `dladdr()`.
- **URL & Encoding Utilities**:
  - `simpleUrl.h`: URL parsing (scheme, user, password, host, port, path, query).
  - `newbase64.h`: High-speed Base64 encoder/decoder supporting 9 standard variants (RFC 4648, URL-safe, MIME, crypt, etc.).
  - `TimerManager.h` & `timers.h`: Scalable timer scheduler executing registered callbacks on expiration.

---

## Storage Engine & Persistence Layer (libpfkfb)

`libpfkfb` is a transactional, block-structured, disk-backed storage engine with built-in caching, dynamic block allocation, compaction, and disk B-trees.

```
+-----------------------------------------------------------+
|              Application Layer (backup, btdump)           |
+-----------------------------------------------------------+
|       Btree (Btree.h)     /   BST (BtreeDbClasses.h)      |
+-----------------------------------------------------------+
|       FileBlockInterface / FileBlockLocal (AUID Storage)  |
+-----------------------------------------------------------+
|          BlockCache (Sub-page chunk buffering)            |
+-----------------------------------------------------------+
|           PageCache (4KB page buffer pool & LRU)          |
+-----------------------------------------------------------+
|    PageIO (FileDescriptor / DirectoryTree / TCP / AES)    |
+-----------------------------------------------------------+
```

### Layer Hierarchy
1. **`PageIO` (`PageIO.h`)**:
   - Manages physical 4KB pages.
   - Implementations: `PageIOFileDescriptor` (standard files), `PageIODirectoryTree` (pages spread across nested directories), and `PageIONetworkTCPServer` (remote networked pages).
   - **Page Encryption**: When a filename is supplied as `filename:password`, `PageIO` derives cryptographic keys and encrypts all 4KB pages transparently via mbedtls AES.
2. **`PageCache` (`PageCache.h`, `PageCache.cc`)**:
   - In-memory cache of 4KB pages with a user-configurable memory cap (`max_bytes`).
   - Maintains dirty page tracking, flushing modifications during explicit commits or eviction.
3. **`BlockCache` (`BlockCache.h`)**:
   - Manages allocations and caches sub-page data regions.
4. **`FileBlockInterface` & `FileBlockLocal` (`FileBlock_iface.h`, `FileBlockLocal.h`)**:
   - Provides dynamic variable-sized allocation units identified by a 32-bit `FB_AUID_T` handle.
   - Blocks range from 1 byte to 512KB (sub-divided across internal size buckets).
   - **Methods**: `alloc(size)`, `realloc(auid, new_size)`, `free(auid)`, `get(auid, for_write)`, `release(blk, dirty)`.
   - **Compaction**: `compact(status_func, arg)` defragments free space and shifts blocks while preserving permanent AUID mappings.
5. **`Btree` (`Btree.h`, `BtreeNode.cc`)**:
   - Disk-backed balanced tree indexing variable-length byte keys (`uint8_t *key, int len`) to 32-bit data handles (typically an `FB_AUID_T`).
   - Created with `Btree::createFile(path, cache_bytes, mode, order)` (order must be an odd integer, e.g., 25).
   - Operations: `put(key, len, data_id, replace)`, `get(key, len, &data_id)`, `del(key, len, &old_data_id)`, and `iterate(BtreeIterator*)`.
6. **Binary Serialization Tools (`BtreeDbClasses.h`, `libpfkutil/bst.h`)**:
   - Sun XDR-like serialization framework preventing platform padding/endian issues in B-tree keys and values.
   - Types: `BST_STRING`, `BST_UINT32_t`, `BST_UINT64_t`, `BST_TIMEVAL`, `BST_ARRAY<T>`, `BST_UNION`.

### Maintenance CLI Tools (`main/`)
- `fbdump`: Parses and prints low-level `FileBlock` allocation maps, headers, buckets, and free lists.
- `fbcompact`: Standalone utility to compact and defragment a `FileBlock` database file.
- `btdump`: Traverses a disk `Btree`, dumping internal nodes, leaf nodes, key distribution, and tree depth.

---

## Deduplicating Backup Subsystem (backup)

### System Overview & Entry Point
The `backup` module compiles into `libpfkbak.a` and is exposed via the `pfkbak` applet (`pfkbak_main` in `backup/main.cc`). It implements an incremental, deduplicated snapshot archiver using `libpfkfb` and SHA-256 content hashing.

### Database Architecture & Schema (`backup/database_items.h`)
The backup engine uses two distinct files:
1. `*.index`: A `libpfkfb` B-tree database storing serialized metadata objects (`bakDatum`).
2. `*.data`: A `libpfkfb` raw file block repository storing 64KB chained data chunks (`bakFileContents`).

#### B-Tree Key & Data Schema
`bakKey` and `bakData` unions map the database hierarchy:
- `DBINFO` (`key.dbinfo = "BKDBINFOKEY"`): Root record containing source directory path, current schema version, next version number, and an array of all active backup version IDs.
- `VERSIONINFO` (`key.version`): Stores timestamp, file count, and aggregate byte size for a specific snapshot version.
- `VERSIONINDEX` (`key.version, key.group`): Arrays of up to 100 filenames per group, indexing all files belonging to a version.
- `FILEINFO` (`key.version, key.filename`): Metadata for a file in a given snapshot: SHA-256 hash, modification time (`pxfe_timeval`), file size, and symlink destination.
- `BLOBHASH` (`key.hash, key.filesize`): Content-addressable file blob index. Stores reference count and the `FB_AUID_T` pointer to the head chunk of file data.

### Storage and Execution Workflows
- **`create` / `update` (`backup/create.cc`, `backup/update.cc`)**:
  1. Recurses through the source directory, calling `stat()` on each file.
  2. Compares modification time and size against the prior version's `FILEINFO`.
  3. If changed or new, computes the SHA-256 hash of the file contents.
  4. Looks up `BLOBHASH` in the index:
     - **Match**: Increments blob reference count without writing data blocks.
     - **Miss**: Reads file in 64KB chunks, allocating `FB_AUID_T` blocks in the `.data` store, linking chunks via `bakFileContents::next_auid`, and registering the new `BLOBHASH` with refcount 1.
  5. Records `FILEINFO` and appends to `VERSIONINDEX`.
- **`extract` / `export` (`backup/extract.cc`, `backup/export.cc`, `backup/tarfile.cc`)**:
  - Reconstructs files from a given version ID by chasing AUID chunk chains.
  - `export` streams the reconstructed tree directly into a GNU tar archive (`tarfile.cc`) with accurate permissions, timestamps, and symlinks.
- **`deletevers` (`backup/deletevers.cc`)**:
  - Removes a snapshot version. Traverses files, decrements `BLOBHASH` refcounts, and frees unreferenced `FB_AUID_T` chunk chains via `FileBlockInterface::free()`.

---

## Secure Networking & Cryptographic Protocols (libprotossl, i2, i3)

### `libprotossl` (`libprotossl.h`, `protossl*.cc`)
A high-level network library bridging mbedtls SSL/TLS/DTLS with Google Protocol Buffers.
- **Certificate Handling (`ProtoSSLCertParams`)**: Loads CA root certificates, host certificates, private keys, and key passwords.
- **Context Manager (`ProtoSSLMsgs`)**: Configures entropy sources (`mbedtls_entropy_context`), CTR-DRBG generators (`mbedtls_ctr_drbg_context`), SSL configurations, and cookie verification for DTLS.
- **Connection Primitives**:
  - `ProtoSSLConnClient` & `ProtoSSLConnServer`: Support non-blocking TCP (TLS) and UDP (DTLS) connections.
  - Protobuf framing: Automatically prefixes messages with wire size, parsing incoming streams into `google::protobuf::Message` instances.
- **DTLS Sliding-Window Protocol (`protossldtlsqueuing.proto`, `protossldtlsqueuing.cc`)**:
  - Implements reliable, in-order packet streaming over UDP/DTLS.
  - Provides packet chunk fragmentation, sliding-window acknowledgment (`max_outstanding_acks`), negative acknowledgments (NACKs), and heartbeat intervals.

### `i2` & `i2f` (`i2/`)
- **`i2` (`i2_main.cc`, `i2_options.cc`)**:
  - Advanced Netcat alternative. Establishes TCP connections (client or server).
  - Can stream from/to stdin, stdout, regular files, `/dev/urandom`, or `/dev/zero`.
  - Employs a non-blocking `select()` loop with `pxfe_ticker` for real-time throughput reporting.
- **`i2f` (`i2f_main.cc`, `i2f_options.cc`)**:
  - Multi-port TCP proxy/forwarder.
  - Concurrently listens on multiple local ports, dynamically establishing forward connections to remote hosts and bridging traffic bidirectionally.

### `i3` (`i3/`)
Secure data transfer utility operating over `libprotossl` with mutual authentication:
- **Protocol Schema (`i3.proto`)**:
  - `i3Msg`: Multi-purpose message envelope.
  - `FileData`: Carries data chunks (sized under 16KB to fit within a single TLS frame) with an optional embedded `Ping`.
  - `FileDone`: Concludes transfer with final byte count and 32-byte SHA-256 checksum.
  - `Ping` / `ping_ack`: Dynamic sender flow control. When ping/ack mode is enabled, the sender limits inflight chunks, awaiting acknowledgement before dispatching further blocks.
- **Engines**:
  - `i3_tcp_main` (`i3_main_tcp.cc`): Uses TLS over TCP.
  - `i3_udp_main` (`i3_main_udp.cc`): Uses DTLS with sliding-window packet queueing over UDP.

---

## WebSocket & Protobuf Messaging (libwebsocket)

`libwebsocket` implements RFC 6455 WebSocket framing directly in C++, serializing and deserializing Google Protocol Buffers over WebSocket binary frames.

```
+-----------------------------------------------------------+
|          Application Protobuf Messages (User Schema)      |
+-----------------------------------------------------------+
|  WebSocketConn (handle_message / sendMessage / RFC 6455)  |
+-----------------------------------------------------------+
|  Handshake Engine (WebSocketServerConn / ClientConn)      |
+-----------------------------------------------------------+
|       CircularReader (Circular byte buffer stream)        |
+-----------------------------------------------------------+
|             POSIX TCP Socket (pxfe_socket)                |
+-----------------------------------------------------------+
```

### Core Architecture (`simpleWebSocket.h`)
- **`CircularReader` (`CircularReader.h`)**: Ring buffer managing non-blocking socket stream consumption. Supports substring searches, line extraction, and zero-copy slicing for HTTP header parsing.
- **Handshake Negotiation**:
  - `WebSocketServerConn`: Parses HTTP GET upgrade requests, extracts `Sec-WebSocket-Key`, computes the SHA-1 response with the standard GUID salt (`258EAFA5-E914-47DA-95CA-C5AB0DC85B11`), and returns `101 Switching Protocols`.
  - `WebSocketClientConn`: Formulates upgrade requests, generates keys, and verifies server acceptance.
- **Frame Processing (`WebSocketConn`)**:
  - Parses RFC 6455 frame headers: FIN bit, opcode (Continuation, Text, Binary, Close, Ping, Pong), mask key, and 7-bit / 16-bit / 64-bit payload lengths.
  - Unmasks client-sent frames in-place.
  - Serializes outgoing `google::protobuf::Message` instances directly into binary frames; reassembles fragmented incoming frames before invoking `handle_message()`.

---

## Inter-Thread Messaging & Hierarchical State Machines (t2t2)

### Thread2Thread2 Engine (`t2t2/thread2thread2.h`)
A deterministic, high-throughput C++ inter-thread messaging framework avoiding dynamic heap allocation during message transfer.

```
+-------------------------------------------------------------+
|   t2t2_pool<BaseT, Derived...> (Fixed-size buffer storage)  |
+-------------------------------------------------------------+
               | alloc()                           ^ release()
               v                                   |
+-----------------------------------+              |
| pxfe_shared_ptr<Msg> (Intrusive)  |--------------+
+-----------------------------------+
               | enqueue()
               v
+-------------------------------------------------------------+
|   t2t2_queue<BaseT> / t2t2_queue_set (FIFO Wait / Select)   |
+-------------------------------------------------------------+
```

- **Fixed-Size Buffer Pools (`t2t2_pool<BaseT, derivedTs...>`)**:
  - Allocates a contiguous pool of uniform buffers sized to `largest_type<BaseT, derivedTs...>::size`.
  - Messages derive from `t2t2_message_base<BaseT>`.
  - Allocations (`pool.alloc(&sp_msg, wait_ms)`) pull pre-allocated slots. Supports non-blocking (`NO_WAIT`), blocking (`WAIT_FOREVER`), or dynamic pool expansion (`GROW`).
- **Queues & Multi-Queue Select (`t2t2_queue`, `t2t2_queue_set`)**:
  - `t2t2_queue`: Thread-safe FIFO message queue protected by robust mutexes and condition variables.
  - `t2t2_queue_set`: Multiplexes multiple queues, allowing a worker thread to block until any queue in the set receives a message (equivalent to `select()` across queues).
- **Intrusive Smart Pointers (`pxfe_shared_ptr`)**:
  - Embedded atomic reference counting in the message header. When refcount reaches zero, the buffer returns to its owning `t2t2_pool` without invoking `free()` or `delete`.
- **Diagnostics**: Custom assertion hook `t2t2_assert_handler` catches double frees, buffer corruptions, and invalid list operations.

### Hierarchical State Machine Framework (`t2t2/hsm.h`)
A UML-compliant hierarchical state machine engine built upon `t2t2`:
- **State Hierarchy Actions**:
  - Handlers return `HANDLED`, `TRANS` (state transition), `SUPER` (delegate to parent superstate), or `TOP` (root reached).
  - Automatically dispatches `ENTRY` and `EXIT` events across the least common ancestor (LCA) path when transitioning states.
- **Event Modeling (`HSMEventBase`)**:
  - Typed events derive from `t2t2_message_base<HSMEventBase>`, enabling zero-copy pooling through `t2t2_pool`.
- **Debugging**: `HsmFuncRegex` parses demangled compiler symbols (`__PRETTY_FUNCTION__`) via regular expressions to emit state traversal trace logs.

---

## Structured Asynchronous Logging (squirrel)

`squirrel` is an asynchronous, SQLite3-backed structured logging subsystem designed for minimal latency overhead in calling threads.

```
+-------------------------------------------------------------+
|       Calling Threads (LOG_F / LOG_S / log_row_buf_ref)     |
+-------------------------------------------------------------+
                               |
                               | ThreadSlinger (Lock-free FIFO)
                               v
+-------------------------------------------------------------+
|   Background Logger Thread (Batching & SQLite3 Insertion)   |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|   SQLite3 Database File (*.db) / Table: LogMessages         |
+-------------------------------------------------------------+
```

### Schema & Code Generation (`squirrel_db.schema`)
The table definition is compiled via `sql3gen`:
- Fields: `time_text` (ISO-8601 string), `area` (subsystem string), `level` (CRIT, ERR, WARN, INFO, DBG, TRACE), `message` (log text), `file`, `line`, `process`, `pid`, `thread`, `tid`, `backtrace` (symbolic stack trace), and `timestamp` (epoch double).
- Generates C++ SQLite ORM wrappers and Protocol Buffer schemas (`squirrel_db.proto`).

### Engine Architecture (`squirrel_log.h`, `squirrel_log.cc`)
- **Write-Behind Logging**: Worker threads invoke formatted logging (`LOG_F`) or stream logging (`LOG_S`). Log records are acquired from a pre-allocated pool (`log_row_buf_ref`), populated, and slung to the logger thread via `ThreadSlinger`.
- **Zero Allocations**: Pre-allocated row pools eliminate heap churn in critical paths.
- **Crash Backtraces**: On critical errors or fatal signals, integration with `signal_backtrace.h` captures symbolic call stacks directly into the `backtrace` column.

---

## ASCII Transport Encoding & Archiving (uuz)

### System Purpose & Architecture (`uuz/`)
`uuz` is a secure 7-bit ASCII transmission archiver inspired by `uuencode`/`uucp` and Motorola S-records. It packages files into printable ASCII streams capable of traversing restrictive, 7-bit clean communication channels.

```
+---------------------------------------------------------------+
|                   Source Files / Payloads                     |
+---------------------------------------------------------------+
                               |
                               v
+---------------------------------------------------------------+
|      Layer 1: Compression (zlib at level 9 / gzip -9)         |
+---------------------------------------------------------------+
                               |
                               v
+---------------------------------------------------------------+
|  Layer 2: Authenticated Encryption (AES-256-CBC + SHA256-HMAC)|
+---------------------------------------------------------------+
                               |
                               v
+---------------------------------------------------------------+
|     Layer 3: Protocol Buffers (uuz.proto / EncryptedContainer)|
+---------------------------------------------------------------+
                               |
                               v
+---------------------------------------------------------------+
| Layer 4: ASCII S-Record Framing ([sS][1239] + Base64 / Hex)   |
+---------------------------------------------------------------+
```

### Protocol & Record Framing (`uuz.proto`, `uuz/README.txt`)
All data records are encoded into lines of at most 78 characters:
- Prefix character: Lowercase `s` marks continuation lines; uppercase `S` marks the terminal line of a logical record.
- Second character: Record type digit:
  - `S1` (**Version Record**): The unencrypted master record. Always encoded in hexadecimal. Defines protocol version, chosen Base64 variant, compression setting, encryption setting, and HMAC salt rules.
  - `s2` / `S2` (**File Info**): Encodes `FileInfo` (filename, file size, permissions). In encrypted mode, supplies the starting AES initialization vector (IV).
  - `s3` / `S3` (**File Data**): Encodes `FileData` containing chunk offset and binary payload. Continuously updates CBC IVs across chunks.
  - `s9` / `S9` (**File Complete**): Encodes `FileComplete` with total compressed byte length and SHA-256 checksum for end-to-end verification.
- **Security Envelope (`EncryptedContainer`)**:
  - When encryption is enabled, `uuzMsg` payloads are padded, encrypted with AES-256-CBC, and wrapped in an `EncryptedContainer` alongside salt, data length, and SHA-256-HMAC signatures.

---

## Process Supervision & Terminal Tooling (pfkscript, pfktop, hilite)

### `pfkscript` (`pfkscript/`)
Enhanced terminal session recording and multiplexing:
- **PTY Architecture (`main.cc`)**: Uses `pty.h` (`openpty`/`forkpty`) to allocate a pseudo-terminal for the shell, transparently proxying stdin/stdout while logging session streams to disk.
- **Log Management (`logfile.cc`)**:
  - Real-time size-based and time-based log rotation.
  - Automatic compression of rotated logs via `gzip`, `bzip2`, or `xz`.
- **Control Socket (`libpfkscriptutil.h`, `libpfkscriptutil.cc`)**:
  - Exposes a UNIX domain datagram socket whose path is exported via an environment variable.
  - Subprocesses or control scripts use `pfkscript_ctrl` to query the current active log path, force immediate log rollover, or close/reopen logs on demand.
- **Network Relay**: Can bind a listening TCP port to stream live PTY session output to remote network viewers.

### `pfktop` & `pfkrenice` (`pfktop/`)
An interactive Linux process and thread monitor:
- **ProcFS Parser (`fileparser.cc`)**: Reads `/proc/stat`, `/proc/loadavg`, `/proc/meminfo`, `/proc/[pid]/stat`, and iterates `/proc/[pid]/task/[tid]/stat`.
- **Thread Tracking (`tiddb.cc`, `tidentry.cc`)**: Maintains per-thread history to calculate exact delta CPU utilization for individual POSIX threads across update intervals.
- **Terminal Rendering (`screen.cc`)**: VT100/ANSI terminal management without standard curses overhead. Intercepts `SIGWINCH` via pipe to dynamically reflow data on terminal resize.
- **`pfkrenice` (`pfkrenice.cc`)**: Interactive utility to modify scheduling niceness of specific threads and processes.

### `hilite` (`hilite/`)
Stream highlighting utility:
- Reads configuration files (`config_file.cc`) containing regex patterns mapped to ANSI color specifications (`color.cc` - red, green, blue, bold, inverse, etc.).
- Compiles expressions via POSIX `regex.h` and intercepts standard input streams to colorize matching tokens in real time.

---

## Desktop Environment & System Administration Utilities (main)

### Privileged Execution (`main/sudo.c`, `main/sudo.h.in`)
- Personal setuid-root command wrapper.
- **Invariant**: Hardcodes `MY_UID` at build time. When executed, checks `getuid()`. If caller UID matches `MY_UID` or 0, it calls `setuid(0)` and `setgid(0)` to run the command without password prompts. Denies execution to all other UIDs.

### Session Management & Display Control
- **`pfkSessionMgr` (`main/sessionManager.cc`, `sessionManager.h`)**:
  - X11 desktop session supervisor. Launches child commands, tracks PIDs, and monitors the X server connection.
  - When the X server shuts down, automatically reaps and terminates all session children.
  - Controls running sessions via UNIX domain sockets or signals (`SIGUSR1` restart, `SIGUSR2` stop).
- **`pfkSessionXrandr` (`main/sessionXrandr.cc`)**:
  - Listens for X11 Xrandr display configuration change events.
  - Signals `pfkSessionMgr` to adjust desktop layout or restart panels on display connection/resolution changes.

### System Utilities
- **`slowlink` (`main/slowlink.cc`)**: Traffic shaping proxy simulating bandwidth restrictions and latency delays on network connections.
- **`ts2` (`main/ts2.cc`)**: Multi-threaded directory tree scanner and file integrity verifier.
- **`tickler` (`main/tickler.cc`)**: Daemon performing periodic non-destructive disk reads across mount points to prevent aggressive spindown on storage arrays.
- **`he` (`main/he.c`)**: Ncurses interactive binary file hexadecimal editor.
- **`rmstar` (`main/rmstar.cc`)**: Recursive `rm -rf` implementation with verbose progress reporting and sanity confirmations.
- **`m` (`libpfkutil/m.c`, `libpfkutil/m.h`)**: Command-line programmer's arithmetic calculator supporting arbitrary hexadecimal, decimal, and binary operations.
- **`waitpid` (`main/waitpid.c`)**: Utility waiting for arbitrary external process IDs to exit before terminating.
- **`genpw`, `random-hex` (`main/genpw.c`, `main/random-hex.c`)**: High-entropy password and random hex generator using system cryptographic random devices.

---

## Authentication & Security Invariants

1. **Privilege Escalation (`main/sudo.c`)**:
   - `sudo` binary must be owned by `root` with mode `4755` (setuid).
   - Only processes whose real UID matches `MY_UID` (defined during compilation from `id -u`) or `0` are permitted to execute commands as root.
2. **Database & Page-Level Encryption (`libpfkfb/PageIO.h`)**:
   - Files created with `filename:password` encrypt all underlying 4KB pages using AES-CBC via mbedtls. Disk dumps without the password reveal only ciphertext.
3. **Network Transport Security (`libprotossl`, `i3`)**:
   - All TLS/DTLS endpoints enforce certificate validation against trusted CA paths (`ProtoSSLCertParams::caCert`).
   - Host identity checks verify common names (`ProtoSSLPeerInfo::common_name`).
4. **Archival Integrity & Encryption (`uuz`)**:
   - Encrypted archives require valid passphrases to derive AES-256 keys via SHA-256.
   - Every data chunk is authenticated using SHA-256-HMAC with unique per-chunk salts; corrupt or tampered records fail validation before decompression.
   - Master version blocks (`S1`) are always unencrypted to allow parsers to identify format parameters before decoding.

---

## External Integrations & System Dependencies

| Component | Purpose | Key Header / Library |
|---|---|---|
| **mbedtls** | TLS/DTLS, AES-256-CBC, SHA-256, HMAC, CTR-DRBG | `<mbedtls/ssl.h>`, `<mbedtls/aes.h>`, `-lmbedcrypto`, `-lmbedtls` |
| **Protocol Buffers** | Structured message schemas and framing | `<google/protobuf/message.h>`, `-lprotobuf` |
| **SQLite3** | Structured database storage in `squirrel` | `<sqlite3.h>`, `-lsqlite3` |
| **zlib** | Archive compression in `uuz` | `<zlib.h>`, `-lz` |
| **X11 / Xrandr** | Desktop session monitoring and display events | `<X11/Xlib.h>`, `<X11/extensions/Xrandr.h>`, `-lX11`, `-lXrandr` |
| **ncurses** | Terminal display in `he` hex editor | `<ncurses.h>`, `-lncurses` |
| **fftw3** | Fast Fourier Transform in `do_fft` | `<fftw3.h>`, `-lfftw3` |
| **POSIX pthreads / rt** | Threading, realtime clocks, POSIX shared memory | `<pthread.h>`, `<sys/mman.h>`, `-lpthread`, `-lrt` |
| **libutil** | Terminal PTY allocation for `pfkscript` | `<pty.h>`, `-lutil` |

---

## CLI Entry Points & Execution Reference

| Binary Name / Symlink | Source File | Implementation / Class | Purpose |
|---|---|---|---|
| `pfkbak` | `backup/main.cc` | `bakFile` (`backup/bakfile.h`) | Content-addressable deduplicating backup tool |
| `i2` | `i2/i2_main.cc` | `i2_program` | High-performance TCP streaming pipe |
| `i2f` | `i2/i2f_main.cc` | `i2f_program` | Multi-port asynchronous TCP proxy forwarder |
| `i3_tcp` | `i3/i3_main_tcp.cc` | `i3_tcp_program` | Protobuf + TLS encrypted file/stream transfer |
| `i3_udp` | `i3/i3_main_udp.cc` | `i3_udp_program` | Protobuf + DTLS reliable packet streaming |
| `pfkscript` | `pfkscript/main.cc` | `Pfkscript_program` | PTY session logger with automated log rotation |
| `pfktop` | `pfktop/pfktop.cc` | `pfktop::PidList` | Per-thread CPU and memory process top monitor |
| `pfkrenice` | `pfktop/pfkrenice.cc` | `pfktop_main` | Interactive process/thread renice utility |
| `uuz` | `uuz/uuz.cc` | `PFK_uuz::uuz` | 7-bit ASCII compressing and encrypting encoder |
| `hilite` | `hilite/hilite.cc` | `Hilite_config` | Regex-based ANSI stream colorizer |
| `sudo` | `main/sudo.c` | `sudo_main` | UID-restricted setuid-root command runner |
| `pfkSessionMgr` | `main/sessionManager.cc` | `sessionManager` | X11 desktop session and process lifecycle supervisor |
| `pfkSessionXrandr` | `main/sessionXrandr.cc` | `xrandr_monitor` | X11 display resolution/output monitor |
| `fbdump` | `main/fbdump.cc` | `FileBlockLocal` | `libpfkfb` database structure and bucket dumper |
| `fbcompact` | `main/fbcompact.cc` | `FileBlockLocal::compact` | `libpfkfb` offline compaction and defragmenter |
| `btdump` | `main/btdump.cc` | `Btree` | `libpfkfb` B-tree node and key traversal dumper |
| `slowlink` | `main/slowlink.cc` | `slowlink` | Bandwidth limiter and network latency simulator |
| `ts2` | `main/ts2.cc` | `ts2` | Multi-threaded file tree scanner and checker |
| `he` | `main/he.c` | `he_main` | Ncurses interactive hexadecimal file editor |
| `rmstar` | `main/rmstar.cc` | `rmstar_main` | Safe, verbose recursive file remover |
| `tickler` | `main/tickler.cc` | `tickler_main` | Periodic disk I/O keepalive daemon |
| `m` | `libpfkutil/m.c` | `m_main` | Command-line programmer's arithmetic calculator |
| `genpw` | `main/genpw.c` | `genpw_main` | Cryptographic random password generator |
| `random_hex` | `main/random-hex.c` | `random_hex_main` | Random hexadecimal token generator |
| `waitpid` | `main/waitpid.c` | `waitpid_main` | Wait for process IDs to exit |
