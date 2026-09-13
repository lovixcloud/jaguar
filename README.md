# Jaguar Programming Language

Jaguar is a classical, extremely fast native compiler with a lightweight runtime and live-reload execution mode.

## Quick Start

### Build Compiler
```bash
cmake -S . -B build
cmake --build build
```

### Run Examples
Linux / macOS:
```bash
./build/jag examples/hello.jag
./build/jag -live=1 examples/live.jag
```

Windows (Visual Studio / MSBuild):
```cmd
.\build\Debug\jag.exe examples\hello.jag
.\build\Debug\jag.exe -live=1 examples\live.jag
```

### Run Tests
Linux / macOS (Single-config generators):
```bash
ctest --test-dir build --output-on-failure
```

Windows (Multi-config generators like Visual Studio):
```cmd
ctest --test-dir build -C Debug --output-on-failure
```
