# libprotossl

`libprotossl` is a high-level C++ network library that bridges **mbedtls** (SSL/TLS/DTLS) with **Google Protocol Buffers**. It provides secure connection primitives for TCP and UDP, as well as a robust reliable sliding-window framing protocol (`ProtoSslDtlsQueue`) over DTLS.

---

## What It Does

1. **Certificate & Crypto Management (`ProtoSSLCertParams`, `ProtoSSLMsgs`)**:
   - Handles loading of CA root certificates, host certificates, private keys, and encrypted key passwords.
   - Manages mbedtls entropy sources, CTR-DRBG random generators, SSL configurations, and cookie verification for DTLS.
2. **Connection Primitives (`ProtoSSLConnClient`, `ProtoSSLConnServer`)**:
   - Provides non-blocking TCP (TLS) and UDP (DTLS) connection handling.
   - Automatically prefixes messages with wire size, parsing incoming streams directly into `google::protobuf::Message` instances.
3. **DTLS Sliding-Window Protocol (`ProtoSslDtlsQueue`)**:
   - Implements reliable, in-order packet streaming over UDP/DTLS.
   - Supports packet chunk fragmentation, sliding-window acknowledgment (`max_outstanding_acks`), negative acknowledgments (NACKs), round-trip delay (RTD) measurement, automatic retransmissions, and multi-priority queuing with backpressure limits.

---

## API Usage Examples

### 1. Initializing Context & Loading Certificates
```cpp
#include "libprotossl.h"

// Specify certificate file paths (prefixed with "file:")
std::string ca_file  = "file:keys/Root-CA.crt";
std::string cert_file= "file:keys/Server-Cert.crt";
std::string key_file = "file:keys/Server-Cert-plain.key";

ProtoSSL::ProtoSSLCertParams cert_params(ca_file, cert_file, key_file, "");

// Create the context manager (non-blocking, TCP or UDP)
ProtoSSL::ProtoSSLMsgs msgs(/*nonblock*/ true, /*debug*/ false,
                            /*read_timeout*/ 0, /*use_tcp*/ false,
                            /*dtls_timeout_min*/ 1000, /*dtls_timeout_max*/ 2000);

if (!msgs.loadCertificates(cert_params) || !msgs.validateCertificates()) {
    // Handle error
}
```

### 2. Setting Up a Server with DTLS Queuing
```cpp
// Start server listener on port 18492
ProtoSSL::ProtoSSLConnServer *server = msgs.startServer(18492);

// Configure reliable DTLS queues
ProtoSSL::ProtoSslDtlsQueueConfig dtlsq_config;
dtlsq_config.fragment_size = 1250;
dtlsq_config.window_size = 1000;

uint32_t queue_id = 0;
dtlsq_config.add_queue(queue_id, /*reliable*/ true,
                       ProtoSSL::ProtoSslDtlsQueueConfig::FIFO,
                       ProtoSSL::ProtoSslDtlsQueueConfig::NONE, 0);

// Accept incoming client connection
ProtoSSL::ProtoSSLConnClient *newclient = server->handle_accept();
if (newclient) {
    // Wrap connection in reliable DTLS queue engine
    ProtoSSL::ProtoSslDtlsQueue *dtlsq = new ProtoSSL::ProtoSslDtlsQueue(dtlsq_config, newclient);
    if (dtlsq->ok()) {
        // Ready for send/receive
    }
}
```

### 3. Sending and Receiving Protobuf Messages
```cpp
// Sending a message on a specific queue
google::protobuf::Message &msg = ...;
dtlsq->send_message(queue_id, msg);

// Receiving messages
google::protobuf::Message &incoming_msg = ...;
ProtoSSL::ProtoSslDtlsQueue::read_return_t ret = dtlsq->handle_read(incoming_msg);
switch (ret) {
    case ProtoSSL::ProtoSslDtlsQueue::GOT_MESSAGE:
        // Process incoming_msg
        break;
    case ProtoSSL::ProtoSslDtlsQueue::GOT_DISCONNECT:
        // Handle disconnect
        break;
    default:
        break;
}
```

---

## Running Self-Tests & Stress Suite

`libprotossl` includes automated regression and stress-testing scripts that simulate network packet loss, burst drops, duplicate packet injection, and multi-queue backpressure.

### Prerequisites
Generate test certificates in the `keys/` directory:
```bash
cd keys
./genkey.sh
cd ..
```

### Running the Comprehensive Stress Suite
The automated stress suite executes 8 distinct scenarios (clean baseline, uniform packet loss, latency jitter, burst loss, tiny fragment splitting, duplicate injection, and multi-queue backpressure flooding) with automated CRC and sequence verification:
```bash
./run_stress_suite.sh
```

To run a specific scenario by ID (1 through 8):
```bash
./obj/dtlsq_stress_test 3
```

To list available test scenarios:
```bash
./obj/dtlsq_stress_test --list
```
