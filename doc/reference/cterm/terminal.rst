.. index::
    single: Terminal Interface

******************
Terminal Interface
******************

Screen Dimensions
=================

``Terminal::size()`` is the full usable screen area, including the last column and row.
Constructor dimensions and ``setSize()`` use the same units and are bounded to 1×1 through 2048×2048. ``setSize()``
changes the assumed screen dimensions and schedules a redraw; it does not resize a native window.
When enabled, native size detection replaces the assumed dimensions.
Screen updates disable automatic wrapping and position rows explicitly, so filling the bottom-right cell does not
require an extra newline or scroll the screen.
Ordinary streaming writes retain normal wrapping and explicit line breaks.

Virtual Terminals
=================

``TerminalFlag::VirtualTty`` selects an independent ANSI stdout backend on every platform.
It bypasses native terminal detection, console setup, signal registration, and input.
After ``initializeScreen()``, ``isInteractive()`` returns true even when stdout is a pipe or file.
The constructor size remains in effect until ``setSize()`` changes it; ``testScreenSize()`` preserves that size.
Use the regular screen lifecycle to emit cursor visibility and alternate-screen restoration sequences.

Virtual input is always empty and never reads stdin.
``readLine()`` returns empty text and ``waitForKey()`` returns an invalid key immediately.
In key mode, ``readKey(timeout)`` waits for a positive timeout before returning an invalid key; zero or negative amounts
return immediately.
In line mode, it returns immediately regardless of the timeout.
Environment variables are interpreted by demo support, not by ``Terminal``.

Key Input Timeouts
==================

For native terminal backends, ``Input::readKey()`` accepts a :cpp:type:`Milliseconds <erbsland::time::Milliseconds>`
amount.
In key mode, zero or negative amounts perform a nonblocking poll; positive amounts use one monotonic deadline across
input collection and decoding.
An expired wait returns an invalid key if no supported key was decoded.
``waitForKey()`` waits without a finite deadline.
In line-reading mode, both operations wait for a complete line and the timeout is ignored.

``TerminalError`` reports a terminal capability required by an operation that is unavailable, such as starting an
interactive editor on a terminal without full-control output.
Violations of internal buffer invariants are logic failures.

Observing Scrolled Rows
=======================

``CursorBuffer::setScrollOutCallback()`` observes rows before they leave the top during a northward shift.
The callback receives a borrowed, complete, one-row ``ReadableBuffer`` view with all colors, attributes, and wide cells.
Automatic overflow and explicit shifts both notify it, with multiple departing rows reported from top to bottom.
The view is valid only during the callback; copy or serialize it to retain its content.
Callbacks must not throw or modify the observed buffer.
Assign an empty callback to stop observing.
Observers belong to their buffer instance: copy/move construction starts without an observer, and assignment preserves
the destination observer.
Copying a frame from ``TerminalEmulation`` therefore cannot retain a callback to its original emulator.
Southward and horizontal shifts, row editing, resizing, and clearing do not report history rows.

Interface
=========

.. doxygenclass:: erbsland::cterm::Backend
    :members:
.. doxygenclass:: erbsland::cterm::Bitmap
    :members:
.. doxygenenum:: erbsland::cterm::BitmapColorMode
.. doxygenclass:: erbsland::cterm::BitmapDrawOptions
    :members:
.. doxygenenum:: erbsland::cterm::BitmapScaleMode
.. doxygenclass:: erbsland::cterm::Block
    :members:
.. doxygenclass:: erbsland::cterm::Block16Style
    :members:

.. doxygentypedef:: erbsland::cterm::Block16StylePtr
.. doxygenclass:: erbsland::cterm::BlockAttributes
    :members:
.. doxygenclass:: erbsland::cterm::BlockCombinationStyle
    :members:

.. doxygentypedef:: erbsland::cterm::BlockCombinationStylePtr
.. doxygentypedef:: erbsland::cterm::BlockCount
.. doxygentypedef:: erbsland::cterm::BlockIndex
.. doxygenclass:: erbsland::cterm::BlockPrintContext
    :members:
.. doxygentypedef:: erbsland::cterm::BlockRange
.. doxygentypedef:: erbsland::cterm::BlockStringLines

.. doxygenclass:: erbsland::cterm::BlockString
    :members:
.. doxygenclass:: erbsland::cterm::BlockStringEditor
    :members:

.. doxygentypedef:: erbsland::cterm::BlockStringEditorLines
.. doxygenclass:: erbsland::cterm::BlockStyle
    :members:
.. doxygenclass:: erbsland::cterm::BlockText
    :members:
.. doxygenenum:: erbsland::cterm::BlockTextAnimation
.. doxygenclass:: erbsland::cterm::BlockTextOptions
    :members:
