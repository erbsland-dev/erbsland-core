..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**************
Key Input Demo
**************

``key-input-demo`` combines immediate key handling with a horizontally scrolling retained buffer. Each pressed key is
stamped into the field as a colored block, so the demo makes keyboard input, buffer updates, and continuous motion
visible at the same time.

Use This Demo When You Need...
==============================

* A support example for ``Input::Mode::Key`` and immediate key handling.
* A visual reference for scrolling content inside a retained buffer.
* A small demo that mixes user input with animation in the same screen.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/key-input-demo

Press any key to stamp a block into the field. Press ``Esc`` to quit.

Captured Output (80x25)
=======================

.. include:: _captures/key-input-demo.rstinc

Features Demonstrated
=====================

* ``Input::Mode::Key`` with immediate, non-line-buffered key handling.
* ``Key::toDisplayText()`` for user-facing key labels.
* A horizontally moving retained buffer used as a simple scrolling field.
* Mixed animation and event-driven drawing on the same screen.

Related Demos
=============

* :doc:`Frame Color Animations <frame-color-animations>` for another interactive demo focused on refresh behavior.
* :doc:`Log Viewer <log-viewer>` for viewport-style scrolling over a larger retained history.
* :doc:`Retro Plasma <retro-plasma>` for another keyboard-controlled full-screen animation.

Relevant Source Files
=====================

If you want to explore the implementation, start with :file:`demos/cterm/key-input-demo/src/KeyInputDemoApp.cpp`.

This file contains the scrolling buffer setup, key stamping logic, and footer/help text generation.
