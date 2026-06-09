..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

******
Buffer
******

The buffer classes represent rendered terminal content in memory before it is written to the screen.

``ReadableBuffer`` provides a read-only inspection API, ``WritableBuffer`` extends this with mutation and drawing
operations, and ``Buffer`` is the concrete 2D storage type used in most applications.

For more specialized use cases, ``RemappedBuffer`` adds efficient row- and column-based reordering.
This is ideal for editors, scrollback views, or any workload that frequently inserts, deletes, or moves whole lines.

Building on top of that, ``CursorBuffer`` provides a VT-style cursor-writing interface via ``CursorWriter``.

Use these types whenever you want to build frames off-screen, compare frames, copy content between buffers, or derive
masks from rendered characters.

Usage
=====

Reading and Writing Through Buffer Interfaces
---------------------------------------------

``ReadableBuffer`` and ``WritableBuffer`` allow helper functions to operate on terminal content without depending on a
specific implementation.

.. code-block:: cpp

    auto renderStatusPanel(WritableBuffer &target, BlockRectangle panel) -> void {
        target.fill(panel, Block{" ", Color{fg::Inherited, bg::Blue}});
        target.drawBlockText(
            "Status",
            panel.insetBy(BlockMargins{1}),
            Alignment::TopLeft,
            Color{fg::BrightWhite, bg::Blue});
    }

    auto screen = Buffer{BlockSize{80, 24}};
    renderStatusPanel(screen, BlockRectangle{2, 2, 24, 8});

Use ``ReadableBuffer`` when your function only needs to inspect content, count differences, or derive masks.

Use ``WritableBuffer`` when your function should modify the target buffer without caring whether it operates on a
standalone ``Buffer`` or another writable implementation.

Write-Clipped Paint Operations
------------------------------

``WriteClippedBufferRef`` is a thin writable wrapper for temporary subsurface painting.
It exposes source coordinates to the drawing code and clips write operations to a target rectangle in the wrapped
buffer.
Read operations are translated into the wrapped buffer without applying the write clip, which lets drawing helpers
sample existing cells around the clipped area.

.. code-block:: cpp

    auto screen = Buffer{BlockSize{80, 24}};
    auto panel = WriteClippedBufferRef{
        screen,
        BlockPosition{0, 0},
        BlockRectangle{10, 4, 32, 8}};

    panel.fill(panel.sourceRect(), Block{" ", Color{fg::Inherited, bg::Blue}});
    panel.drawBlockText("Panel title", BlockRectangle{0, 0, 32, 1}, Alignment::Center);

Use ``WriteClippedBuffer`` when the wrapper must store a shared pointer to the wrapped buffer.
Use ``WriteClippedBufferRef`` for short-lived paint passes where the wrapped buffer already outlives the wrapper.

Cloning, Copying, and Resizing
------------------------------

``Buffer`` supports the typical frame-management tasks required by interactive terminal applications: creating new
frames, cloning the current state, and resizing buffers when the terminal size changes.

.. code-block:: cpp

    auto current = Buffer{BlockSize{80, 24}};
    current.fill(Block{" ", Color{fg::Inherited, bg::Black}});

    const auto previous = current.clone();
    current.resize(BlockSize{100, 30}, BufferResizeMode::PreserveContent, Block::space());

``clone()`` returns a writable copy through the abstract interface.
This makes it easy to store previous frames for diffing, animation steps, or rollback logic.

Use ``BufferResizeMode`` to make the resize intent explicit:

* ``BufferResizeMode::Fast`` uses the fastest resize path and leaves existing content undefined.
* ``BufferResizeMode::PreserveContent`` keeps the visible rectangle stable and fills newly exposed cells with the
  provided fill character.

Working with Remapped Buffers
-----------------------------

``RemappedBuffer`` is designed for workloads where the content remains logically grid-based, but rows or columns are
frequently reshuffled.

Instead of rewriting every affected cell, the buffer maintains remapping tables and only updates rows or columns that
become newly visible.
This keeps operations like scrolling or line insertion efficient, even for large buffers.

