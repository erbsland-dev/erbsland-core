..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Getting started; PathWalker
    single: Getting started; Regular expressions
    single: Recursive search

***************
Searching Files
***************

Read One Unicode Line at a Time
===============================

The file-search method opens a decoded text stream and reads one logical line at a time.
It normalizes malformed units through Core's deterministic replacement handling, then removes only trailing CR and LF
characters.
All other whitespace is preserved for matching and display.

.. literalinclude:: files/elgrep/src/ElGrepApp.cpp
    :language: cpp
    :caption: ElGrepApp.cpp — search one file
    :start-at: auto ElGrepApp::searchFile(
    :end-before: void ElGrepApp::searchDirectory(

The regular-expression engine reports UTF-8 byte ranges.
These ranges can safely slice the same read-only :cpp:type:`String <erbsland::text::String>` without splitting a Unicode
code point.
Only the ranges and matching lines are retained; unmatched lines are released immediately.

``collectAll()`` returns non-overlapping matches in input order.
Applying it to one line at a time gives ``elgrep`` conventional line-oriented behavior and keeps memory use bounded by
the matching output rather than the complete input file.

Walk a Directory Deterministically
==================================

When ``--recursive`` is present, :cpp:class:`PathWalker <erbsland::path::PathWalker>` visits regular files in a stable
order.
The configuration skips symbolic links so the tutorial cannot accidentally leave the selected tree or enter a cycle.

.. literalinclude:: files/elgrep/src/ElGrepApp.cpp
    :language: cpp
    :caption: ElGrepApp.cpp — recursive traversal
    :start-at: void ElGrepApp::searchDirectory(
    :end-before: void ElGrepApp::printResult(

Each displayed path is relative to the selected directory.
A filesystem error from ``walkOrThrow()`` propagates to ``Application::run()`` just like an error from reading a file.

.. note::

    This tutorial treats every regular file as UTF-8 text and stops at the first read or traversal error.
    A production search tool could add binary-file detection, ignore rules, and a policy for continuing after selected
    errors.

.. button-ref:: 05-unicode-terminal-output
    :ref-type: doc
    :color: success
    :align: center
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    Render Safe Unicode Output →
