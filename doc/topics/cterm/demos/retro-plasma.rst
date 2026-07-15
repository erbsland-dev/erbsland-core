..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************
Retro Plasma
************

``retro-plasma`` is a full-screen animation demo that renders a classic plasma effect directly in the terminal.
It is both a visual showcase and a practical example of continuous buffer updates, palette switching, keyboard controls,
and resize-aware rendering.

Use This Demo When You Need...
==============================

* A support example for continuous full-screen animation with keyboard input.
* A place to study palette-based color animation with ``ColorSequence``.
* A lively example of a resize-aware terminal application that still keeps its controls readable.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/cterm/retro-plasma

Use ``F`` to speed up, ``S`` to slow down, ``P`` or Space to pause, ``C`` to switch palettes, and ``Q`` to quit.

Captured Output (80x25)
=======================

.. include:: _captures/retro-plasma.rstinc

The capture may not show the terminal output correctly.
Here a screenshot:

.. figure:: images/retro-plasma.jpg
    :width: 100%

Features Demonstrated
=====================

* Full-screen rendering using ``Buffer`` together with ``Terminal::updateScreen()``.
* A timed animation loop combined with keyboard input in ``Input::Mode::Key``.
* Palette-based color animation implemented with ``ColorSequence``.
* Resize-aware rendering using custom ``UpdateSettings`` with minimum-size handling.
* A dynamic footer prompt composed from colored ``BlockString`` fragments.

Related Demos
=============

* :doc:`Frame Color Animations <frame-color-animations>` for a more structured view of repeated ``updateScreen()`` calls.
* :doc:`Frame Color Animations <frame-color-animations>` for a simpler, frame-focused color animation example.
* :doc:`Key Input Demo <key-input-demo>` for another example of immediate input in a full-screen animation loop.

Relevant Source Files
=====================

If you want to explore how the effect is implemented, start with :file:`demos/cterm/retro-plasma/src/RetroPlasmaApp.cpp`
and :file:`demos/cterm/retro-plasma/src/PlasmaRenderer.cpp`.
