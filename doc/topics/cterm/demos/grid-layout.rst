..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********
Grid Layout
***********

``grid-layout`` demonstrates how :cpp:any:`FrameBorder <erbsland::cterm::FrameBorder>` and
:cpp:any:`GridLayout <erbsland::cterm::GridLayout>` work together. You can change each border element independently
and resize the logical grid while the layout keeps every content cell aligned.

Use This Demo When You Need...
==============================

* A compact example for drawing table-like grids into a ``Buffer``.
* A visual check for mixed light, dashed, heavy, double, rounded, and ``None`` border elements.
* A reference for using ``GridLayout::cellRect()`` to place content inside grid cells.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/grid-layout

Use keys ``1`` through ``6`` to cycle the top, bottom, left, right, horizontal separator, and vertical separator
elements through the supported grid line styles. Use ``C`` to cycle between one and five columns, ``R`` to cycle
between one and three rows, and ``Q`` to quit.

Captured Output (80x25)
=======================

.. include:: _captures/grid-layout.rstinc

Features Demonstrated
=====================

* Uniform ``FrameBorder`` setup with bright white grid lines.
* Per-element border style changes without rebuilding the layout code.
* Responsive ``GridLayout`` creation from the available terminal area.
* Cell content placement with ``GridLayout::cellRect()``.

Related Demos
=============

* :doc:`Frame Weaver <frame-weaver>` for overlapping frame intersections and custom frame styles.
* :doc:`Frame Color Animations <frame-color-animations>` for animated frame color sequences.
* :doc:`Bitmap Showcase <bitmap-showcase>` for another framed layout with a preview area.

Relevant Source Files
=====================

If you want to explore the implementation, start with :file:`demos/cterm/grid-layout/src/GridLayoutApp.cpp`.

This file contains the border state, grid sizing logic, and the render loop that draws the cells and grid lines.
