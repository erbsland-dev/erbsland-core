..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*******************
BlockText Rendering
*******************

The text rendering classes describe how terminal text is drawn into a writable buffer.
They turn ``BlockString`` content into aligned labels, wrapped paragraphs, reusable text presets and animated
headlines.

Bitmap fonts plug into the same pipeline through ``BlockTextOptions`` and ``BlockText``.
For font creation and font presets, see :doc:`font`.

This page focuses on placing text inside rectangles, rendering wrapped text blocks, and reusing ``BlockTextOptions``
across multiple ``BlockText`` instances.
For the underlying ``Block``, ``BlockString`` and ``BlockStringEditor`` value types, see
:doc:`text`.

.. dropdown:: Details about the example output on this page

    The examples below were rendered with the dedicated documentation
    helper :file:`doc/tools/drawing-text-reference.cpp` at a fixed width
    of 72 terminal columns. This makes it easy to regenerate the visual
    output together with the code snippets.

Usage
=====

Drawing Direct Strings and Aligned Labels
-----------------------------------------

``WritableBuffer`` provides two main entry points for text that does not need paragraph formatting:

* ``drawBlockText(BlockPosition, BlockString)`` writes text at an exact start position.
* ``drawBlockText(BlockString, BlockRectangle, Alignment, Color)`` aligns text inside a
  rectangle.

.. code-block:: cpp

    auto footer = BlockStringEditor{};
    footer.append(
        bg::BrightBlack,
        fg::BrightYellow,
        "[Q]",
        fg::BrightWhite,
        " quit  ",
        fg::BrightCyan,
        "[R]",
        fg::BrightWhite,
        " refresh");

    buffer.drawBlockText(BlockPosition{4, 4}, footer);

    buffer.drawFilledFrame(
        BlockRectangle{42, 2, 24, 5},
        FrameStyle::LightWithRoundedCorners,
        Block{" ", Color{fg::Inherited, bg::Blue}},
        Color{fg::BrightCyan, bg::Inherited});
    buffer.drawBlockText("Overview", BlockRectangle{42, 2, 24, 5}, Alignment::Center, Color{fg::BrightWhite, bg::Inherited});

