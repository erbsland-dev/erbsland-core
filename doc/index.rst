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

    Erbsland Core is in **early alpha**.
    The available parts are useful for experiments and new applications, but the public API can change without a
    compatibility period and several planned subsystems are incomplete.
    Pin the Git revision you use and expect to update application code when moving to a newer revision.

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
        :link: requirements
        :link-type: doc

        Check compiler, CMake, Git, platform, and contributor-tool requirements.

What Is Available Today
=======================

The implemented foundation already covers:

*   Extensive and reliable Unicode-aware strings, text and formatting (UTF-8/16/32).
*   Safe and reliable regular expression engine.
*   Filesystem paths, path infos, file and directory operations, and file streams
*   Full features command-line option parsing including formatted help and version output.
*   Application framework to minimize boilerplate.
*   Terminal output with colors, styles, formatting, cursor movement, terminal size detection, and more.
*   Reliable date and time types, durations, system-independent time-zone calculation.
*   Safe numeric utilities, saturating math/integers, safe numeric conversions, and more.
*   Random generators, APIs to use fast or secure random generators safely.
*   Event primitives: Event loops, scheduler, timer, function invocation, event threads.
*   Stream framework: Byte and text streams, text encodings, buffers, and more.
*   Error handling: Predefined error classes, error diagnostic, formatted diagnostic error output.
*   Utilities: COW containers, co-routine primitives, enum flags, ...

The repository README lists the major areas that are still missing from the alpha.

All Documentation
=================

.. toctree::
    :maxdepth: 3

    get-started/index
    usage/index
    topics/index
    reference/index
    requirements
    addendum/changelog
    guidelines/index

The :doc:`guidelines/index` section is contributor material for developing Core itself.

Documentation Indices
=====================

*   :ref:`genindex`
*   :ref:`search`
