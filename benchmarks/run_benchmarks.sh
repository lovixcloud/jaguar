#!/bin/bash
set -e

echo "=== Jaguar Compiler Benchmarks ==="
TIMEFORMAT="Time: %R seconds"

echo "1. Cold Compilation Time:"
time ./build/jag build examples/canonical.jag > /dev/null

echo "2. Native Binary Execution Time:"
time ./examples/canonical > /dev/null

echo "3. Type Checking Speed:"
time ./build/jag check examples/canonical.jag > /dev/null

echo "=== Benchmarks Complete ==="
