..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

****************
Buffer Rendering
****************

``ReadableBuffer::toString()`` serializes a buffer's logical cells into a ``String``.
It is inherited by ``Buffer``, ``CursorBuffer``, and readable buffer views.

Formats and Options
===================

``BufferRenderOptions`` is a fluent value type.
``setFormat(RenderFormat::Text)`` selects plain text; ``RenderFormat::Ansi`` selects ANSI SGR presentation.
ANSI is the default, and the enum permits additional representations in future versions.

``setTrimTrailingSpaces(true)`` removes default-style spaces at row ends.
``setTrimTrailingEmptyLines(true)`` removes default-style empty rows at the bottom.
Both options default to false, preserving the buffer's dimensions.
Visually meaningful styled blanks are retained, including when trimming is enabled.

Output Contract
===============

Rows are separated by LF; no final newline is appended.
Wide-character continuation cells are not emitted a second time.
ANSI output resolves inherited presentation against terminal defaults, preserves supported colors and attributes, and
resets presentation at its boundaries.
Plain output contains no terminal control sequences.
Rendering does not modify the source buffer or cursor.

For compact documentation output, enable both trimming options and append a final newline only if the surrounding file
format requires one.

Interface
=========

.. doxygenclass:: erbsland::cterm::BufferRenderOptions
    :members:
.. doxygenenum:: erbsland::cterm::RenderFormat
