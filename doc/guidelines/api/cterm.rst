************************************
Color Terminal Domain API Guidelines
************************************

Core Semantics
==============

Cell Model
----------

.. code-block:: text

    screen = rectangular grid of styled terminal cells
    block = one cell with color, attributes, and zero or more Unicode code points
    display width = zero-width continuation, ordinary one-cell block, or leading cell of a two-cell block
    inherited color = defer foreground or background to the destination
    reset color = restore the terminal default

Buffer Model
------------

.. code-block:: text

    readable buffer = rectangular cell source in terminal-cell coordinates
    writable buffer = readable buffer with mutation and drawing operations
    view = translated or clipped access to another buffer without copying its cells
    cursor buffer = retained buffer with terminal-style streaming writes and overflow behavior

Terminal Model
--------------

.. code-block:: text

    direct output = immediate cursor-oriented writes through the active backend
    screen update = render a complete buffer using clear, overwrite, or differential refresh
    session = scoped ownership of terminal setup and restoration
    key input = decoded key, text, and modifier information with bounded or blocking reads
    secret input = fixed protected editing storage with a marked String result after commit

Primary Types
=============

.. code-block:: text

    Terminal // high-level terminal screen, cursor output, and input interface
    Block // one styled terminal cell
    BlockString // retained read-only cell sequence
    Color, BlockStyle, BlockAttributes // cell color and presentation values
    ReadableBuffer, WritableBuffer, Buffer // abstract and concrete two-dimensional cell buffers
    CursorWriter, CursorBuffer // shared streaming output API and retained implementation
    Input, Key // terminal key-input interface and decoded key value

Buffer Types
============

.. code-block:: text

    BufferView, BufferConstRefView // owning and borrowed rectangular buffer views
    RemappedBuffer // row- or column-remapped buffer for efficient reordering
    WriteClippedBuffer, WriteClippedBufferRef // owning and borrowed clipped paint targets
    BufferResizeMode // fast or content-preserving buffer resize policy
    BufferDrawOptions, CropEdges // buffer composition and clipping values
    BlockIndex, BlockCount, BlockRange, BlockUnit // block-string unit family
    BlockStringEditor // explicit mutable cell sequence
    ColorPart, ColorRole // color component and semantic role

Drawing Types
=============

.. code-block:: text

    BlockText, BlockTextOptions // positioned and styled text drawing description
    BlockTextAnimation, ColorSequence // animated text and color sequencing
    ParagraphOptions, ParagraphIndents // paragraph wrapping, spacing, and indentation policy
    ParagraphBackgroundMode, ParagraphSpacing, ParagraphOnError // paragraph layout and failure policies
    TabOverflowBehavior // paragraph tab-stop overflow policy
    Bitmap, BitmapDrawOptions // boolean pixel image and cell rendering options
    Font, FontGlyph // named bitmap glyph collection and one glyph
    FrameBorder, FrameDrawOptions, FrameStyle // frame geometry and presentation
    FrameBorderElement // one selectable frame-border component
    GridLayout // reusable framed grid geometry
    Block16Style, BlockCombinationStyle, Tile9Style // block-glyph composition strategies

Input Types
===========

.. code-block:: text

    Keys, KeyModifiers // key collections and active modifier flags
    MoveMode // absolute or relative cursor movement
    ReadLine, ReadSecret // ordinary and protected interactive single-line editors
    ReadLineOptions, ReadLineDisplayStyle // reusable editor behavior and presentation
    ReadLineResult, ReadLineStatus // shared interactive input result and status

Terminal Integration Types
==========================

.. code-block:: text

    Backend // platform terminal output and input backend interface
    TerminalSession // scoped terminal setup and restoration
    TerminalStream // text stream adapter for terminal output
    TerminalStreamSynchronization // shared synchronization state for terminal streams
    TerminalFlags, UpdateSettings // terminal construction and screen update policies

Document Types
==============

.. code-block:: text

    TerminalDocumentRenderer // renderer from semantic text documents to terminal cells
    TerminalDocumentStyle // selector-driven terminal document style sheet
    TerminalDocumentStyleSelector, TerminalDocumentStyleRule // style matching and declarations
    TerminalDocumentStyleMarker // list-marker definition for terminal documents

