..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Getting started; Build
    single: elgrep; Running

********************
Build and Run elgrep
********************

Build the Project
=================

Configure an out-of-source debug build with Ninja, then build the application target:

.. code-block:: console

    $ cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
    $ cmake --build cmake-build-debug --target elgrep

The executable is located below the application's build directory:

.. code-block:: console

    $ ./cmake-build-debug/elgrep/elgrep --help
    Searches a UTF-8 text file, or recursively searches all regular files below a directory.
    Usage:
    elgrep [options] <path> <pattern>
    Options:
    -h, --help       Display this help.
    <path>           File or directory to search.
    <pattern>        Erbsland Core regular-expression pattern.
    -r, --recursive  Recursively searches a directory and skips symbolic links.
        --version    Display version information.

Try a Single File
=================

Create ``observations.txt`` with multilingual Unicode text:

.. code-block:: text

    Morgen: Eisvogel am See.
    Matin : martin-pêcheur près du lac.
    Mañana: martín pescador junto al lago.

Search case-insensitively for ``ma``:

.. code-block:: console

    $ ./cmake-build-debug/elgrep/elgrep observations.txt "(?i)ma"
    observations.txt: 3 matches
        2: Matin : martin-pêcheur près du lac.
        3: Mañana: martín pescador junto al lago.

On an interactive terminal the path, count, line numbers, and matched words use distinct colors.
Both matches on line 2 are highlighted, but the line is printed only once.
The text remains readable when redirected:

.. code-block:: console

    $ ./cmake-build-debug/elgrep/elgrep observations.txt "orca" > result.txt
    $ cat result.txt
    observations.txt: 0 matches

Zero-width expressions use a visible marker so their positions do not disappear:

.. code-block:: console

    $ ./cmake-build-debug/elgrep/elgrep one-line.txt "^|$"
    one-line.txt: 2 matches
        1: ▏Unicode text▏

Search a Directory
==================

Use ``--recursive`` or ``-r`` when the path is a directory:

.. code-block:: console

    $ ./cmake-build-debug/elgrep/elgrep --recursive notes "lake|lac|lago"
    day-1.txt: 2 matches
        4: The lake was still before sunrise.
        7: Deux hérons ont traversé le lac.
    archive/day-2.txt: 0 matches

The summary appears for every regular file in deterministic path order.
Matching lines appear once even if a line contains several matches, and every non-overlapping match is highlighted.

Explore Error Handling
======================

Try omitting an argument, passing a directory without ``--recursive``, or using an invalid expression:

.. code-block:: console

    $ ./cmake-build-debug/elgrep/elgrep observations.txt "("
    Error: Failed to parse regular expression
      Unclosed group at the end of the pattern

The exception is reported by the application framework with a consistent diagnostic; no local exception handler is
needed in ``elgrep``.

What to Read Next
=================

*   :doc:`/usage/integrate-as-submodule` explains the recommended project layout in more detail.
*   :doc:`/topics/options/index` covers richer command-line interfaces.
*   :doc:`/topics/path/index` explains portable paths and filesystem operations.
*   :doc:`/topics/re/index` describes the regular-expression syntax and engine.
*   :doc:`/topics/stream/index` covers text, byte, file, and coroutine streams.
*   :doc:`/topics/cterm/index` introduces terminal rendering and input.

.. button-ref:: /usage/index
    :ref-type: doc
    :color: info
    :align: center
    :outline:
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    Continue with the Usage Guide →