.. doxygenstruct:: erbsland::cterm::BlockUnit
    :members:
.. doxygenclass:: erbsland::cterm::Buffer
    :members:
.. doxygenclass:: erbsland::cterm::BufferConstRefView
    :members:
.. doxygenclass:: erbsland::cterm::BufferDrawOptions
    :members:
.. doxygenenum:: erbsland::cterm::BufferResizeMode
.. doxygenclass:: erbsland::cterm::BufferView
    :members:
.. doxygenclass:: erbsland::cterm::BufferViewBase
    :members:
.. doxygenclass:: erbsland::cterm::Color
    :members:

.. doxygentypedef:: erbsland::cterm::ColorList
.. doxygenclass:: erbsland::cterm::ColorBase
    :members:
.. doxygenclass:: erbsland::cterm::ColorPart
    :members:

.. doxygentypedef:: erbsland::cterm::Background

.. doxygentypedef:: erbsland::cterm::bg

.. doxygentypedef:: erbsland::cterm::Foreground

.. doxygentypedef:: erbsland::cterm::fg
.. doxygenenum:: erbsland::cterm::ColorRole
.. doxygenclass:: erbsland::cterm::ColorSequence
    :members:
.. doxygenclass:: erbsland::cterm::CropEdges
    :members:
.. doxygenclass:: erbsland::cterm::CursorBuffer
    :members:
.. doxygenclass:: erbsland::cterm::CursorWriter
    :members:
.. doxygenclass:: erbsland::cterm::Font
    :members:

.. doxygentypedef:: erbsland::cterm::FontPtr
.. doxygenclass:: erbsland::cterm::FontGlyph
    :members:
.. doxygenclass:: erbsland::cterm::FrameBorder
    :members:
.. doxygenenum:: erbsland::cterm::FrameBorderElement
.. doxygenenum:: erbsland::cterm::FrameColorMode
.. doxygenclass:: erbsland::cterm::FrameDrawOptions
    :members:
.. doxygenenum:: erbsland::cterm::FrameStyle
.. doxygenclass:: erbsland::cterm::GridLayout
    :members:
.. doxygenclass:: erbsland::cterm::Input
    :members:
.. doxygenclass:: erbsland::cterm::Key
    :members:
.. doxygenenum:: erbsland::cterm::KeyModifier
.. doxygenclass:: erbsland::cterm::KeyModifiers
    :members:
.. doxygenclass:: erbsland::cterm::Keys
    :members:
.. doxygenclass:: erbsland::cterm::MatrixBlockCombinationStyle
    :members:
.. doxygenenum:: erbsland::cterm::MoveMode
.. doxygenenum:: erbsland::cterm::ParagraphBackgroundMode
.. doxygenclass:: erbsland::cterm::ParagraphIndents
    :members:
.. doxygenenum:: erbsland::cterm::ParagraphOnError
.. doxygenclass:: erbsland::cterm::ParagraphOptions
    :members:
.. doxygenenum:: erbsland::cterm::ParagraphSpacing
.. doxygenclass:: erbsland::cterm::ReadableBuffer
    :members:
.. doxygenclass:: erbsland::cterm::RemappedBuffer
    :members:
.. doxygenclass:: erbsland::cterm::SimpleBlockCombinationStyle
    :members:
.. doxygenenum:: erbsland::cterm::TabOverflowBehavior
.. doxygenclass:: erbsland::cterm::Terminal
    :members:
.. doxygenclass:: erbsland::cterm::TerminalError
    :members:
.. doxygenenum:: erbsland::cterm::TerminalFlag
.. doxygenclass:: erbsland::cterm::TerminalFlags
    :members:
.. doxygenclass:: erbsland::cterm::TerminalOutputGuard
    :members:
.. doxygenclass:: erbsland::cterm::TerminalSession
    :members:
.. doxygenclass:: erbsland::cterm::TerminalStream
    :members:
.. doxygenclass:: erbsland::cterm::Tile9Style
    :members:

.. doxygentypedef:: erbsland::cterm::Tile9StylePtr
.. doxygenconcept:: erbsland::cterm::PrintableArg

.. doxygenconcept:: erbsland::cterm::CharColorConstructorArgs
.. doxygenclass:: erbsland::cterm::UpdateSettings
    :members:
.. doxygenclass:: erbsland::cterm::WritableBuffer
    :members:
.. doxygenclass:: erbsland::cterm::WriteClippedBuffer
    :members:
.. doxygenclass:: erbsland::cterm::WriteClippedBufferBase
    :members:
.. doxygenclass:: erbsland::cterm::WriteClippedBufferRef
    :members:
