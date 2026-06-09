..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

****
Font
****

The font classes allow you to render large, bitmap-based text directly in the terminal.
You typically use fonts for titles, banners, or other decorative elements that should visually stand out in your
interface.

A ``Font`` defines how characters are mapped to bitmap glyphs.
These glyphs are then rendered onto the terminal buffer, giving you full control over how text appears beyond standard
character rendering.

.. dropdown:: Details about the example output on this page

    The examples below were rendered using the dedicated documentation
    helper :file:`doc/tools/font-reference.cpp` at a fixed width of
    72 terminal columns. This allows you to regenerate the visual output
    alongside the code snippets.

Usage
=====

Rendering a Bitmap Font Title
-----------------------------

``Font`` integrates directly with ``BlockText``.
Assign a font to a ``BlockText`` instance and render it like any other text block.

.. code-block:: cpp

    auto title = BlockText{BlockString{"COLOR TERM"}, BlockRectangle{0, 0, 60, 6}, Alignment::Center};
    title.setFont(Font::defaultAscii());
    title.setColorSequence(ColorSequence{
        Color{fg::BrightBlue, bg::Black},
        Color{fg::BrightCyan, bg::Black},
        Color{fg::BrightMagenta, bg::Black},
        Color{fg::BrightYellow, bg::Black},
    });
    title.setAnimation(BlockTextAnimation::ColorDiagonal);

    buffer.drawBlockText(title, animationCycle);

The built-in ``defaultAscii()`` font renders large ASCII characters using a bitmap representation.
Because fonts integrate seamlessly with ``BlockText``, all existing features—such as alignment, color sequences, and
animations—work without additional configuration.