Use the position-based overload for status lines, overlays, and other exact placements.
Use the rectangle overload for titles, centered labels, and other layout-driven text.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97;40m    drawBlockText(BlockPosition, BlockString)         drawBlockText(BlockString, BlockRectangle, ...) ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m  ␛[90m012345678901234567890123456789␛[97m          ␛[96m╭──────────────────────╮␛[97m      ␛[39;49m
    ␛[97;40m                                          ␛[96m│␛[97;44m                      ␛[96;40m│␛[97m      ␛[39;49m
    ␛[97;40m    ␛[93;100m[Q]␛[97m quit  ␛[96m[R]␛[97m refresh␛[40m                 ␛[96m│␛[97;44m       Overview       ␛[96;40m│␛[97m      ␛[39;49m
    ␛[97;40m                                          ␛[96m│␛[97;44m                      ␛[96;40m│␛[97m      ␛[39;49m
    ␛[97;40m                                          ␛[96m╰──────────────────────╯␛[97m      ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m   ␛[90mexact position, mixed colors␛[97m        ␛[90msame API, aligned in a rectangle␛[97m ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m

BlockText Alignment Inside a BlockRectangle
-------------------------------------------

``BlockText`` uses the same ``Alignment`` model as the geometry and bitmap helpers.
This means you can place a text block inside its target rectangle without manual offset calculations.

.. code-block:: cpp

    auto title = BlockText{BlockString{"Short note"}, BlockRectangle{26, 2, 20, 4}, Alignment::Center};
    title.setColor(Color{fg::BrightYellow, bg::Inherited});
    buffer.drawBlockText(title);

The same content can be anchored to the top-left, center, or bottom-right simply by changing the alignment value.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97;40m   BlockText uses the same alignment model as other geometry-aware drawing   ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m  ┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐  ␛[39;49m
    ␛[97;40m  │␛[93;100mShort note␛[97m        ␛[40m│    │␛[100m    ␛[93mShort note␛[97m    ␛[40m│    │␛[100m                  ␛[40m│  ␛[39;49m
    ␛[97;40m  │␛[100m                  ␛[40m│    │␛[100m                  ␛[40m│    │␛[100m        ␛[93mShort note␛[97;40m│  ␛[39;49m
    ␛[97;40m  └──────────────────┘    └──────────────────┘    └──────────────────┘  ␛[39;49m
    ␛[97;40m        ␛[90mTopLeft␛[97m                  ␛[90mCenter␛[97m               ␛[90mBottomRight␛[97m       ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m

Rendering Wrapped Paragraphs
----------------------------

``BlockText`` becomes especially useful when text needs wrapping, paragraph spacing, or paragraph-aware indentation.

.. code-block:: cpp

    auto help = BlockText{
        BlockString{"Navigation: Use arrow keys to move.\n\nActions: Press Enter to open."},
        BlockRectangle{10, 3, 52, 7},
        Alignment::TopLeft};
    help.setParagraphSpacing(ParagraphSpacing::DoubleLine);
    help.setWrappedLineIndent(2);
    help.setColor(Color{fg::BrightWhite, bg::Inherited});

    buffer.drawBlockText(help);

This keeps the content, target rectangle, and paragraph behavior in one object.
For the full paragraph-formatting reference, continue with
:doc:`paragraph-options`.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m      ╔════════════════════════␛[44mHelp Panel␛[40m════════════════════════╗      ␛[39;49m
    ␛[97;40m      ║␛[44m                                                          ␛[40m║      ␛[39;49m
    ␛[97;40m      ║␛[44m   Navigation: Use arrow keys to move.                    ␛[40m║      ␛[39;49m
    ␛[97;40m      ║␛[44m                                                          ␛[40m║      ␛[39;49m
    ␛[97;40m      ║␛[44m                                                          ␛[40m║      ␛[39;49m
    ␛[97;40m      ║␛[44m                                                          ␛[40m║      ␛[39;49m
    ␛[97;40m      ║␛[44m   Actions: Press Enter to open.                          ␛[40m║      ␛[39;49m
    ␛[97;40m      ║␛[44m                                                          ␛[40m║      ␛[39;49m
    ␛[97;40m      ║␛[44m                                                          ␛[40m║      ␛[39;49m
    ␛[97;40m      ║␛[44m                                                          ␛[40m║      ␛[39;49m
    ␛[97;40m      ╚══════════════════════════════════════════════════════════╝      ␛[39;49m
    ␛[97;40m     ␛[90mText automatically wraps and keeps explicit paragraph breaks.␛[97m      ␛[39;49m

Reusing BlockTextOptions and Animation
--------------------------------------

``BlockTextOptions`` bundles color sequences, fonts, animation, and paragraph layout settings into one reusable preset.

.. code-block:: cpp

    auto options = BlockTextOptions{Alignment::Center};
    options.setColorSequence(
        ColorSequence{
            Color{fg::BrightYellow, bg::Inherited},
            Color{fg::BrightRed, bg::Inherited},
            Color{fg::BrightMagenta, bg::Inherited},
            Color{fg::BrightCyan, bg::Inherited},
        });
    options.setAnimation(BlockTextAnimation::ColorDiagonal);

    auto left = BlockText{BlockString{"ALERT PANEL"}, BlockRectangle{4, 3, 28, 2}, Alignment::Center};
    left.setBlockTextOptions(options);
    auto right = BlockText{BlockString{"ALERT PANEL"}, BlockRectangle{40, 3, 28, 2}, Alignment::Center};
    right.setBlockTextOptions(options);

    buffer.drawBlockText(left, 1);
    buffer.drawBlockText(right, 3);

This works well when several labels or headings should share one visual style.
``BlockTextAnimation::ColorDiagonal`` shifts the configured color sequence diagonally across the rendered text based on
the supplied animation cycle.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97;40m                      Reuse one BlockTextOptions preset                      ␛[39;49m
    ␛[97;40m  ╭──────────────────────────────╮    ╭──────────────────────────────╮  ␛[39;49m
    ␛[97;40m  │␛[100m                              ␛[40m│    │␛[100m                              ␛[40m│  ␛[39;49m
    ␛[97;40m  │␛[100m         ␛[93mA␛[91mL␛[95mE␛[96mR␛[93mT␛[91m ␛[95mP␛[96mA␛[93mN␛[91mE␛[95mL␛[97m          ␛[40m│    │␛[100m         ␛[95mA␛[96mL␛[93mE␛[91mR␛[95mT␛[96m ␛[93mP␛[91mA␛[95mN␛[96mE␛[93mL␛[97m          ␛[40m│  ␛[39;49m
    ␛[97;40m  │␛[100m                              ␛[40m│    │␛[100m                              ␛[40m│  ␛[39;49m
    ␛[97;40m  │␛[100m                              ␛[40m│    │␛[100m                              ␛[40m│  ␛[39;49m
    ␛[97;40m  │␛[100m                              ␛[40m│    │␛[100m                              ␛[40m│  ␛[39;49m
    ␛[97;40m  ╰──────────────────────────────╯    ╰──────────────────────────────╯  ␛[39;49m
    ␛[97;40m             ␛[90mcycle = 1␛[97m                           ␛[90mcycle = 3␛[97m              ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
