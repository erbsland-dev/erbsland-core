..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

********
Geometry
********

The geometry classes provide the building blocks for positioning and layout inside a terminal buffer.
They describe sizes, positions, rectangles, and directions, and let you derive new regions from existing ones.

Using explicit geometry types keeps layout code readable and makes it easier to build structured terminal interfaces.

.. dropdown:: Details about the example output on this page

    The examples below were rendered with the dedicated documentation
    helper :file:`doc/tools/geometry-reference.cpp` at a fixed width of
    72 terminal columns. This makes it easy to regenerate the visual
    output together with the code snippets.

Usage
=====

Deriving Layout Regions from a Canvas
-------------------------------------

The geometry types are designed to make screen layout explicit and easy to follow.
Instead of calculating coordinates manually, you derive smaller regions from a larger canvas and keep each intermediate
rectangle named.

.. code-block:: cpp

    const auto canvas = BlockRectangle{2, 3, 68, 8};
    const auto header = canvas.subRectangle(BlockAnchor::TopCenter, BlockSize{0, 2}, BlockMargins{0, 1, 0, 1});
    const auto footer = canvas.subRectangle(BlockAnchor::BottomCenter, BlockSize{0, 1}, BlockMargins{0, 1, 0, 1});
    const auto body = canvas.insetBy(BlockMargins{2, 1, 1, 1});
    const auto sidebar = body.subRectangle(BlockAnchor::Left, BlockSize{18, 0}, BlockMargins{0, 1, 0, 0});
    const auto content = body.subRectangle(BlockAnchor::Right, BlockSize{body.width() - 19, 0}, BlockMargins{0});

    buffer.drawFrame(canvas, FrameStyle::Double, Color{fg::BrightWhite, bg::Inherited});
    buffer.drawBlockText("Header", header, Alignment::Center, Color{fg::BrightWhite, bg::Blue});
    buffer.drawBlockText("Sidebar", sidebar, Alignment::Center, Color{fg::BrightWhite, bg::Green});
    buffer.drawBlockText("Content", content, Alignment::Center, Color{fg::BrightWhite, bg::Magenta});
    buffer.drawBlockText("Footer", footer, Alignment::Center, Color{fg::BrightWhite, bg::BrightBlack});

