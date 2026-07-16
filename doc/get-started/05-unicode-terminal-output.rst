..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Getting started; Terminal output
    single: Unicode; Safe display
    single: Match highlighting

***********************
Safe Unicode and Colors
***********************

Print the File Summary
======================

Every visited file gets a summary, including files without matches.
The terminal print interface accepts text and style changes in one call.

.. literalinclude:: files/elgrep/src/ElGrepApp.cpp
    :language: cpp
    :caption: ElGrepApp.cpp — file summaries
    :start-at: void ElGrepApp::printResult(
    :end-before: void ElGrepApp::printMatchingLine(

Highlight Every Match
=====================

Each matching line is printed once.
The stored byte ranges divide it into unmatched and matched slices, and the matched slices use a bright, bold style.
An empty regular-expression match has no text to color, so ``elgrep`` inserts a narrow marker at its position.

.. literalinclude:: files/elgrep/src/ElGrepApp.cpp
    :language: cpp
    :caption: ElGrepApp.cpp — highlighted matching lines
    :start-at: void ElGrepApp::printMatchingLine(
    :end-before: void ElGrepApp::printText(

Before writing any source text, ``printText()`` applies the display escaping format.
Invisible and control characters become readable escape sequences instead of changing the terminal state.

.. literalinclude:: files/elgrep/src/ElGrepApp.cpp
    :language: cpp
    :caption: ElGrepApp.cpp — display-safe text
    :start-at: void ElGrepApp::printText(
    :end-before: }

The terminal representation also preserves valid Unicode characters and combining sequences.
Malformed UTF-8 is decoded using deterministic replacement characters.

Interactive and Redirected Output
=================================

The program entry point enables terminal support before it calls ``run()``.

.. literalinclude:: files/elgrep/src/main.cpp
    :language: cpp
    :caption: <project>/elgrep/src/main.cpp
    :linenos:

On an interactive terminal, Core renders the selected colors.
For redirected output or a platform without terminal control, the entry point selects the terminal's plain block-text
mode.
It omits ANSI control sequences while the search and output code remain unchanged.

.. button-ref:: 06-build-and-run
    :ref-type: doc
    :color: success
    :align: center
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    Build and Run elgrep →
