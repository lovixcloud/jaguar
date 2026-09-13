# Jaguar Installation Guide

## Build Requirements
- C17 compatible compiler (GCC, Clang, or MSVC)
- CMake 3.15+
- Make, Ninja, or Visual Studio 2019/2022

## Build and Install

### Linux / macOS
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
```

### Windows (Visual Studio / MSBuild)
```cmd
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
cmake --install build --config Debug
```