This means you can treat bitmap text just like regular text while achieving a much stronger visual impact.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[39m␛[97;40m                    defaultAscii() for large titles                     ␛[39;49m
    ␛[97;40m ␛[90mBitmap fonts still use BlockText alignment, color sequences, and animation.␛[97m ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m                          ␛[94m▗␛[96m▄␛[95m▖␛[93m▗␛[94m▄␛[96m▖␛[95m▗␛[93m▄␛[94m▖␛[96m▗␛[95m▖␛[93m ␛[94m▄␛[96m ␛[95m▗␛[93m▖␛[94m ␛[96m▄␛[95m ␛[93m ␛[97m                          ␛[39;49m
    ␛[97;40m                          ␛[96m█␛[95m ␛[93m▀␛[94m ␛[96m█␛[95m ␛[93m█␛[94m ␛[96m▀␛[95m▐␛[93m█␛[94m▖␛[96m█␛[95m▗␛[93m▛␛[94m▜␛[96m▖␛[95m█␛[93m ␛[94m ␛[97m                          ␛[39;49m
    ␛[97;40m                          ␛[95m▝␛[93m▀␛[94m▙␛[96m ␛[95m█␛[93m ␛[94m█␛[96m▝␛[95m█␛[93m▐␛[94m▌␛[96m▜␛[95m█␛[93m▐␛[94m▛␛[96m▜␛[95m▌␛[93m█␛[94m ␛[96m ␛[97m                          ␛[39;49m
    ␛[97;40m                          ␛[93m▜␛[94m▄␛[96m▛␛[95m▗␛[93m█␛[94m▖␛[96m▜␛[95m▄␛[93m▛␛[94m▐␛[96m▌␛[95m ␛[93m█␛[94m▐␛[96m▌␛[95m▐␛[93m▌␛[94m█␛[96m▄␛[95m▄␛[97m                          ␛[39;49m
    ␛[97;40m             ␛[90manimated headline in a regular BlockText rectangle␛[97m              ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m

Creating a Font from Row Masks
------------------------------

For small custom alphabets, the most direct approach is to create a ``Font`` with a fixed bitmap height and add only the
glyphs you need using ``FontGlyph``.

Each row in the bitmap is defined as a bitmask, where set bits represent filled pixels.
This gives you precise control over the glyph shape while keeping the definition compact and efficient.

.. code-block:: cpp

    auto font = std::make_shared<Font>(5);
    font->addGlyph("A", FontGlyph{std::vector<uint64_t>{
        0b01110U,
        0b10001U,
        0b11111U,
        0b10001U,
        0b10001U}});
    font->addGlyph("M", FontGlyph{std::vector<uint64_t>{
        0b10001U,
        0b11011U,
        0b10101U,
        0b10001U,
        0b10001U}});
    font->addGlyph("O", FontGlyph{std::vector<uint64_t>{
        0b01110U,
        0b10001U,
        0b10001U,
        0b10001U,
        0b01110U}});
    font->addGlyph("T", FontGlyph{std::vector<uint64_t>{
        0b11111U,
        0b00100U,
        0b00100U,
        0b00100U,
        0b00100U}});

    auto text = BlockText{BlockString{"ATOM"}, BlockRectangle{0, 0, 72, 3}, Alignment::Center};
    text.setFont(font);
    text.setColor(Color{fg::BrightGreen, bg::Black});
    buffer.drawBlockText(text);

This approach works well for logos, badges, and domain-specific mini alphabets.
A five-row bitmap font is rendered as three terminal rows because the renderer automatically packs the bitmap into
Unicode block characters.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[39m␛[97;40m                       Custom font from row masks                       ␛[39;49m
    ␛[97;40m   ␛[90mAdd just the glyphs you need for logos, badges, or tiny alphabets.␛[97m   ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m                              ␛[92m▞▀▖▀▛▘▞▀▖▙▗▌␛[97m                              ␛[39;49m
    ␛[97;40m                              ␛[92m▛▀▌ ▌ ▌ ▌▌▘▌␛[97m                              ␛[39;49m
    ␛[97;40m                              ␛[92m▘ ▘ ▘ ▝▀ ▘ ▘␛[97m                              ␛[39;49m
    ␛[97;40m                  ␛[90m5 bitmap rows become 3 terminal rows␛[97m                  ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m

Embedding a Reusable Font Preset
--------------------------------

If you want a custom font to live directly in your source tree, you can define glyphs as small pixel patterns and store
them in a ``Font::GlyphMap``.

This keeps glyph definitions readable and easy to maintain.

.. code-block:: cpp

    auto makeGlyph(std::initializer_list<std::string_view> rows) -> FontGlyph {
        auto bitmap = Bitmap::fromPattern(rows);
        auto glyph = FontGlyph{bitmap.size()};
        glyph.draw(BlockPosition{0, 0}, bitmap);
        return glyph;
    }

    auto makeClockFont() -> FontPtr {
        auto glyphs = Font::GlyphMap{};
        glyphs.emplace("1", makeGlyph({
            "..#..",
            ".##..",
            "..#..",
            "..#..",
            "..#..",
            "..#..",
            ".###."}));
        // ...
        return std::make_shared<Font>(7, std::move(glyphs));
    }

    auto clock = BlockText{BlockString{"12:30"}, BlockRectangle{12, 3, 48, 4}, Alignment::Center};
    clock.setFont(makeClockFont());
    clock.setColor(Color{fg::BrightYellow, bg::Inherited});
    buffer.drawBlockText(clock);

This approach is ideal when you only need a limited alphabet, such as digits, status labels, or brand-specific glyphs.
Characters that are not defined in the font are simply skipped during rendering.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[39m␛[97;40m                                                                        ␛[39;49m
    ␛[97;40m          ╔═══════════════␛[44mEmbedded font preset␛[40m═══════════════╗          ␛[39;49m
    ␛[97;40m          ║␛[44m                                                  ␛[40m║          ␛[39;49m
    ␛[97;40m          ║␛[44m                  ␛[93m▗▌ ▟▜▖▖▀▜▖▟▜▖␛[97m                   ␛[40m║          ␛[39;49m
    ␛[97;40m          ║␛[44m                  ␛[93m ▌  ▟▘ ▗▟▘█▐▌␛[97m                   ␛[40m║          ␛[39;49m
    ␛[97;40m          ║␛[44m                  ␛[93m ▌ ▟▘ ▘ ▐▌█▐▌␛[97m                   ␛[40m║          ␛[39;49m
    ␛[97;40m          ║␛[44m                  ␛[93m▝▀ ▀▀▘ ▀▀ ▝▀ ␛[97m                   ␛[40m║          ␛[39;49m
    ␛[97;40m          ║␛[44m    Build one GlyphMap and reuse the FontPtr.     ␛[40m║          ␛[39;49m
    ␛[97;40m          ╚══════════════════════════════════════════════════╝          ␛[39;49m
    ␛[97;40m                                                                        ␛[39;49m

Reusing Font Settings with BlockTextOptions
-------------------------------------------

Fonts are part of ``BlockTextOptions``.
This allows you to bundle a font together with colors, animation, alignment, and paragraph settings, and then reuse that
configuration across multiple ``BlockText`` instances.

.. code-block:: cpp

    auto options = BlockTextOptions{Alignment::Center};
    options.setFont(makeRowMaskFont());
    options.setColorSequence(ColorSequence{
        Color{fg::BrightYellow, bg::Inherited},
        Color{fg::BrightRed, bg::Inherited},
        Color{fg::BrightMagenta, bg::Inherited},
    });
    options.setAnimation(BlockTextAnimation::ColorDiagonal);

    auto left = BlockText{BlockString{"ATOM"}, BlockRectangle{4, 3, 28, 3}, Alignment::Center};
    left.setBlockTextOptions(options);
    auto right = BlockText{BlockString{"TOMATO"}, BlockRectangle{40, 3, 28, 3}, Alignment::Center};
    right.setBlockTextOptions(options);

    buffer.drawBlockText(left, 1);
    buffer.drawBlockText(right, 6);

This is especially useful if your application uses a consistent headline style across multiple views.

.. erbsland-ansi::
    :escape-char: ␛

    ␛[39m␛[97;40m                      Reuse one BlockTextOptions preset                      ␛[39;49m
    ␛[97;40m  ╭──────────────────────────────╮    ╭──────────────────────────────╮  ␛[39;49m
    ␛[97;40m  │␛[100m                              ␛[40m│    │␛[100m                              ␛[40m│  ␛[39;49m
    ␛[97;40m  │␛[100m         ␛[91m▞␛[95m▀␛[93m▖␛[91m▀␛[95m▛␛[93m▘␛[91m▞␛[95m▀␛[93m▖␛[91m▙␛[95m▗␛[93m▌␛[97m         ␛[40m│    │␛[100m      ␛[93m▀␛[91m▛␛[95m▘␛[93m▞␛[91m▀␛[95m▖␛[93m▙␛[91m▗␛[95m▌␛[93m▞␛[91m▀␛[95m▖␛[93m▀␛[91m▛␛[95m▘␛[93m▞␛[91m▀␛[95m▖␛[97m      ␛[40m│  ␛[39;49m
    ␛[97;40m  │␛[100m         ␛[95m▛␛[93m▀␛[91m▌␛[95m ␛[93m▌␛[91m ␛[95m▌␛[93m ␛[91m▌␛[95m▌␛[93m▘␛[91m▌␛[97m         ␛[40m│    │␛[100m      ␛[91m ␛[95m▌␛[93m ␛[91m▌␛[95m ␛[93m▌␛[91m▌␛[95m▘␛[93m▌␛[91m▛␛[95m▀␛[93m▌␛[91m ␛[95m▌␛[93m ␛[91m▌␛[95m ␛[93m▌␛[97m      ␛[40m│  ␛[39;49m
    ␛[97;40m  │␛[100m         ␛[93m▘␛[91m ␛[95m▘␛[93m ␛[91m▘␛[95m ␛[93m▝␛[91m▀␛[95m ␛[93m▘␛[91m ␛[95m▘␛[97m         ␛[40m│    │␛[100m      ␛[95m ␛[93m▘␛[91m ␛[95m▝␛[93m▀␛[91m ␛[95m▘␛[93m ␛[91m▘␛[95m▘␛[93m ␛[91m▘␛[95m ␛[93m▘␛[91m ␛[95m▝␛[93m▀␛[91m ␛[97m      ␛[40m│  ␛[39;49m
    ␛[97;40m  │␛[100m                              ␛[40m│    │␛[100m                              ␛[40m│  ␛[39;49m
    ␛[97;40m  ╰──────────────────────────────╯    ╰──────────────────────────────╯  ␛[39;49m
    ␛[97;40m                 ␛[90mone font preset, multiple text blocks␛[97m                  ␛[39;49m

