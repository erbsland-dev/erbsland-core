..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**********
Log Viewer
**********

``log-viewer`` demonstrates how ``CursorBuffer`` and ``BufferView`` work together in a live scrollback-style
application. It generates fictive web-server log messages, appends them to a growing history, and lets the user pan
through that history or jump back into follow mode.

Use This Demo When You Need...
==============================

* A support example for ``CursorBuffer`` growth plus viewport-style navigation.
* A realistic reference for tail-like log UIs with follow mode and manual panning.
* A place to study paragraph formatting inside one scrolling text surface.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/log-viewer

Use the arrow keys to pan, ``F`` to jump back to the newest entries, ``+`` or ``-`` to change the message pace, and
``Q`` or ``Esc`` to quit.

Captured Output (80x25)
=======================

.. include:: _captures/log-viewer.rstinc

Features Demonstrated
=====================

* ``CursorBuffer`` with ``ExpandThenShift`` and a stable black-background fill character.
* ``BufferView`` used as a live viewport onto the bottom-left or a manually panned area of that history.
* ``printParagraph()`` with custom indent, wrap markers, and ellipsis handling for terminal-friendly log formatting.
* Independent coloring for timestamps, log levels, and continuation lines inside one scrolling text surface.
* Follow mode versus manual mode in a realistic log-tail workflow.

Related Demos
=============

* :doc:`Key Input Demo <key-input-demo>` for another ``CursorBuffer`` plus ``BufferView`` driven viewport.
* :doc:`Key Input Demo <key-input-demo>` for another example of interactive input combined with a retained buffer.
* :doc:`Text Gallery <text-gallery>` for a more layout-focused retained-buffer demo.

Relevant Source Files
=====================

If you want to explore the implementation, start with :file:`demos/cterm/log-viewer/src/LogViewerApp.cpp`.

This file contains the log generator, the cursor-buffer rendering logic, and the follow versus pan viewport handling.
