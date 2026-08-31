..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***************************
Erbsland Core Documentation
***************************

**Erbsland Core** is a cross-platform C++20 foundation library for building secure, portable applications with little
boilerplate.
It provides a consistent API for Linux, macOS, and Windows and depends only on the C++ standard library.

.. warning::

    Erbsland Core is in **alpha**.
    The available status is already useful for experiments and new applications, but the public API can change without a
    compatibility period. See :doc:`addendum/roadmap` for details.

.. button-ref:: get-started/index
    :ref-type: doc
    :color: success
    :align: center
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    Build ``elgrep`` — Start the Tutorial →

Choose Your Starting Point
==========================

.. grid:: 1 2 2 3
    :gutter: 2

    .. grid-item-card:: :fas:`rocket;sd-text-success` Getting Started
        :link: get-started/index
        :link-type: doc

        Build a practical recursive search tool using the application framework, paths, streams, regular expressions,
        and safe terminal output.

    .. grid-item-card:: :fas:`book-open;sd-text-success` Usage
        :link: usage/index
        :link-type: doc

        Integrate Core as a Git submodule, configure its build, or consume an installed static library.

    .. grid-item-card:: :fas:`shapes;sd-text-success` Feature Topics
        :link: topics/index
        :link-type: doc

        Learn the concepts and patterns behind individual library areas.

    .. grid-item-card:: :fas:`code;sd-text-success` API Reference
        :link: reference/index
        :link-type: doc

        Look up public types, functions, and detailed behavior by API area.

    .. grid-item-card:: :fas:`list-check;sd-text-success` Requirements
        :link: addendum/requirements
        :link-type: doc

        Check compiler, CMake, Git, platform, and contributor-tool requirements.

Features Available Today
========================

Text, data, and utilities
--------------------------

- Unicode-aware strings, text, and formatting for UTF-8, UTF-16, and UTF-32
- A safe, reliable regular-expression engine
- Byte and text streams, text encodings, and buffers
- Filesystem paths, file and directory operations, and file streams
- Date and time types, durations, and system-independent time-zone calculation
- Safe numeric utilities, including saturating arithmetic and checked conversions
- Fast and secure random-number generators with safe APIs
- Copy-on-write containers, coroutine primitives, enum flags, compression, and more
- Template render framework with a Jinja like syntax.

Applications and system integration
-----------------------------------

- Command-line option parsing with formatted help and version output
- An application framework and application-part management to minimize boilerplate
- A resource system for automatically compiled-in resources
- Terminal output with colors, styles, cursor movement, and terminal-size detection
- Error classes, diagnostics, and formatted diagnostic output
- The Erbsland Configuration Language, including validation rules
- Bounded asynchronous logging with hierarchical streams, runtime reconfiguration, and statistics

Events and networking
---------------------

- Event loops, schedulers, timers, function invocation, and event threads
- Event-based TCP and UDP connections, host-name resolution, and more
- A TLS 1.3 network layer over TCP
- An event-based HTTP server and client framework for plain TCP and TLS

Cryptography
------------

- An extensive cryptography layer with no dependency on other libraries

All Documentation
=================

.. toctree::
    :maxdepth: 3

    get-started/index
    usage/index
    topics/index
    reference/index
    addendum/background
    addendum/roadmap
    addendum/requirements
    addendum/changelog
    guidelines/index

The :doc:`guidelines/index` section is contributor material for developing Core itself.

Documentation Indices
=====================

*   :ref:`genindex`
*   :ref:`search`
