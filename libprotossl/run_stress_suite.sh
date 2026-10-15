#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

# Ensure binaries are built
if [ ! -x "./obj/dtlsq_stress_test" ]; then
    echo "Building dtlsq_stress_test..."
    make -j4
fi

# Ensure test certificates exist and are valid
if [ ! -f "keys/Root-CA.crt" ]; then
    mkdir -p keys
    (cd keys && ./genkey.sh)
fi

echo "========================================================"
echo "         Running Automated Stress Test Suite            "
echo "========================================================"

./obj/dtlsq_stress_test "$@"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "========================================================"
    echo "          ALL STRESS TEST SCENARIOS PASSED!             "
    echo "========================================================"
else
    echo "========================================================"
    echo "         SOME STRESS TEST SCENARIOS FAILED!             "
    echo "========================================================"
fi

exit $EXIT_CODE
