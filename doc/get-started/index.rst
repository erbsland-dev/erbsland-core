..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Getting started
    single: elgrep

***************
Getting Started
***************

.. toctree::
    :maxdepth: 1
    :hidden:

    01-project-structure
    02-cmake-configuration
    03-application-framework
    04-searching-files
    05-unicode-terminal-output
    06-build-and-run

.. rst-class:: sd-fs-4

    Build a Unicode-Safe Recursive Search Tool

In this tutorial you will build ``elgrep``, a small command-line tool that searches files with an Erbsland regular
expression.
It accepts a file or a directory, walks directories recursively when requested, and highlights every match using safe
terminal output.

.. code-block:: console

    $ elgrep [--recursive] <path> <pattern>

The example is deliberately practical.
It combines several parts of Erbsland Core in one compact application:

*   :cpp:class:`Application <erbsland::core::Application>` manages the executable lifecycle, options, help, and errors.
*   :cpp:class:`Path <erbsland::path::Path>` and :cpp:class:`PathWalker <erbsland::path::PathWalker>` provide portable
    filesystem access.
*   :cpp:class:`RegEx <erbsland::re::RegEx>` searches Unicode text and reports byte-safe match ranges.
*   :cpp:class:`TextInputStream <erbsland::stream::TextInputStream>` reads one decoded line at a time.
*   :cpp:class:`Terminal <erbsland::cterm::Terminal>` renders colored interactive output and plain redirected output
    through the same interface.

The tutorial starts with an empty Git repository and finishes with a useful C++20 application.
You only need a compiler, CMake, Git, and an editor or IDE.

.. button-ref:: 01-project-structure
    :ref-type: doc
    :color: success
    :align: center
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    Create the Project Structure →

.. card:: What This Tutorial Is Not

    ``elgrep`` is a compact teaching application, not a replacement for a mature grep implementation.
    It treats regular files as UTF-8 text, searches each line independently, and returns success when the search
    completes even when no matches are found.
