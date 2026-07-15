..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

******************
Terminal Chronicle
******************

``terminal-chronicle`` tells a short story using only ``Terminal::print()`` and ``Terminal::printLine()``.
It is the most direct example in the repository of colorful, structured terminal output without a back buffer or
full-screen render loop.

Use This Demo When You Need...
==============================

* A readable example of mixed text, colors, and formatting in ordinary command-line output.
* A support reference for status messages, splash screens, or narrated console output.
* A compact alternative to the fullscreen demos when you want simple print calls only.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/cterm/terminal-chronicle

The program prints the full story directly into the terminal history and exits on its own.

Captured Output (80x25)
=======================

.. include:: _captures/terminal-chronicle.rstinc

Features Demonstrated
=====================

* Direct output via ``Terminal::print()`` and ``Terminal::printLine()``.
* Readable mixed argument lists combining plain text, colors, and formatting state.
* Inline foreground and background changes inside one output flow.
* Structured command-line output without building or managing a ``Buffer`` first.

Related Demos
=============

* :doc:`Display All Attributes <display-all-attributes>` for more styled line-oriented output.
* :doc:`Display All Colors <display-all-colors>` for a palette-focused reference of the built-in ANSI colors.
* :doc:`Text Gallery <text-gallery>` for the next step up into full-screen retained-buffer rendering.

Relevant Source Files
=====================

If you want to explore the implementation, start with :file:`demos/cterm/terminal-chronicle/src/StoryDemo.cpp`.

This file contains the complete sequence and shows how expressive terminal output can stay compact and readable.
