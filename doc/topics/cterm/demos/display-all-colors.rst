..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

******************
Display All Colors
******************

``display-all-colors`` is the quickest overview of the built-in 16 terminal colors and the combinations they can form.
It prints a named foreground/background table, a full 16x16 matrix, and a rainbow block pattern that makes contrast
differences easy to compare.

Use This Demo When You Need...
==============================

* A support reference for the built-in ANSI 16-color palette.
* A visual way to compare foreground/background combinations before choosing terminal colors for an application.
* A compact example of palette-oriented output without a redraw loop.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/cterm/display-all-colors

The program writes the full palette reference into terminal history and exits on its own.

Captured Output (80x25)
=======================

.. include:: _captures/display-all-colors.rstinc

Features Demonstrated
=====================

* ``Foreground::fromIndex16()`` and ``Background::fromIndex16()`` across the full ANSI palette.
* Named color output via ``toString()``.
* Dense matrix rendering with ``Buffer::fill()`` and ``Buffer::drawBlockText()``.
* Practical contrast checks using the rainbow block pattern.

Related Demos
=============

* :doc:`Display All Attributes <display-all-attributes>` for the matching attribute-focused reference.
* :doc:`Frame Color Animations <frame-color-animations>` for color sequences applied to animated frames.
* :doc:`Retro Plasma <retro-plasma>` for palette-driven animation in a full-screen demo.

Relevant Source Files
=====================

If you want to explore the implementation, start with
:file:`demos/cterm/display-all-colors/src/DisplayAllColorsApp.cpp`.

This file contains the palette table, matrix, and rainbow pattern rendering logic.