.. code-block:: cpp

    auto history = RemappedBuffer{BlockSize{80, 2'000}, Orientation::Vertical};
    history.fill(Block::space());

    history.eraseRows(0, Block::space(), 1);      // Scroll everything up by one row.
    history.set(BlockPosition{0, 1'999}, BlockString{"new log line"});

    history.resize(BlockSize{100, 2'000}, BufferResizeMode::PreserveContent, Block::space());

Use the plain ``RemappedBuffer::resize()`` overload when you want maximum performance and plan to redraw the content
anyway.

Use ``BufferResizeMode::PreserveContent`` when the visible order must remain stable while expanding or cropping the
buffer.

For ``RemappedBuffer``, this preserve-content mode is fast when only the primary orientation axis changes.
If the secondary axis changes, preserving content requires rebuilding the logical view and is therefore significantly
more expensive.

Streaming Scrollback with CursorBuffer
--------------------------------------

``CursorBuffer`` is the right choice when text is appended over time, as if it were written directly to a terminal.

It tracks a cursor position, maintains an active color, and supports streaming writes via ``CursorWriter``.
When the cursor reaches the bottom edge, it can wrap, scroll, or grow vertically depending on the configured overflow
mode.

Newly created cells are initialized using ``fillChar()``, allowing you to keep a consistent background color or
placeholder glyph as the buffer grows.

.. code-block:: cpp

    auto logHistory = CursorBuffer{
        BlockSize{120, 10},
        CursorBuffer::OverflowMode::ExpandThenShift,
        BlockSize{120, 500},
        Block{" ", Color{fg::Default, bg::Black}}};

    logHistory.setColor(Color{fg::BrightBlue, bg::Black});
    logHistory.printParagraph("2026-03-26 09:02:23 INF Request completed in 43 ms");

    logHistory.setColor(Color{fg::BrightYellow, bg::Black});
    logHistory.printParagraph("2026-03-26 09:03:04 WRN Cache refresh is still pending");

    const auto visibleTop = std::max(0, logHistory.size().height() - 20);
    auto view = BufferConstRefView{logHistory, BlockRectangle{0, visibleTop, 120, 20}};
    terminal.updateScreen(view);

This pattern works especially well for log viewers, REPL-style tools, dashboards, or any application that needs a
growing history buffer with a live viewport onto the most recent content.

If your fill strategy changes later, update it with ``CursorBuffer::setFillChar``.

For details about the streaming API itself—such as ``print()``, ``printLine()``, and cursor movement—see
:doc:`Cursor Output <cursor-output>`.

.. important::

    For an efficient render loop, keep a persistent instance of ``Buffer`` and
    simply resize it when the terminal size changes.

    Reusing the same buffer avoids unnecessary memory allocations and helps keep rendering predictable and fast.

    A typical render loop might look like this:

    .. code-block:: cpp

        struct MyApp {
            void renderLoop() {
                for (;;) {
                    _terminal.testScreenSize();
                    _buffer.resize(_terminal.size());

                    // Render the current frame into the buffer.
                    _terminal.updateScreen(_buffer);

                    // Handle key presses or other input.
                }
            }

            Terminal _terminal;
            Buffer _buffer;
        };

Building Buffers from BlockText Lines
-------------------------------------

For status panels, generated reports, or static UI elements, ``Buffer`` can be constructed directly from line-oriented
text.

.. code-block:: cpp

    const auto help = Buffer::fromLinesInString(BlockString{
        "Q  Quit\n"
        "R  Refresh\n"
        "H  Toggle help"});

    auto screen = Buffer{BlockSize{40, 12}};
    screen.setFrom(help, Block::space());

This is often the fastest way to turn preformatted terminal text into a buffer that can later be positioned within a
larger layout.

Copying and Aligning Sub-Regions
--------------------------------

``BufferDrawOptions`` makes buffer-to-buffer composition explicit.
It bundles the target region, the optional source crop, and the color-combination strategy for
``WritableBuffer::drawBuffer()``.

.. code-block:: cpp

    auto frame = Buffer{BlockSize{52, 16}};
    auto sprite = Buffer{BlockSize{12, 5}};
    sprite.drawFrame(sprite.rect(), FrameStyle::Double, Color{fg::BrightCyan, bg::Inherited});
    sprite.drawBlockText("CPU", sprite.rect(), Alignment::Center, Color{fg::BrightWhite, bg::Inherited});

    auto options = BufferDrawOptions{
        BlockRectangle{30, 3, 18, 7},
        BlockRectangle{0, 0, 12, 5}};
    options.setOverwriteColors(false);

    frame.drawBuffer(sprite, options);

Use a zero-sized ``targetRect`` when you only need an exact target position.
Use a zero-sized ``sourceRect`` when the whole source buffer should be copied.

If you need to preserve the target glyphs while only adopting parts of the source style, attach a
``BlockCombinationStyle`` through ``setCombinationStyle()``.

Comparing Frames and Deriving Masks
-----------------------------------

``ReadableBuffer`` also provides analysis helpers that are useful for tests, animation pipelines, and bitmap-based
effects.

.. code-block:: cpp

    const auto changedCells = previous->countDifferencesTo(current);
    const auto frameMask = current.toMask({U'|', U'-', U'+', U'┌', U'┐', U'└', U'┘'});

    if (changedCells > 0 && frameMask.size().contains(BlockPosition{0, 0})) {
        // React to the changed frame content.
    }

Use ``toMask()`` when you want to reason about the *structure* of rendered content (for example, line art or borders)
instead of raw character or color data.

