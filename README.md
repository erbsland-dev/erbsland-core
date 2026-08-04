# Erbsland Core

Erbsland Core is a cross-platform C++20 foundation library for building secure, portable applications with little
boilerplate. It supports Linux, macOS, and Windows and has no required dependency beyond the C++ standard library.

## Features

- Extensive and reliable Unicode-aware strings, text and formatting (UTF-8/16/32).
- Safe and reliable regular expression engine.
- Filesystem paths, path infos, file and directory operations, and file streams
- Full features command-line option parsing including formatted help and version output.
- Networking with event-based TCP and UDP connection, host name resolution, and more.
- Erbsland Configuration Language implementation, with support for validation rules.
- Application framework to minimize boilerplate.
- Extensive cryptography layer that is fully independent of other libraries.
- Implementation of TLS 1.3 network layer over TCP. 
- Terminal output with colors, styles, formatting, cursor movement, terminal size detection, and more.
- Reliable date and time types, durations, system-independent time-zone calculation.
- Safe numeric utilities, saturating math/integers, safe numeric conversions, and more.
- Random generators, APIs to use fast or secure random generators safely.
- Event primitives: Event loops, scheduler, timer, function invocation, event threads.
- Stream framework: Byte and text streams, text encodings, buffers, and more.
- Error handling: Predefined error classes, error diagnostic, formatted diagnostic error output.
- Utilities: COW containers, co-routine primitives, enum flags, ...

## Alpha Status

Erbsland Core is in alpha state. Its public API may change without a compatibility period, so pin the Git revision
used by your project and expect migrations when updating. Some of the unfinished areas are:

- Logging
- Application component lifecycle

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
`<project>/erbsland/CMakeLists.txt` as the aggregation point for Core and future Erbsland extensions. Link applications
against `erbsland::core`. The [source integration guide](https://core.erbsland.dev/usage/integrate-as-submodule.html)
contains the complete layout and commands; an
[installed static library](https://core.erbsland.dev/usage/install-static-library.html) is supported as well.

## Requirements

- C++20 compiler and standard library
- CMake 3.28 or newer
- Git for the recommended submodule integration

Ninja is optional. See the [requirements page](https://core.erbsland.dev/requirements.html) for supported platforms and
Core-development tooling.

## Development

The [contributor guidelines](https://core.erbsland.dev/guidelines/) cover the API, source, documentation, and platform
rules used to develop Erbsland Core.

## License

Copyright © 2026 Tobias Erbsland — [erbsland.dev](https://erbsland.dev/)

Licensed under the [Apache License, Version 2.0](LICENSE).