Passing ``0`` as the width or height to ``BlockRectangle::subRectangle()`` means "use the full available size on that
axis".
This is especially useful for headers, footers, and sidebars that should stretch with the parent rectangle.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97;40m            subRectangle() + insetBy() keep layouts explicit            ␛[39;49m
    ␛[97;40m ␛[90mOne canvas can derive headers, sidebars, content, and footers cleanly.␛[97m ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m  ╔␛[96m╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤␛[97mheader␛[96m╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤╤␛[97m╗  ␛[39;49m
    ␛[97;40m  ║␛[96m╰┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴╯␛[97m║  ␛[39;49m
    ␛[97;40m  ║␛[92m╭────────────────╮␛[97m ␛[95m╭─────────────────────────────────────────────╮␛[97m║  ␛[39;49m
    ␛[97;40m  ║␛[92m│␛[97;42m                ␛[92;40m│␛[97m ␛[95m│␛[97;45m                                             ␛[95;40m│␛[97m║  ␛[39;49m
    ␛[97;40m  ║␛[92m│␛[97;42m    sidebar     ␛[92;40m│␛[97m ␛[95m│␛[97;45m                   content                   ␛[95;40m│␛[97m║  ␛[39;49m
    ␛[97;40m  ║␛[92m│␛[97;42m                ␛[92;40m│␛[97m ␛[95m│␛[97;45m                                             ␛[95;40m│␛[97m║  ␛[39;49m
    ␛[97;40m  ║␛[92m╰────────────────╯␛[97m ␛[95m╰─────────────────────────────────────────────╯␛[97m║  ␛[39;49m
    ␛[97;40m  ╚══════════════════════════════footer══════════════════════════════╝  ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m

Combining, Intersecting, and Testing Rectangles
-----------------------------------------------

``BlockRectangle`` supports union and intersection directly.
This is useful when you need to compute redraw regions, selection overlaps, or the visible area shared by two panels.

.. code-block:: cpp

    const auto a = BlockRectangle{4, 2, 13, 5};
    const auto b = BlockRectangle{11, 4, 16, 5};

    if (a.overlaps(b)) {
        const auto dirtyRegion = a | b;
        const auto sharedRegion = a & b;

        buffer.drawFrame(dirtyRegion, FrameStyle::Heavy, Color{fg::BrightMagenta, bg::Inherited});
        buffer.fill(sharedRegion, Block{" ", Color{fg::Inherited, bg::BrightBlack}});
        buffer.drawFrame(sharedRegion, FrameStyle::Double, Color{fg::BrightGreen, bg::Inherited});
    }

Use ``a | b`` when you need the combined area covered by both rectangles.
Use ``a & b`` when you need only their shared visible area.
The inexpensive ``BlockRectangle::overlaps()`` check is often the right guard before doing extra work.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97;40m               A | B                               A & B                ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m    ␛[95m┏━━━━━━━━━━━┯━━━━━━━━━┓␛[97m             ␛[96m┌───────────┐␛[97m                   ␛[39;49m
    ␛[97;40m    ␛[95m┃␛[97m     ␛[96mA␛[97m     ␛[96m│␛[97m         ␛[95m┃␛[97m             ␛[96m│␛[97m     ␛[96mA␛[97m     ␛[96m│␛[97m                   ␛[39;49m
    ␛[97;40m    ␛[95m┃␛[97m      ␛[93m┌────┼─────────␛[95m┨␛[97m             ␛[96m│␛[97m      ␛[92;100m╔════╗␛[93;40m─────────┐␛[97m         ␛[39;49m
    ␛[97;40m    ␛[95m┃␛[97m      ␛[93m│␛[97m    ␛[96m│␛[97m ␛[93mB␛[97m       ␛[95m┃␛[97m             ␛[96m│␛[97m      ␛[92;100m║␛[39m    ␛[92m║␛[97;40m ␛[93mB␛[97m       ␛[93m│␛[97m         ␛[39;49m
    ␛[97;40m    ␛[95m┠␛[96m──────␛[93m┼␛[96m────┘␛[97m         ␛[95m┃␛[97m             ␛[96m└──────␛[92;100m╚════╝␛[97;40m         ␛[93m│␛[97m         ␛[39;49m
    ␛[97;40m    ␛[95m┃␛[97m      ␛[93m│␛[97m              ␛[95m┃␛[97m                    ␛[93m│␛[97m              ␛[93m│␛[97m         ␛[39;49m
    ␛[97;40m    ␛[95m┗━━━━━━┷━━━━━━━━━━━━━━┛␛[97m                    ␛[93m└──────────────┘␛[97m         ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m               ␛[90munion␛[97m                            ␛[90mintersection␛[97m            ␛[39;49m

Aligning Smaller Content and Cropping Larger Sources
----------------------------------------------------

There are two closely related alignment tools:

* ``BlockRectangle::alignmentOffset()``
  computes where a smaller block should start.
* ``BlockRectangle::alignedSource()``
  returns an ``AlignedSource``
  with both the effective target and the cropped source rectangle.

.. code-block:: cpp

    const auto badgeRect = BlockRectangle{
        panel.alignmentOffset(BlockSize{8, 3}, Alignment::BottomRight),
        BlockSize{8, 3}};
    buffer.drawFrame(badgeRect, FrameStyle::Double, Color{fg::BrightCyan, bg::Inherited});

    const auto aligned = panel.insetBy(BlockMargins{1}).alignedSource(
        BlockRectangle{0, 0, 18, 5},
        Alignment::Center);

    aligned.targetRect.forEach([&](const BlockPosition pos) {
        const auto sourceX = aligned.sourceRect.x1() + (pos.x() - aligned.targetRect.x1());
        const auto sourceY = aligned.sourceRect.y1() + (pos.y() - aligned.targetRect.y1());
        // Sample from the centered source area here.
    });

This saves you from writing separate "place when smaller" and "crop when larger" branches.
The same alignment value handles both cases.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97;40m alignmentOffset() places smaller content; alignedSource() crops larger ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m  ␛[96m╔══════╦␛[97m───────────┐    ┌──────────────────┐    ┌──────────────────┐  ␛[39;49m
    ␛[97;40m  ␛[96m║␛[97;44m 8x3  ␛[96;40m║␛[97m           │    │     ␛[96m╔══════╗␛[97m     │    │                  │  ␛[39;49m
    ␛[97;40m  ␛[96m╞══════╝␛[97m           │    │     ␛[96m║␛[97;44m 8x3  ␛[96;40m║␛[97m     │    │           ␛[96m╔══════╡␛[97m  ␛[39;49m
    ␛[97;40m  │                  │    │     ␛[96m╚══════╝␛[97m     │    │           ␛[96m║␛[97;44m 8x3  ␛[96;40m║␛[97m  ␛[39;49m
    ␛[97;40m  └──────────────────┘    └──────────────────┘    └───────────␛[96m╩══════╝␛[97m  ␛[39;49m
    ␛[97;40m        ␛[90mTopLeft␛[97m                  ␛[90mCenter␛[97m               ␛[90mBottomRight␛[97m       ␛[39;49m
    ␛[97;40m                ╔══════════════════════════════════════╗                ␛[39;49m
    ␛[97;40m                ║          ␛[93;100m234567890123456789␛[97;40m          ║                ␛[39;49m
    ␛[97;40m                ║          ␛[93;100m456789012345678901␛[97;40m          ║                ␛[39;49m
    ␛[97;40m                ║          ␛[93;100m678901234567890123␛[97;40m          ║                ␛[39;49m
    ␛[97;40m                ╚══════════════════════════════════════╝                ␛[39;49m
    ␛[97;40m                       ␛[90mcenter crop of 18x5 source␛[97m                       ␛[39;49m

Splitting a BlockRectangle into Grid Cells
------------------------------------------

``BlockRectangle::gridCells()`` divides a larger canvas into evenly spaced sub-rectangles.
This is useful for dashboards, menu grids, option panels, and any row-major layout where each cell should stay
predictable.

.. code-block:: cpp

    const auto grid = BlockRectangle{2, 2, 68, 8};
    const auto cells = grid.gridCells(2, 3, 2, 1);

    for (std::size_t index = 0; index < cells.size(); ++index) {
        const auto &cell = cells[index];
        buffer.drawFrame(cell, FrameStyle::Light, Color{fg::BrightWhite, bg::Inherited});
        buffer.drawBlockText(
            el::StringFormat{"#{}  {}x{}"_el}.build(index, cell.width(), cell.height()),
            cell,
            Alignment::Center);
    }

Remainder pixels are distributed to the top-left cells first, and the resulting vector is returned in row-major order
from left to right, then top to bottom.

If the requested number of rows, columns, and spacing no longer fits into the rectangle, ``gridCells()`` throws
``std::invalid_argument``.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97;40m   gridCells() distributes remainder to the top-left cells and keeps    ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m  ┌────────────────────┐  ┌───────────────────┐  ┌───────────────────┐  ␛[39;49m
    ␛[97;40m  │␛[44m      #0 22x4       ␛[40m│  │␛[46m      #1 21x4      ␛[40m│  │␛[45m      #2 21x4      ␛[40m│  ␛[39;49m
    ␛[97;40m  │␛[44m                    ␛[40m│  │␛[46m                   ␛[40m│  │␛[45m                   ␛[40m│  ␛[39;49m
    ␛[97;40m  └────────────────────┘  └───────────────────┘  └───────────────────┘  ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m  ┌────────────────────┐  ┌───────────────────┐  ┌───────────────────┐  ␛[39;49m
    ␛[97;40m  │␛[42m      #3 22x3       ␛[40m│  │␛[41m      #4 21x3      ␛[40m│  │␛[100m      #5 21x3      ␛[40m│  ␛[39;49m
    ␛[97;40m  └────────────────────┘  └───────────────────┘  └───────────────────┘  ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m

Walking Neighbors and Frame Perimeters
--------------------------------------

``BlockPosition`` and ``BlockRectangle`` include traversal helpers that are useful for custom layout logic, collision
checks, and procedural drawing.

.. code-block:: cpp

    const auto center = BlockPosition{12, 5};
    for (const auto pos : center.cardinalFour()) {
        buffer.set(pos, Block{U'+', Color{fg::BrightYellow, bg::Inherited}});
    }
    for (const auto pos : center.ringEight()) {
        // ringEight() returns the eight surrounding positions clockwise.
    }

    const auto frame = BlockRectangle{46, 2, 18, 7};
    frame.forEachInFrame([&](const BlockPosition pos, const int index) {
        buffer.set(
            pos,
            Block{static_cast<char32_t>(U'0' + (index % 10)), Color{fg::BrightWhite, bg::Inherited}});
    });

    const auto currentIndex = frame.frameIndex(BlockPosition{63, 5});

The ``BlockDirection`` wrapper fits into the same workflow when directions come from configuration or input, because it
can convert to and from deltas and strings.

.. code-block:: cpp

    auto cursor = BlockPosition{10, 5};
    cursor += BlockDirection::fromString("east").toDelta();

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97;40m      BlockPosition::ringEight()              BlockRectangle::forEachInFrame()    ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m                                              ␛[49m012345678901234567␛[40m        ␛[39;49m
    ␛[97;40m                                              ␛[49m5␛[40m                ␛[49m8␛[40m        ␛[39;49m
    ␛[97;40m           ␛[96;49m5␛[93m+␛[96m7␛[97;40m                                ␛[49m4␛[40m                ␛[49m9␛[40m        ␛[39;49m
    ␛[97;40m           ␛[93;49m+␛[97mX␛[93m+␛[97;40m                                ␛[49m3␛[40m                ␛[49m0␛[40m        ␛[39;49m
    ␛[97;40m           ␛[96;49m3␛[93m+␛[96m1␛[97;40m                                ␛[49m2␛[40m                ␛[49m1␛[40m        ␛[39;49m
    ␛[97;40m                                              ␛[49m1␛[40m                ␛[49m2␛[40m        ␛[39;49m
    ␛[97;40m                                              ␛[49m098765432109876543␛[40m        ␛[39;49m
    ␛[97;40m     ␛[90m0..7 clockwise around X␛[97m               ␛[90mindex order around frame␛[97m     ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m

Deriving Bounds and Keeping Cursors in Range
--------------------------------------------

Two other helpers are worth using regularly:

* ``BlockRectangle::bounds()``
  creates the smallest rectangle that contains a set of positions.
* ``BlockRectangle::clamp()``
  keeps a cursor or probe position inside a rectangle.

.. code-block:: cpp

    const auto points = PositionList{
        BlockPosition{5, 5},
        BlockPosition{9, 3},
        BlockPosition{13, 6},
        BlockPosition{17, 4},
        BlockPosition{11, 7},
    };
    const auto highlight = BlockRectangle::bounds(points).expandedBy(BlockMargins{1});

    const auto viewport = BlockRectangle{46, 2, 16, 6};
    const auto rawCursor = BlockPosition{66, 8};
    const auto safeCursor = viewport.clamp(rawCursor);

    if (viewport.contains(safeCursor)) {
        buffer.set(safeCursor, Block{"@", Color{fg::BrightGreen, bg::Inherited}});
    }

This pattern works well for drag selections, hit testing, and cursor movement that should stop cleanly at the viewport
boundary instead of spilling outside the drawable area.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97;40m       BlockRectangle::bounds()                    BlockRectangle::clamp()        ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m    ␛[95m╔═════════════╗␛[97m                           ␛[96m┌───␛[90mviewport␛[96m───┐␛[97m          ␛[39;49m
    ␛[97;40m    ␛[95m║␛[97m    ␛[93;49m•␛[97;40m        ␛[95m║␛[97m                           ␛[96m│␛[97m              ␛[96m│␛[97m          ␛[39;49m
    ␛[97;40m    ␛[95m║␛[97m            ␛[93;49m•␛[95;40m║␛[97m                           ␛[96m│␛[97m              ␛[96m│␛[97m          ␛[39;49m
    ␛[97;40m    ␛[95m║␛[93;49m•␛[97;40m            ␛[95m║␛[97m                           ␛[96m│␛[97m              ␛[96m│␛[97m          ␛[39;49m
    ␛[97;40m    ␛[95m║␛[97m        ␛[93;49m•␛[97;40m    ␛[95m║␛[97m                           ␛[96m│␛[97m              ␛[96m│␛[97m          ␛[39;49m
    ␛[97;40m    ␛[95m║␛[97m      ␛[93;49m•␛[97;40m      ␛[95m║␛[97m                           ␛[96m└──────────────␛[92;49m@␛[97;40m          ␛[39;49m
    ␛[97;40m    ␛[95m╚═════════════╝␛[97m                                           ␛[91mraw␛[97m ␛[91;49mX␛[97;40m     ␛[39;49m
    ␛[97;40m  ␛[90mderive one enclosing rectangle␛[97m           ␛[90mclamped to nearest edge␛[97m      ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m

Orientation Helpers and BlockCoordinate Values
----------------------------------------------

``Orientation`` and ``BlockCoordinate`` are the small glue types that keep geometry code readable when logic needs to
switch between horizontal and vertical behavior.

.. code-block:: cpp

    auto axis = Orientation::Horizontal;
    auto size = BlockSize{24, 7};
    auto cursor = BlockPosition{4, 2};

    const auto primaryExtent = size.coordinate(axis);
    const auto primaryOffset = cursor.coordinate(axis);
    const auto crossAxis = axis.crossed();

    if (crossAxis == Orientation::Vertical) {
        buffer.drawBlockText(
            el::StringFormat{"axis={} offset={}"_el}.build(primaryExtent, primaryOffset),
            BlockRectangle{2, 2, 24, 1},
            Alignment::Left);
    }

The ``coordinate()`` helpers on ``BlockSize`` and ``BlockPosition`` let one code path serve both axes without branching
on ``x/y`` or ``width/height`` names all the way through the implementation.
