# Erbsland Core

Erbsland Core is a cross-platform C++20 foundation library for building secure, portable applications with little
boilerplate. It supports Linux, macOS, and Windows and has no required dependency beyond the C++ standard library.

## Features

### Text, data, and utilities

- Unicode-aware strings, text, and formatting for UTF-8, UTF-16, and UTF-32
- A safe, reliable regular-expression engine
- Byte and text streams, text encodings, and buffers
- Filesystem paths, file and directory operations, and file streams
- Date and time types, durations, and system-independent time-zone calculation
- Safe numeric utilities, including saturating arithmetic and checked conversions
- Fast and secure random-number generators with safe APIs
- Copy-on-write containers, coroutine primitives, enum flags, compression, and more

### Applications and system integration

- Command-line option parsing with formatted help and version output
- An application framework and application-part management to minimize boilerplate
- A resource system for automatically compiled-in resources
- Terminal output with colors, styles, cursor movement, and terminal-size detection
- Error classes, diagnostics, and formatted diagnostic output
- The Erbsland Configuration Language, including validation rules
- Bounded asynchronous logging with hierarchical streams, runtime reconfiguration, and statistics

### Events and networking

- Event loops, schedulers, timers, function invocation, and event threads
- Event-based TCP and UDP connections, host-name resolution, and more
- A TLS 1.3 network layer over TCP
- An event-based HTTP server and client framework for plain TCP and TLS

### Cryptography

- An extensive cryptography layer with no dependency on other libraries

## Alpha Status

Erbsland Core is in alpha state. Its public API may change without a compatibility period.

**Use the cryptography API at your own risk!**

We do our best to review the cryptography implementations while we build this library.
But an independent review will be done earliest when the library reaches maturity.
Therefore, be warned: **do not** use TLS connections for public networks.

## Documentation

- [Documentation home](https://core.erbsland.dev/)
- [Getting started: build `elgrep`](https://core.erbsland.dev/get-started/)
- [Usage and integration](https://core.erbsland.dev/usage/)
- [Feature topics](https://core.erbsland.dev/topics/)
- [API reference](https://core.erbsland.dev/reference/)

The recommended source integration places Core at `<project>/erbsland/core` as a pinned Git submodule and uses
`<project>/erbsland/CMakeLists.txt` as the aggregation point for Core and future Erbsland Core extensions. Configure
application targets with `erbsland_core_setup_application(TARGET <target>)`. The
[source integration guide](https://core.erbsland.dev/usage/integrate-as-submodule.html)
contains the complete layout and commands; an
[installed static library](https://core.erbsland.dev/usage/install-static-library.html) is supported as well.

## Requirements

- C++20 compiler and standard library
- CMake 3.28 or newer
- Git for the recommended submodule integration

Ninja is optional. See the [requirements page](https://core.erbsland.dev/addendum/requirements.html) for supported platforms and
Core-development tooling.

## Development

The [contributor guidelines](https://core.erbsland.dev/guidelines/) cover the API, source, documentation, and platform
rules used to develop Erbsland Core.

## License

Copyright © 2026 Tobias Erbsland — [erbsland.dev](https://erbsland.dev/)

Licensed under the [Apache License, Version 2.0](LICENSE).
