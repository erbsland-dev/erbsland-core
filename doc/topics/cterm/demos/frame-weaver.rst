..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************
Frame Weaver
************

``frame-weaver`` demonstrates how the frame drawing system resolves intersections while multiple frames with varying
sizes, colors, and styles are added over time.
It is the best showcase for mixed line styles, custom frame tiles, and automatic frame combination behavior.

Use This Demo When You Need...
==============================

* A support example for frame intersections and overlapping panels.
* A place to compare predefined ``FrameStyle`` values with custom ``Block16Style`` tiles.
* A full-screen animation that makes frame composition behavior easy to inspect over time.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/cterm/frame-weaver

Use ``F`` and ``S`` to change the pacing, ``C`` to clear the scene, the number keys to switch style groups, and ``Q`` to
quit.

Captured Output (80x25)
=======================

.. include:: _captures/frame-weaver.rstinc

The capture may not show the terminal output correctly.
Here a screenshot:

.. figure:: images/frame-weaver3.jpg
    :width: 100%

Features Demonstrated
=====================

* Predefined ``FrameStyle`` values and a custom ``Block16Style`` frame tile set.
* Automatic frame intersection handling via ``BlockCombinationStyle::commonBoxFrame()``.
* Repeated full-screen redraws with palette-driven color variations.
* Randomized frame placement that stresses mixed styles and crossing borders.

Related Demos
=============

* :doc:`Frame Color Animations <frame-color-animations>` for animated color sequences on cleaner, fixed panels.
* :doc:`Grid Layout <grid-layout>` for a structured frame-drawing example.
* :doc:`Bitmap Showcase <bitmap-showcase>` for another example of ``Block16Style``-based rendering behavior.

Relevant Source Files
=====================

If you want to explore the implementation, start with :file:`demos/cterm/frame-weaver/src/FrameWeaverApp.cpp`.

This file contains the render loop, random frame placement, and the custom style definition used by the demo.
