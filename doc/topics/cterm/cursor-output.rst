..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*************
Cursor Output
*************

Cursor output is the shared streaming API used by ``Terminal`` and ``CursorBuffer``.

``CursorWriter`` combines cursor movement, active color state, active character attributes, plain text output, and
paragraph printing into a single, consistent interface.
``MoveMode`` distinguishes between absolute and relative cursor movement.

Use this API when you want to print mixed text and color arguments, write terminal-style output into a ``CursorBuffer``,
or precisely control cursor positioning.

This page documents the shared output model only.
Screen lifecycle, full-screen rendering, refresh settings, and backend ownership remain on
:doc:`terminal`.

Usage
=====

Printing Mixed Arguments
------------------------

``CursorWriter`` is designed for fluent, terminal-style output.

The ``print()`` and ``printLine()`` functions accept a mix of colors, character styles, character attributes, terminal
strings, and plain text in a single call.
As arguments are processed, the active color and attribute state is updated automatically.

.. code-block:: cpp

    terminal.printLine(
        fg::BrightGreen,
        "Service started at ",
        fg::BrightWhite,
        "09:42");

    terminal.print(
        bg::BrightBlack,
        fg::BrightYellow,
        "[Q] ",
        fg::BrightWhite,
        "quit");

    auto emphasis = BlockAttributes{};
    emphasis.setBold(true);
    emphasis.setUnderline(true);
    const auto headingStyle = BlockStyle{Color{fg::BrightWhite, bg::Inherited}, emphasis};

    terminal.printLine(
        headingStyle,
        "Important",
        BlockAttributes::reset(),
        fg::BrightBlack,
        " uses both bold and underline.");

Because the active color and attribute state is part of the writer, subsequent output continues using the most recently
selected values until you change them again or call ``CursorWriter::setDefaultColor()``.
Use ``BlockAttributes::reset()`` when you want to explicitly turn all attributes back off.
Use ``CursorWriter::style()`` and ``CursorWriter::setStyle()`` when you want to read or update the combined style in one
step.

Moving the Cursor Explicitly
----------------------------

Use ``MoveMode`` when you need to control whether a movement is relative to the current cursor position or absolute
within the writable area.

.. code-block:: cpp

    cursorBuffer.moveCursor(BlockPosition{10, 3}, MoveMode::Absolute);
    cursorBuffer.print("Header");

    cursorBuffer.moveCursor(BlockPosition{0, 2}, MoveMode::Relative);
    cursorBuffer.printLine("First entry");

All convenience functions—such as ``moveLeft()``, ``moveRight()``, ``moveUp()``, ``moveDown()``, ``moveTo()``, and
``moveHome()`` —are built on top of this same distinction.

VT100-Compatible Line Wrapping
------------------------------

The cursor writer follows a wrapping model compatible with traditional VT100-style terminals.
This behavior is slightly different from a naïve “wrap immediately at the edge” approach.

When writing characters near the right edge of the buffer:

* A character written into the **last column** does *not* immediately
  move the cursor to the next line.
* Instead, a *pending wrap* is recorded internally.
* The actual line break only happens **when the next character is written**.

This means:

* The last column can be filled without triggering an immediate line break.
* The next character causes the cursor to move to the beginning of the
  next line (if auto-wrap is enabled).

Wide characters (display width ``2`` ) are handled carefully:

* If a wide character would not fully fit at the end of the line:

  - With auto-wrap enabled, a line break occurs first.
  - With auto-wrap disabled, the character is ignored.

Additional details to keep in mind:

* If auto-wrap is disabled, writing at the right edge keeps the cursor
  pinned to the last column.
* Explicit cursor movement (e.g. ``moveCursor()``) clears any pending wrap.
* Line breaks triggered by wrapping use the configured overflow mode
  (wrap, shift, or expand).

This behavior ensures compatibility with terminal output expectations and avoids subtle off-by-one rendering issues when
mixing manual positioning and streaming text.

Printing Wrapped Paragraphs
---------------------------

Both ``Terminal`` and ``CursorBuffer`` implement the same paragraph-printing workflow via ``CursorWriter``.

This allows you to reuse one paragraph configuration for direct terminal output and for buffered, scrollable content.

.. code-block:: cpp

    auto options = ParagraphOptions::defaultOptions();
    options.setMaximumLineWraps(2);
    options.setParagraphSpacing(ParagraphSpacing::DoubleLine);

    logBuffer.printParagraph(
        "The same paragraph API works for live terminal output and for buffered history views.",
        options);

When writing to a ``Terminal``, output is sent immediately to the active backend.

When writing to a ``CursorBuffer``, the output becomes part of the buffer content and can later be rendered using
``Terminal::updateScreen()`` or displayed through a buffer view.

``printParagraph()`` uses the current writer style as the base for any indentation or trailing padding that must be
materialized as spaces.
If the wrapped paragraph should visually sit on a colored panel, set the writer background before printing.

Choosing Between Terminal and CursorBuffer
------------------------------------------

Use ``Terminal`` when output should be written directly to the real console.

Use ``CursorBuffer`` when you want to keep the same streaming API, but retain the output for scrolling, clipping,
copying, or re-rendering.

In practice, ``CursorWriter`` acts as the bridge between immediate output and retained, in-memory rendering.

