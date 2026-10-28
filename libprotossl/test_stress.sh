#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

NUM_MSGS="${1:-100}"
DROP_PERCENT="${2:-10}"
LATENCY_TICKS="${3:-1}"

SERVER_PORT=19482
PROXY_PORT=19483

echo "=== ProtoSslDtlsQueue Automated Stress Test ==="
echo "Messages per queue: $NUM_MSGS"
echo "Packet drop rate:   $DROP_PERCENT%"
echo "Latency (ticks):    $LATENCY_TICKS"
echo "Server port:        $SERVER_PORT"
echo "Proxy port:         $PROXY_PORT"
echo "==============================================="

# Ensure binaries are built
if [ ! -x "./obj/protossl_test" ] || [ ! -x "./obj/udp_proxy" ]; then
    echo "Building targets..."
    make -j4
fi

# Ensure valid test keys exist
if [ ! -f "keys/Root-CA.crt" ] || [ ! -f "keys/Server-Cert.crt" ] || [ ! -f "keys/Client-Cert.crt" ]; then
    echo "Generating test certificates..."
    mkdir -p keys
    (cd keys && ./genkey.sh)
fi

SERVER_LOG=$(mktemp /tmp/dtlsq_server_XXXXXX.log)
PROXY_LOG=$(mktemp /tmp/dtlsq_proxy_XXXXXX.log)
CLIENT_LOG=$(mktemp /tmp/dtlsq_client_XXXXXX.log)

cleanup() {
    if [ -n "$SERVER_PID" ]; then kill -9 "$SERVER_PID" 2>/dev/null || true; fi
    if [ -n "$PROXY_PID" ]; then
        kill "$PROXY_PID" 2>/dev/null || true
        wait "$PROXY_PID" 2>/dev/null || true
    fi
    rm -f "$SERVER_LOG" "$PROXY_LOG" "$CLIENT_LOG"
}
trap cleanup EXIT

echo "Starting server on port $SERVER_PORT..."
./obj/protossl_test sU $SERVER_PORT --stress $NUM_MSGS > "$SERVER_LOG" 2>&1 &
SERVER_PID=$!
sleep 0.3

TARGET_PORT=$SERVER_PORT
if [ "$DROP_PERCENT" -gt 0 ] || [ "$LATENCY_TICKS" -gt 0 ]; then
    echo "Starting udp_proxy on port $PROXY_PORT (${DROP_PERCENT}% loss, ${LATENCY_TICKS} ticks latency)..."
    ./obj/udp_proxy $PROXY_PORT 127.0.0.1 $SERVER_PORT $DROP_PERCENT $LATENCY_TICKS > "$PROXY_LOG" 2>&1 &
    PROXY_PID=$!
    TARGET_PORT=$PROXY_PORT
    sleep 0.3
fi

echo "Starting client connecting to port $TARGET_PORT..."
set +e
timeout 60 ./obj/protossl_test cU 127.0.0.1 $TARGET_PORT --stress $NUM_MSGS > "$CLIENT_LOG" 2>&1
CLIENT_RET=$?

wait $SERVER_PID
SERVER_RET=$?
set -e

echo ""
echo "=== Test Results ==="
echo "Client exit code: $CLIENT_RET"
echo "Server exit code: $SERVER_RET"
echo ""

echo "--- Server Summary ---"
grep -E "SERVER:|DTLSQ STATS|bytes_|frags_" "$SERVER_LOG" || cat "$SERVER_LOG"
echo ""
echo "--- Client Summary ---"
grep -E "CLIENT:|DTLSQ STATS|bytes_|frags_" "$CLIENT_LOG" || cat "$CLIENT_LOG"
echo ""

if [ $CLIENT_RET -ne 0 ] || [ $SERVER_RET -ne 0 ]; then
    echo "FAILED: Stress test exited with non-zero status!"
    echo "=== Full Server Log ==="
    cat "$SERVER_LOG"
    echo "=== Full Client Log ==="
    cat "$CLIENT_LOG"
    exit 1
fi

if [ "$DROP_PERCENT" -gt 0 ]; then
    RESENT=$(grep "frags_resent:" "$CLIENT_LOG" | awk '{print $2}')
    if [ -n "$RESENT" ] && [ "$RESENT" -gt 0 ]; then
        echo "VERIFIED: Retransmissions stimulated and recovered ($RESENT fragments resent)."
    else
        echo "WARNING: Expected retransmissions with $DROP_PERCENT% packet drop, but frags_resent was $RESENT"
    fi
fi

echo "SUCCESS: All messages received in-order and verified by CRC."
