# Jaguar Installation Guide

## Build Requirements
- C17 compatible compiler (GCC or Clang)
- CMake 3.15+
- Make / Ninja

## Build and Install
```bash
cmake -S . -B build
cmake --build build
sudo cmake --install build
```