Pattern Definitions
===================

.. code-block:: text

    B = ReadableBuffer/WritableBuffer/Buffer // participating buffer type
    R = ReadLineResult // ordinary or secret interactive read result
    S = BlockString/BlockStringEditor // read-only or mutable block text

Block and Style Patterns
========================

.. code-block:: text

    T(text[, style]) // create a block or block string from text and presentation
    o.text()/color()/attributes() -> T // inspect block content or presentation
    o.setColor/setAttributes(value) -> T& // update cell presentation
    o.displayWidth() -> int // get the terminal-cell width of a block
    o.toString() -> text::String // create the shortest lossless ELCL representation
    T::fromString/fromStringOrThrow(text) -> T // parse a style value with fallback or diagnostics

Buffer Read Patterns
====================

.. code-block:: text

    o.size()/rect() -> T // get buffer dimensions or bounds
    o.get(position[, fallback]) -> Block // read a cell with tolerant bounds behavior
    o.clone() -> WritableBufferPtr // create an independent writable copy
    o.toMask(characters[, invert]) -> Bitmap // derive a bitmap from matching cell content
    T(buffer, rectangle) // create a translated rectangular view of another buffer

Buffer Write Patterns
=====================

.. code-block:: text

    o.set(position, block) // write one block using display-width rules
    o.fill([rectangle], block) // fill the complete buffer or a clipped rectangle
    o.resize(size[, mode, fill]) // resize with explicit preservation intent
    o.drawBuffer(source[, options]) // compose another buffer into the target
    o.drawBlockText(text[, options]) // layout and draw terminal text
    o.drawBitmap/drawFrame/drawGridLayout(value[, options]) // draw reusable visual primitives
    T::fromLines(lines) -> Buffer // build a buffer from retained block-text lines

Cursor Write Patterns
=====================

.. code-block:: text

    o.write(block/text/buffer) // write content at the cursor using the active style
    o.print/printLine(arguments) // write mixed text and style arguments
    o.printParagraph(text[, options]) -> int // write wrapped paragraph lines
    o.moveCursor(position, mode) // move absolutely or relatively and clear pending wrap
    o.setColor/setStyle(value) // update presentation for subsequent writes
    o.setAutoWrap/setCursorVisible(enabled) // configure cursor output behavior

Terminal Patterns
=================

.. code-block:: text

    T([size, flags]) // create a terminal using the platform backend
    T(backend[, size]) // create a terminal using a custom backend
    o.updateScreen(buffer[, settings]) // render a complete retained screen
    o.testScreenSize() // refresh detected drawable dimensions
    o.size() -> bgeo::BlockSize // inspect drawable dimensions
    o.clearScreen()/flush() // apply immediate terminal output control
    o.input() -> Input& // access the terminal-owned input interface
    o.beginSession() -> TerminalSession // enter scoped terminal control

Input Patterns
==============

.. code-block:: text

    o.readKey(timeout) -> Key // read a key or return an empty key after the timeout
    o.waitForKey() -> Key // block until a decoded key is available
    o.codePoint()/modifiers() -> T // inspect text and modifier data
    o.is❮Key❯()/isText() -> bool // classify decoded input

Interactive Read Patterns
=========================

.. code-block:: text

    T::create(terminal[, options]) -> TPtr // retain a terminal and copy normalized editor options
    o.start()/stop() // begin or end a polling input lifecycle
    o.update()/waitForInput() -> ReadLineResult // poll once or block until a terminal result
    o.status()/data() -> T // inspect a ReadLineResult outcome and ordinary or marked committed text
    o.set❮Property❯(value) -> ReadLineOptions& // fluently configure shared editor behavior

Document Rendering Patterns
===========================

.. code-block:: text

    T([style]) // create a semantic text-document renderer
    T::defaultPlain/defaultSystemOutput() -> T // create predefined document presentation
    o.renderTo(writer, document) // write completed styled physical lines to a cursor writer
    T::default❮Style❯() -> TerminalDocumentStyle // create a predefined style sheet
