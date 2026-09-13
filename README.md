# Jaguar Programming Language

Jaguar is a classical, extremely fast native compiler with a lightweight runtime and live-reload execution mode.

## Quick Start

### Build Compiler
```bash
cmake -S . -B build
cmake --build build
```

### Run Examples
```bash
./build/jag examples/hello.jag
./build/jag -live=1 examples/live.jag
```

### Run Tests
```bash
ctest --test-dir build --output-on-failure
```
