# Agent Operational Guidelines & Knowledge Routing

This document specifies mandatory execution policies, domain knowledge routing, and contextual bootstrapping rules for automated agents working across the `pfkutils` codebase.

## GIT permission

Always treat the git database as read-only. You may READ anything you wish (git status, log, diff, show, etc).
IMPORTANT: DO NOT run git add, commit, push, or any other git command which modifies the git database.

---

## Knowledge Routing & Context Bootstrap

### Mandatory Bootstrap Rule
> **Mandatory Bootstrap Rule:** On session start or when receiving tasks touching [DOMAIN], read the corresponding section of `OVERVIEW.md` first.

Under no circumstances should code modifications, refactorings, or architectural assumptions be made without consulting the authoritative technical definitions, invariants, and persistence schemas in `OVERVIEW.md`.

### Developer Intent & Domain Routing Table

| Domain / Task Type | Trigger Keywords | Required Reading Section in `OVERVIEW.md` | Line Offset |
|---|---|---|---|
| **Multi-Call Binary & Build Dispatch** | `Makefile`, `Makefile.inc`, `config/rules`, `pfkutils`, `main.cc`, `programs.h`, `PROG`, `-makelinks`, `-dellinks`, `-config`, `PFK_BUILD_*`, dispatcher, applet | `## Architecture & Multi-Call Dispatcher` | Lines 7–28 |
| **Intrusive Containers & Lists** | `libpfkdll2`, `dll2.h`, `dll2_btree.h`, `dll3.h`, `LList`, `LListHash`, `LListOrderedQ`, `LListLRU`, `LListHashLRU`, `LListBTREE`, `DLL3`, intrusive node, magic checksum | `## Intrusive Containers & Memory Management (libpfkdll2 & libpfkutil/dll3)` | Lines 29–51 |
| **POSIX Wrappers, IPC & Threading** | `libpfkutil`, `posix_fe.h`, `pxfe_*`, `shmempipe`, `thread_slinger`, `thread_states`, `childprocessmanager`, `signal_backtrace`, `simpleUrl`, `newbase64`, `TimerManager` | `## POSIX Abstraction & System Foundation (libpfkutil)` | Lines 52–91 |
| **Storage Engine, Blocks & Disk B-Trees** | `libpfkfb`, `PageIO`, `PageCache`, `BlockCache`, `FileBlock`, `FileBlockInterface`, `FileBlockLocal`, `Btree.h`, `BtreeNode`, `BST_*`, `FB_AUID_T`, `fbdump`, `fbcompact`, `btdump` | `## Storage Engine & Persistence Layer (libpfkfb)` | Lines 92–141 |
| **Deduplicating Backup & Archiving** | `backup`, `pfkbak`, `bakFile`, `bakDatum`, `bakKey`, `bakData`, `BLOBHASH`, `VERSIONINFO`, `VERSIONINDEX`, `FILEINFO`, `tarfile`, snapshot, dedup, content-addressed | `## Deduplicating Backup Subsystem (backup)` | Lines 142–176 |
| **Secure Networking & DTLS/TLS Pipes** | `libprotossl`, `i2`, `i2f`, `i3`, `ProtoSSL*`, `ProtoSslDtlsQueuing`, `i3.proto`, `i3Msg`, `FileData`, `Ping`, `i3_tcp`, `i3_udp`, mbedtls certs, sliding window | `## Secure Networking & Cryptographic Protocols (libprotossl, i2, i3)` | Lines 177–211 |
| **WebSocket Framing & Protobuf Wire** | `libwebsocket`, `simpleWebSocket`, `WebSocketConn`, `WebSocketServerConn`, `WebSocketClientConn`, `CircularReader`, RFC 6455, WebSocket handshake, binary frames | `## WebSocket & Protobuf Messaging (libwebsocket)` | Lines 212–241 |
| **Thread-to-Thread Messaging & HSM** | `t2t2`, `thread2thread2`, `t2t2_pool`, `t2t2_queue`, `t2t2_queue_set`, `pxfe_shared_ptr`, `hsm.h`, `HSMEventBase`, hierarchical state machine, LCA, TRANS | `## Inter-Thread Messaging & Hierarchical State Machines (t2t2)` | Lines 242–284 |
| **Asynchronous Structured Logging** | `squirrel`, `squirrel_log`, `LOG_F`, `LOG_S`, `squirrel_db.schema`, `sql3gen`, `LogMessages`, write-behind logging, log pooling, log row buffer | `## Structured Asynchronous Logging (squirrel)` | Lines 285–317 |
| **ASCII S-Record Encoding & Encryption** | `uuz`, `uuz.proto`, `uuzMsg`, `EncryptedContainer`, `S1`, `S2`, `S3`, `S9`, S-record, 7-bit ASCII, zlib compression, AES-256-CBC, SHA-256-HMAC | `## ASCII Transport Encoding & Archiving (uuz)` | Lines 318–361 |
| **Process Supervision & Terminal Tools** | `pfkscript`, `logfile.cc`, `libpfkscriptutil`, `pfkscript_ctrl`, `pfktop`, `pfkrenice`, `screen.cc`, `tiddb`, `fileparser.cc`, `hilite`, ANSI colors, SIGWINCH | `## Process Supervision & Terminal Tooling (pfkscript, pfktop, hilite)` | Lines 362–388 |
| **Desktop Session & Admin Utilities** | `main/sudo.c`, `MY_UID`, `pfkSessionMgr`, `pfkSessionXrandr`, `sessionManager`, `slowlink`, `ts2`, `tickler`, `he.c`, `rmstar`, `m.c`, `genpw`, `random-hex` | `## Desktop Environment & System Administration Utilities (main)` | Lines 389–415 |
| **Security Boundaries & Cryptography** | setuid root, permissions, AES password encryption, CA validation, peer CN checks, SHA256-HMAC, IV generation, tamper resistance | `## Authentication & Security Invariants` | Lines 416–432 |
| **Third-Party Integrations & Libraries** | mbedtls, protobuf, sqlite3, zlib, X11, Xrandr, ncurses, fftw3, pthreads, libutil, compiler warnings, linker symbols | `## External Integrations & System Dependencies` | Lines 433–448 |
| **CLI Dispatch & Command Reference** | command-line arguments, tool invocation, flags, entry points, subcommands, usage string | `## CLI Entry Points & Execution Reference` | Lines 449–480 |

### Invariant & Safety Checklist for Agents
Before proposing or applying any code changes:
1. **Intrusive Links**: Verify that data structures embedding `LListLinks` or `DLL3::List::Links` are not double-enqueued or cleared without removal.
2. **Persistence Compatibility**: Never alter existing `BST` structures or `BtreeDbClasses` without bumping the schema version (e.g., `CURRENT_DBINFO_VERSION`).
3. **Threading Safety**: Never perform blocking allocations inside real-time threads; utilize `t2t2_pool::alloc` or `thread_slinger` worker queues.
4. **Build System Registration**: When adding a new program or library, update both the subsystem `Makefile.pfk` and `main/programs.h.in` to maintain multi-call dispatcher compatibility.
