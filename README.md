# Erbsland Core

**Erbsland Core** is a modern C++ foundation library that provides a consistent, secure, and platform-independent API for application development.

It offers native support for **Linux**, **macOS**, and **Windows**, allowing you to write portable code without platform-specific boilerplate.

## Highlights

- Cross-platform abstraction layer for system and application code
- Modern UTF-aware text processing
- Secure and reliable low-level utilities
- Consistent APIs across all supported platforms
- Designed for clean architecture and rapid development
- Security-focused by design

# Project Status

Erbsland Core is currently under active development (~40% done).

The project consolidates and modernizes several internal proprietary libraries into a unified open-source ecosystem. Large parts of the codebase are already production-proven and have been used internally for years.

While the library is already usable for real-world projects, the public API is still evolving toward a stable **1.0 release**. Until then, expect occasional breaking changes and minor migration adjustments between versions.

# Design Goals

The library focuses on providing a dependable base layer between applications and the underlying operating system.

Core principles include:

- **Security before performance**
- Cross-platform consistency
- Minimal boilerplate
- Predictable and explicit APIs
- Modern C++20 design
- Strong Unicode and type safety support

# Feature Overview

The following list describes the planned scope for the core library version **1.0**.

| Status | Meaning |
|---|---|
| ✅ | Implemented |
| 🟠 | Partially implemented |
| ❌ | Planned / not yet implemented |

## Scope

The core library provides the minimal infrastructure required to build portable command-line tools and applications. It includes foundational building blocks such as memory management, Unicode-aware text processing, timing utilities, and event handling.

Higher-level extension libraries will provide additional functionality beyond this foundation layer.

# Features

## Memory Management

- Copy-on-write memory model for efficient and thread-safe data sharing ✅
- Byte containers and integer encoding/decoding utilities ✅
- Bit manipulation and bit-mask utilities ✅

## Text Processing

- Full UTF-8, UTF-16, and UTF-32 string support ✅
- Extensive string manipulation API ✅
- Case-insensitive comparison and matching ✅
- Byte-level and code-point-level indexing ✅
- Transparent conversion between UTF encodings and standard library types ✅
- Optional strict UTF validation ✅
- Error-tolerant UTF decoding using replacement characters ✅
- Zero-copy UTF string literals in read-only memory ✅
- Embedded minimal Unicode database:
    - lower/upper case conversion ✅
    - single code-point case folding ✅
- Format string parsing and formatting ✅

## Command Line Parsing

- Structured command-line parsing ✅
- Argument validation ✅
- Action-based command systems ✅

## Configuration Files

- Minimal implementation of the Erbsland Configuration Language (ELCL) ❌
- Structured configuration parsing ❌

## Date and Time

- Platform-independent date/time algorithms ✅
- Safe 64-bit date, time, duration, and date-time types ✅
- Time-zone calculations and conversions ✅
- Elapsed time calculations and conversions ✅

## Units

- Strongly typed integer units for indexes, offsets, and lengths ✅

## Mathematics

- Saturating integer types and operations ✅
- Safe mixed-integer arithmetic ✅
- Clamped integer arithmetic ✅
- Random generator utilities ✅

## Event Handling

- Event system for single-threaded and multithreaded applications ✅

## Filesystem

- Filesystem path abstraction ❌
- UTF-aware text file handling ❌
- Binary file utilities ❌
- Recursive directory traversal ❌
- Temporary file and directory management ❌

## File I/O

- UTF-aware text streams ❌
- Binary stream encoding/decoding ❌

## Application Framework

- Unified application singleton ✅
- Component startup/shutdown framework ❌

## Logging

- Configurable logging framework ❌

## Networking

- Network address and port utilities ❌
- Event-driven socket communication ❌

## Cryptography

- SHA algorithms ❌
- AES algorithms ❌

## Utilities

- Version and version-range handling ✅
- Hash calculation helpers ✅

## Error Handling

- Domain-specific exception hierarchy ✅

## Event System

- Event loop and event handling framework ❌
- Event-driven asynchronous operations ❌

## Regular Expressions

- Erbsland Regular expressions ❌

# Documentation

Documentation is continuously updated alongside the library.

Latest documentation:

👉 https://core.erbsland.dev/

# How to Use the Library

The recommended integration method is using a **Git submodule** together with **CMake**.

## 1. Add as a Submodule

```console
cd <project-root>

git init
mkdir erbsland

git submodule add https://github.com/erbsland-dev/erbsland-core.git erbsland/core
```

## 2. Include in CMake

Add the library to your top-level `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28)

project(ExampleProject LANGUAGES CXX)

add_subdirectory(erbsland/core EXCLUDE_FROM_ALL)

add_executable(example src/main.cpp)

target_link_libraries(example PRIVATE erbsland::core)
```

## 3. Building

Configure and build the project using CMake:

```console
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

# Running Unit Tests

The project includes standalone unit tests.

```console
cd <src dir>
git clone https://github.com/erbsland-dev/erbsland-core.git
cd erbsland-core
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
```

# Requirements

## Using the Library

- C++20 compatible compiler
- CMake 3.28 or newer

## Development

- Python 3.14 or newer  
  Used for tooling, helper scripts, and metadata generation.

# License

Copyright © 2026 Tobias Erbsland 
https://erbsland.dev

Licensed under the **Apache License, Version 2.0**.
