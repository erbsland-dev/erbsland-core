..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************
Text Gallery
************

``text-gallery`` demonstrates the text rendering side of the library in one interactive full-screen page set.
It combines framed panels, wrapped paragraphs, mixed-width Unicode text, animated bitmap-font titles, and compact footer
prompts.

Use This Demo When You Need...
==============================

* A support example for alignment, wrapping, and mixed-width Unicode text inside framed panels.
* A practical reference for bitmap-font text via ``Font::defaultAscii()``.
* A single demo that shows several text-rendering techniques working together in one retained buffer.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/cterm/text-gallery

Use the left and right arrow keys to switch pages.
Press ``Q`` to quit.

Captured Output (80x25)
=======================

.. include:: _captures/text-gallery.rstinc

The capture may not show the terminal output correctly.
Here a screenshot:

.. figure:: images/text-gallery3.jpg
    :width: 100%

Features Demonstrated
=====================

* BlockText layout using ``BlockText`` together with multiple ``Alignment`` modes.
* Unicode-aware wrapping and centering of mixed-width characters.
* Bitmap-font rendering using ``Font::defaultAscii()``.
* Animated title coloring with ``ColorSequence`` and ``BlockTextAnimation``.
* Colored footer prompts assembled from ``BlockString`` and ``Block`` elements.

Related Demos
=============

* :doc:`Display All Attributes <display-all-attributes>` for line-oriented output with styled terminal text.
* :doc:`Grid Layout <grid-layout>` for centered content inside framed grid cells.
* :doc:`Bitmap Showcase <bitmap-showcase>` for a companion demo centered on bitmap-based rendering instead of text.

Relevant Source Files
=====================

If you want to explore the implementation, start with :file:`demos/cterm/TextGallery/src/TextGalleryApp.cpp`.

This file contains the complete layout logic for the panels, titles, and footer prompts shown in the demo.
