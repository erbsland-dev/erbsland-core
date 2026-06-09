..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**********************
Frames, Borders, Tiles
**********************

The drawing types define how frames, borders, filled areas, and character combinations are rendered into writable
buffers.

They form the foundation for panels, boxes, separators, decorative borders, tiled backgrounds, and line-art style
layouts.

.. dropdown:: Details about the example output on this page

    The examples below were rendered with the dedicated documentation helper
    :file:`doc/tools/drawing-reference.cpp` at a fixed width of
    70 terminal columns. This makes it easy to regenerate the visual output
    together with the code snippets.

Usage
=====

Filling Panels and Drawing Simple Frames
----------------------------------------

``WritableBuffer`` provides convenient functions to fill rectangles and draw frames.
The drawing types on this page determine how those operations appear.

.. code-block:: cpp

    const auto panel = BlockRectangle{2, 2, 30, 10};

    buffer.fill(panel, Block{" ", Color{fg::Inherited, bg::Blue}});
    buffer.drawFrame(panel, FrameStyle::LightWithRoundedCorners, Color{fg::BrightWhite, bg::Blue});
    buffer.drawBlockText("Overview", panel.insetBy(BlockMargins{1}), Alignment::TopLeft);

``FrameStyle`` is the fastest way to choose one of the built-in Unicode box styles.
It works well for common UI elements such as panels, dialogs, and separators.

You can restyle the same layout code simply by changing the enum value:

.. code-block:: cpp

    buffer.drawFilledFrame(
        BlockRectangle{2, 4, 20, 4},
        FrameStyle::Light,
        Block{" ", Color{fg::Inherited, bg::Blue}},
        Color{fg::BrightCyan, bg::Inherited});
    buffer.drawFilledFrame(
        BlockRectangle{25, 4, 20, 4},
        FrameStyle::LightWithRoundedCorners,
        Block{" ", Color{fg::Inherited, bg::Green}},
        Color{fg::BrightGreen, bg::Inherited});
    buffer.drawFilledFrame(
        BlockRectangle{48, 9, 20, 4},
        FrameStyle::FullBlock,
        Block{" ", Color{fg::Inherited, bg::BrightBlack}});

