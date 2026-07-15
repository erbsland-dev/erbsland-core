..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**********************
Display All Attributes
**********************

``display-all-attributes`` gives you a compact visual overview of the ANSI character attributes supported by the
library.
It prints one row per attribute together with backend support information, sample text, and a few practical
combinations.

Use This Demo When You Need...
==============================

* A fast support checklist for attributes like italic, blink, hidden text, or strikethrough.
* A real-terminal comparison between what the backend can emit and what the emulator actually renders.
* A small reference for applying ``BlockAttributes`` directly in line-oriented output.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/cterm/display-all-attributes

The demo prints the table and waits for Enter in an interactive terminal.

Captured Output (80x25)
=======================

.. include:: _captures/display-all-attributes.rstinc

Features Demonstrated
=====================

* Individual ``BlockAttributes`` flags such as bold, dim, italic, underline, blink, reverse, hidden, and strikethrough.
* Backend-reported support checks via ``Terminal::supportedBlockAttributes()``.
* Side-by-side display of ANSI SGR enable and disable codes.
* Practical combination rows that make reset behavior easier to inspect.

Related Demos
=============

* :doc:`Display All Colors <display-all-colors>` for the complementary palette reference.
* :doc:`Terminal Chronicle <terminal-chronicle>` for another line-oriented output demo.
* :doc:`Frame Color Animations <frame-color-animations>` for a color-sequence based animation example.

Relevant Source Files
=====================

If you want to explore the implementation, start with
:file:`demos/cterm/display-all-attributes/src/DisplayAllAttributesApp.cpp`.

This file contains the attribute table generation and the direct use of ``BlockAttributes`` with terminal print calls.
