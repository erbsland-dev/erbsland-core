..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********
HTML Viewer
***********

``html-viewer`` demonstrates the complete path from a filesystem-backed HTML file to styled terminal output.
It reads the file with ``Path`` and ``PathContent``, parses the text with ``text::html::HtmlParser`` into a
``text::TextDocument``, and renders the document with ``TerminalDocumentRenderer`` into a scrollable ``CursorBuffer``.

Use This Demo When You Need...
==============================

* A complete example for loading text files through the Core path APIs.
* A practical reference for turning HTML into ``TextDocument`` trees.
* A terminal viewer that scrolls rendered document content with ``CursorBuffer`` and ``BufferView``.

Run the Demo
============

Start the demo from the build directory:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/cterm/html-viewer

Use the arrow keys to scroll, page up and page down to move by pages, ``S`` to cycle the document style, and ``Q`` or
``Esc`` to quit.
You can pass your own input file and render once into standard output:

.. code-block:: console

    $ ./cmake-build-debug/demo-apps/cterm/html-viewer --print --style=simple my-document.html

Captured Output (80x25)
=======================

.. include:: _captures/html-viewer.rstinc

Features Demonstrated
=====================

* ``Path::fromNativeOrThrow()`` and ``PathContent::readTextOrThrow()`` for file input.
* ``HtmlParser`` as the public tolerant HTML parser.
* ``TextDocument`` as the semantic document tree shared between parser and renderer.
* ``TerminalDocumentRenderer`` with the plain, simple, and styled default document styles.
* ``CursorBuffer`` plus ``BufferView`` for a retained scrollable document viewport.

Related Demos
=============

* :doc:`Log Viewer <log-viewer>` for another retained-buffer viewport.
* :doc:`Text Gallery <text-gallery>` for styled text and block rendering examples.
* :doc:`Terminal Chronicle <terminal-chronicle>` for direct terminal output without a retained buffer.

Relevant Source Files
=====================

If you want to explore the implementation, start with :file:`demos/cterm/html-viewer/src/HtmlViewerApp.cpp`.

This file contains the option setup, Core path-based file loading, HTML parsing, document rendering, and viewport
navigation.
