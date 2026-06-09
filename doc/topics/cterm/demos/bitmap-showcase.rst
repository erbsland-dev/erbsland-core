..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

****************
Bitmap Showcase
****************

``bitmap-showcase`` demonstrates how bitmap rendering behaves when a ``Bitmap`` is passed to ``Buffer::drawBitmap()``.
It uses a selector panel on the left and a large preview area on the right so different scale modes, color modes,
layout options, and style replacements stay easy to compare.

Use This Demo When You Need...
==============================

* A support reference for ``drawBitmap()`` and its rendering options.
* A practical way to compare scale modes, color modes, alignment, cropping, and style replacements.
* A place to study how bitmap rendering interacts with existing frame grids and ``Block16Style`` output.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/bitmap-showcase

Use the left and right arrow keys to switch pages. Use the up and down arrow keys to move through the variants on the
current page. Press ``Q`` to quit.

Captured Output (80x25)
=======================

.. include:: _captures/bitmap-showcase.rstinc

Features Demonstrated
=====================

* ``Buffer::drawBitmap()`` across ``HalfBlock``, ``FullBlock``, ``DoubleBlock``, and ``Block16Style`` output modes.
* Animated ``BitmapColorMode`` variants with ``ColorSequence``.
* Drawing a bitmap into a rectangle with alignment-dependent cropping.
* Custom ``fullBlock()``, ``doubleBlocks()``, and ``halfBlocks()`` replacements.
* ``BlockCombinationStyle`` when a line-based bitmap is drawn over an existing frame grid.

Related Demos
=============

* :doc:`Text Gallery <text-gallery>` for the font and text-oriented side of terminal rendering.
* :doc:`Frame Weaver <frame-weaver>` for more ``Block16Style`` and combination-style behavior.
* :doc:`Frame Color Animations <frame-color-animations>` for another showcase built around visual comparisons in one screen.

Relevant Source Files
=====================

If you want to explore the implementation, start with :file:`demos/cterm/bitmap-showcase/src/BitmapShowcaseApp.cpp`.

This file contains the page layout, sample bitmaps, and the rendering option combinations used by the demo.
