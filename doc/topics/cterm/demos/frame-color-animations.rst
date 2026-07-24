..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**********************
Frame Color Animations
**********************

``frame-color-animations`` demonstrates the animated color modes of ``FrameDrawOptions``.
Several frame styles are rendered side by side so you can compare one-color, striped, diagonal, and chasing-border
animation modes in one screen.

Use This Demo When You Need...
==============================

* A support example for animated frame borders or panel fills.
* A quick comparison of the available ``FrameColorMode`` behaviors.
* A reference for combining one retained ``Buffer`` with color sequences and a lightweight render loop.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/cterm/frame-color-animations

Press ``Q`` to quit.

Captured Output (80x25)
=======================

.. include:: _captures/frame-color-animations.rstinc

Features Demonstrated
=====================

* ``FrameDrawOptions::setFrameColorSequence()`` with several ``FrameColorMode`` values.
* Animated outer frames combined with animated fill colors.
* A side-by-side comparison of multiple ``FrameStyle`` values.
* Simple full-screen rendering with one persistent ``Buffer``.

Related Demos
=============

* :doc:`Frame Weaver <frame-weaver>` for frame intersections, mixed line styles, and custom tiles.
* :doc:`Display All Colors <display-all-colors>` for the underlying palette combinations.
* :doc:`Retro Plasma <retro-plasma>` for another view of repeated full-screen redraw behavior.

Relevant Source Files
=====================

If you want to explore the implementation, start with
:file:`demos/cterm/FrameColorAnimations/src/FrameColorAnimationsApp.cpp`.

This file contains the animation loop, panel layout, and color sequences for each frame mode.