.. erbsland-ansi::
    :escape-char: ␛

    ␛[39m␛[97;40m                     Built-in FrameStyle presets                      ␛[39;49m
    ␛[97;40m      ␛[90mChoose a built-in style without changing your layout code.␛[97m      ␛[39;49m
    ␛[97;40m                                                                      ␛[39;49m
    ␛[97;40m         Light                 Rounded                 Heavy          ␛[39;49m
    ␛[97;40m  ␛[96m┌──────────────────┐␛[97m   ␛[92m╭──────────────────╮␛[97m   ␛[93m┏━━━━━━━━━━━━━━━━━━┓␛[97m  ␛[39;49m
    ␛[97;40m  ␛[96m│␛[97;44m      panel       ␛[96;40m│␛[97m   ␛[92m│␛[97;42m      panel       ␛[92;40m│␛[97m   ␛[93m┃␛[97;41m      panel       ␛[93;40m┃␛[97m  ␛[39;49m
    ␛[97;40m  ␛[96m│␛[97;44m                  ␛[96;40m│␛[97m   ␛[92m│␛[97;42m                  ␛[92;40m│␛[97m   ␛[93m┃␛[97;41m                  ␛[93;40m┃␛[97m  ␛[39;49m
    ␛[97;40m  ␛[96m└──────────────────┘␛[97m   ␛[92m╰──────────────────╯␛[97m   ␛[93m┗━━━━━━━━━━━━━━━━━━┛␛[97m  ␛[39;49m
    ␛[97;40m         Double               OuterHalf              FullBlock        ␛[39;49m
    ␛[97;40m  ␛[95m╔══════════════════╗␛[97m   ␛[94m▛▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▜␛[97m   ████████████████████  ␛[39;49m
    ␛[97;40m  ␛[95m║␛[97;45m      panel       ␛[95;40m║␛[97m   ␛[94m▌␛[97;46m      panel       ␛[94;40m▐␛[97m   █␛[100m      panel       ␛[40m█  ␛[39;49m
    ␛[97;40m  ␛[95m║␛[97;45m                  ␛[95;40m║␛[97m   ␛[94m▌␛[97;46m                  ␛[94;40m▐␛[97m   █␛[100m                  ␛[40m█  ␛[39;49m
    ␛[97;40m  ␛[95m╚══════════════════╝␛[97m   ␛[94m▙▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▟␛[97m   ████████████████████  ␛[39;49m
    ␛[97;40m                                                                      ␛[39;49m

Combining Frames Automatically
------------------------------

When multiple frames intersect, drawing them directly on top of each other would produce broken or overlapping
characters.

``BlockCombinationStyle`` resolves these overlaps by choosing the correct combined glyph for each intersection.

.. code-block:: cpp

    auto frameStyle = Block16Style::lightFrame();
    auto combination = BlockCombinationStyle::commonBoxFrame();

    buffer.drawFrame(BlockRectangle{2, 2, 18, 8}, frameStyle, combination, Color{fg::BrightCyan, bg::Black});
    buffer.drawFrame(BlockRectangle{8, 5, 20, 8}, frameStyle, combination, Color{fg::BrightYellow, bg::Black});

This ensures that intersecting lines produce consistent box-drawing characters instead of visual artifacts.

Drawing Grid Layouts
--------------------

Use ``GridLayout`` when you want a stable table-like layout where content cells keep their own rectangles and the frame
lines are drawn around them.

The ``FrameBorder`` object describes which outer and inner line groups are visible.
A ``None`` element consumes no terminal cells, while supported line styles consume one cell and are resolved with their
neighbors automatically.
Block frame styles are not grid line styles and are treated like ``None`` here.

.. code-block:: cpp

    auto border = FrameBorder{FrameStyle::Light, Color{fg::BrightWhite, bg::Black}};
    border.set(FrameBorderElement::HLine, FrameStyle::Heavy, Color{fg::BrightWhite, bg::Black});

    auto layout = GridLayout{{16, 16, 16}, {3, 3}};
    auto origin = BlockPosition{2, 2};

    for (std::size_t row = 0; row < layout.rowCount(); ++row) {
        for (std::size_t column = 0; column < layout.columnCount(); ++column) {
            buffer.drawBlockText(
                "cell",
                layout.cellRect(row, column, origin, border),
                Alignment::Center,
                Color{fg::BrightCyan, bg::Black});
        }
    }
    buffer.drawGridLayout(origin, layout, border);

This keeps content placement independent from the selected border style.
If you later hide separator lines or switch from light to double borders, the same ``cellRect()`` calls still describe
the usable content area.

The difference is easiest to see when the same pair of frames is rendered once with plain overwrite behavior and once
with a box-aware combiner:

.. code-block:: cpp

    auto style = Block16Style::lightFrame();

    buffer.drawFrame(
        BlockRectangle{2, 2, 17, 6},
        style,
        BlockCombinationStyle::overwrite(),
        Color{fg::BrightCyan, bg::Inherited});
    buffer.drawFrame(
        BlockRectangle{8, 4, 18, 6},
        style,
        BlockCombinationStyle::overwrite(),
        Color{fg::BrightYellow, bg::Inherited});

    buffer.drawFrame(
        BlockRectangle{37, 2, 17, 6},
        style,
        BlockCombinationStyle::commonBoxFrame(),
        Color{fg::BrightCyan, bg::Inherited});
    buffer.drawFrame(
        BlockRectangle{43, 4, 18, 6},
        style,
        BlockCombinationStyle::commonBoxFrame(),
        Color{fg::BrightYellow, bg::Inherited});

.. erbsland-ansi::
    :escape-char: ␛

    ␛[39m␛[97;40m            overwrite()                     commonBoxFrame()          ␛[39;49m
    ␛[90;40mLater frames simply replace earlier␛[97m   ␛[90mIntersections become matching␛[97m   ␛[39;49m
    ␛[97;40m  ␛[96;49m┌───────────────┐␛[97;40m                  ␛[96m┌───────────────┐␛[97m                ␛[39;49m
    ␛[97;40m  ␛[96;49m│␛[97;40m               ␛[96;49m│␛[97;40m                  ␛[96m│␛[97m               ␛[96m│␛[97m                ␛[39;49m
    ␛[97;40m  ␛[96;49m│␛[97;40m     ␛[93;49m┌────────────────┐␛[97;40m           ␛[96m│␛[97m     ␛[93m┌─────────┼──────┐␛[97m         ␛[39;49m
    ␛[97;40m  ␛[96;49m│␛[97;40m     ␛[93;49m│␛[97;40m         ␛[96;49m│␛[97;40m      ␛[93;49m│␛[97;40m           ␛[96m│␛[97m     ␛[93m│␛[97m         ␛[96m│␛[97m      ␛[93m│␛[97m         ␛[39;49m
    ␛[97;40m  ␛[96;49m│␛[97;40m     ␛[93;49m│␛[97;40m         ␛[96;49m│␛[97;40m      ␛[93;49m│␛[97;40m           ␛[96m│␛[97m     ␛[93m│␛[97m         ␛[96m│␛[97m      ␛[93m│␛[97m         ␛[39;49m
    ␛[97;40m  ␛[96;49m└─────␛[93m│␛[96m─────────┘␛[97;40m      ␛[93;49m│␛[97;40m           ␛[96m└─────␛[93m┼␛[96m─────────┘␛[97m      ␛[93m│␛[97m         ␛[39;49m
    ␛[97;40m        ␛[93;49m│␛[97;40m                ␛[93;49m│␛[97;40m                 ␛[93m│␛[97m                ␛[93m│␛[97m         ␛[39;49m
    ␛[97;40m        ␛[93;49m└────────────────┘␛[97;40m                 ␛[93m└────────────────┘␛[97m         ␛[39;49m
    ␛[97;40m                                                                      ␛[39;49m

To define your own combination behavior, use ``SimpleBlockCombinationStyle`` for straightforward mappings or
``MatrixCombinationStyle`` for full control over all combinations.

Repeating 9-Tile Styles
-----------------------

``Tile9Style`` fills or frames a rectangle using a repeating 3×3 tile layout.

This lets you define separate characters for corners, edges, and the center while still scaling cleanly to any size.

.. code-block:: cpp

    const auto panel = BlockRectangle{2, 2, 30, 10};
    const auto style = Tile9Style::create("╔═╗║ ║╚═╝");

    buffer.fill(panel, style, Color{fg::BrightBlack, bg::Black});
    buffer.drawFrame(panel, style, Color{fg::BrightCyan, bg::Black});

The 9-tile layout covers the standard case: corners, edges, and center.
You can also read a configured tile directly by name, for example ``style->block(Tile9Style::Element::West)`` or
``style->block(Tile9Style::Element::Center)``.
This is useful for small theme-controlled text fragments that reuse the same tile table without drawing a rectangle.

If you also need specialized tiles for degenerate cases such as a single row, a single column, or a single cell,
construct the style with 16 characters instead.

The left example below uses a decorative 9-tile pattern.
The right side uses letter-coded 16-tile input so the special degenerate tiles are easy to identify:

.. code-block:: cpp

    const auto decorative = Tile9Style::create("╔═╗║·║╚═╝");
    buffer.fill(BlockRectangle{3, 2, 28, 7}, decorative, Color{fg::BrightCyan, bg::Inherited});

    const auto degenerate = Tile9Style::create("ABCDEFGHIJKLMNOP");
    buffer.fill(BlockRectangle{39, 2, 10, 5}, degenerate, Color{fg::BrightYellow, bg::Inherited});
    buffer.fill(BlockRectangle{52, 2, 10, 1}, degenerate, Color{fg::BrightYellow, bg::Inherited});
    buffer.fill(BlockRectangle{52, 4, 1, 5}, degenerate, Color{fg::BrightYellow, bg::Inherited});
    buffer.fill(BlockRectangle{57, 4, 1, 1}, degenerate, Color{fg::BrightYellow, bg::Inherited});

.. erbsland-ansi::
    :escape-char: ␛

    ␛[39m␛[97;40m  9 tiles repeat edges and center    16 tiles cover thin edge cases   ␛[39;49m
    ␛[97;40m                                                       ␛[90mrow␛[97m            ␛[39;49m
    ␛[97;40m   ␛[96;49m╔══════════════════════════╗␛[97;40m        ␛[93;49mABBBBBBBBC␛[97;40m   ␛[93;49mJKKKKKKKKL␛[97;40m        ␛[39;49m
    ␛[97;40m   ␛[96;49m║··························║␛[97;40m        ␛[93;49mDEEEEEEEEF␛[97;40m                     ␛[39;49m
    ␛[97;40m   ␛[96;49m║··········␛[97mRepeat␛[96m··········║␛[97;40m        ␛[93;49mDEEEEEEEEF␛[97;40m   ␛[93;49mM␛[97;40m    ␛[93;49mP␛[97;40m ␛[90mcell␛[97m       ␛[39;49m
    ␛[97;40m   ␛[96;49m║··························║␛[97;40m        ␛[93;49mDEEEEEEEEF␛[97;40m   ␛[93;49mN␛[97;40m                 ␛[39;49m
    ␛[97;40m   ␛[96;49m║··························║␛[97;40m        ␛[93;49mGHHHHHHHHI␛[97;40m   ␛[93;49mN␛[97;40m                 ␛[39;49m
    ␛[97;40m   ␛[96;49m║··························║␛[97;40m                     ␛[93;49mN␛[97;40m                 ␛[39;49m
    ␛[97;40m   ␛[96;49m╚══════════════════════════╝␛[97;40m          ␛[90mnormal␛[97m     ␛[93;49mO␛[97;40m                 ␛[39;49m
    ␛[97;40m                                                  ␛[90mcolumn␛[97m              ␛[39;49m
    ␛[97;40m                                                                      ␛[39;49m
    ␛[97;40m                                                                      ␛[39;49m

Configuring Reusable Frame Presets
----------------------------------

When a frame configuration goes beyond a simple style and color, ``FrameDrawOptions`` lets you bundle all settings into
a reusable object.

.. code-block:: cpp

    auto panelStyle = FrameDrawOptions{};
    panelStyle.setStyle(FrameStyle::LightWithRoundedCorners);
    panelStyle.setFillBlock(Block{" ", Color{fg::Inherited, bg::Black}});
    panelStyle.setFrameColorSequence(
        ColorSequence{
            Color{fg::BrightBlue, bg::Black},
            Color{fg::BrightCyan, bg::Black},
            Color{fg::BrightWhite, bg::Black},
        },
        FrameColorMode::ChasingBorderCW);
    panelStyle.setFillColor(Color{fg::Inherited, bg::Blue});

    buffer.drawFrame(BlockRectangle{2, 2, 28, 10}, panelStyle, animationCycle);
    buffer.drawFrame(BlockRectangle{34, 2, 28, 10}, panelStyle, animationCycle + 4);

This approach is especially useful when the same panel style is reused throughout your application, or when you work
with animated borders, custom fill behavior, or non-default combination rules.

Because ``FrameDrawOptions`` bundles style, fill, and color animation in one object, it is also a good fit for reusable
presets:

.. code-block:: cpp

    auto stripeOptions = FrameDrawOptions{};
    stripeOptions.setStyle(FrameStyle::Double);
    stripeOptions.setFillBlock(Block{" "});
    stripeOptions.setFrameColorSequence(
        ColorSequence{
            Color{fg::BrightYellow, bg::Inherited},
            Color{fg::BrightMagenta, bg::Inherited},
            Color{fg::BrightCyan, bg::Inherited},
        },
        FrameColorMode::VerticalStripes);
    stripeOptions.setFillColorSequence(
        ColorSequence{
            Color{fg::Inherited, bg::Blue},
            Color{fg::Inherited, bg::Magenta},
        },
        FrameColorMode::HorizontalStripes);

    auto chasingOptions = FrameDrawOptions{};
    chasingOptions.setStyle(FrameStyle::Heavy);
    chasingOptions.setFillBlock(Block{"·", Color{fg::BrightBlack, bg::Inherited}});
    chasingOptions.setFrameColorSequence(
        ColorSequence{
            Color{fg::BrightRed, bg::Inherited},
            Color{fg::BrightYellow, bg::Inherited},
            Color{fg::BrightGreen, bg::Inherited},
            Color{fg::BrightCyan, bg::Inherited},
        },
        FrameColorMode::ChasingBorderCW);

    buffer.drawFrame(BlockRectangle{24, 1, 22, 8}, stripeOptions);
    buffer.drawFrame(BlockRectangle{47, 1, 22, 8}, chasingOptions, 3);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[39m␛[97;40m      Static fill         Striped sequences        Chasing border     ␛[39;49m
    ␛[97;40m ␛[96m╭────────────────────╮␛[97m ␛[93m╔␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m╗␛[97m ␛[93m┏␛[92m━␛[96m━␛[91m━␛[93m━␛[92m━␛[96m━␛[91m━␛[93m━␛[92m━␛[96m━␛[91m━␛[93m━␛[92m━␛[96m━␛[91m━␛[93m━␛[92m━␛[96m━␛[91m━␛[93m━␛[92m┓␛[97m ␛[39;49m
    ␛[97;40m ␛[96m│␛[44m                    ␛[40m│␛[97m ␛[93m║␛[95;45m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93;40m║␛[97m ␛[91m┃␛[90;100m····················␛[96;40m┃␛[97m ␛[39;49m
    ␛[97;40m ␛[96m│␛[44m                    ␛[40m│␛[97m ␛[93m║␛[95;44m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93;40m║␛[97m ␛[96m┃␛[90;100m····················␛[91;40m┃␛[97m ␛[39;49m
    ␛[97;40m ␛[96m│␛[44m                    ␛[40m│␛[97m ␛[93m║␛[95;45m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93;40m║␛[97m ␛[92m┃␛[90;100m····················␛[93;40m┃␛[97m ␛[39;49m
    ␛[97;40m ␛[96m│␛[44m                    ␛[40m│␛[97m ␛[93m║␛[95;44m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93;40m║␛[97m ␛[93m┃␛[90;100m····················␛[92;40m┃␛[97m ␛[39;49m
    ␛[97;40m ␛[96m│␛[44m                    ␛[40m│␛[97m ␛[93m║␛[95;45m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93;40m║␛[97m ␛[91m┃␛[90;100m····················␛[96;40m┃␛[97m ␛[39;49m
    ␛[97;40m ␛[96m│␛[44m                    ␛[40m│␛[97m ␛[93m║␛[95;44m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93m ␛[95m ␛[96m ␛[93;40m║␛[97m ␛[96m┃␛[90;100m····················␛[91;40m┃␛[97m ␛[39;49m
    ␛[97;40m ␛[96m╰────────────────────╯␛[97m ␛[93m╚␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m═␛[95m═␛[96m═␛[93m╝␛[97m ␛[92m┗␛[93m━␛[91m━␛[96m━␛[92m━␛[93m━␛[91m━␛[96m━␛[92m━␛[93m━␛[91m━␛[96m━␛[92m━␛[93m━␛[91m━␛[96m━␛[92m━␛[93m━␛[91m━␛[96m━␛[92m━␛[93m┛␛[97m ␛[39;49m
    ␛[97;40m      ␛[90mfixed colors␛[97m           ␛[90mstripe modes␛[97m            ␛[90mcycle = 3␛[97m        ␛[39;49m
    ␛[97;40m                                                                      ␛[39;49m

Choosing the Right Style
------------------------

Each drawing style has a slightly different purpose:

* Use ``FrameStyle``
  when a predefined border style is enough.

* Use ``Block16Style``
  when each cell should adapt to its neighbors, for example for custom
  line art or circuit-like rendering.

* Use ``Tile9Style``
  when corners, edges, and center tiles should scale independently.

Choosing the right abstraction early keeps your rendering code simpler and avoids unnecessary post-processing later.

