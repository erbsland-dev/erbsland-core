..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************
Strings and Chars
*****************

The string classes describe terminal text before it is rendered.
They let you represent a single display character with color and character attributes, build styled text fragments,
measure Unicode-aware width, and split or wrap text without interacting with a buffer.

This page focuses on working with ``Block``, ``BlockStyle``, and ``BlockString`` as data types.
For rendering text blocks into rectangles with ``BlockText``, see :doc:`drawing-text`.

Usage
=====

Working with Block
------------------

``Block`` represents a single terminal display cell together with its foreground color, background color, and optional
character attributes such as bold or underline.

Constructing Characters ^^^^^^^^^^^^^^^^^^^^^^^

Use a Unicode code point when you construct a regular character.
This is the most explicit form when you already know the exact character.

.. code-block:: cpp

    const auto checkMark = Block{U'✓'};
    const auto whiteOnBlue = Block{U'a', fg::White, bg::Blue};

For combined characters, construct ``Block`` from UTF-8 or UTF-32 text:

.. code-block:: cpp

    const auto combinedFromUtf8 = Block{"e\xCC\x81"};
    const auto combinedFromUtf32 = Block{U"e\u0301"};

To append a combining mark programmatically, use ``withCombining()``:

.. code-block:: cpp

    const auto acute = Block{U"e"}.withCombining(char32_t{0x0301});

``displayWidth()`` follows terminal cell width, so wide and combining characters behave correctly during layout:

.. code-block:: cpp

    const auto wide = Block{U'界'};
    const auto combined = Block{"e\xCC\x81"};

    const auto wideCells = wide.displayWidth();          // 2
    const auto combinedCells = combined.displayWidth();  // 1

Testing and Comparing Characters ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To compare only the character value and ignore color, compare against a Unicode code point:

.. code-block:: cpp

    if (character == U'?') {
        // ...
    }

Use regular comparisons for small sets of accepted characters:

.. code-block:: cpp

    if (character == U'Y' || character == U'y' || character == U'J' || character == U'j') {
        // ...
    }

For screen-level comparisons, ``renderedEquals()`` is often more useful than ``operator==`` because it treats inherited
colors like terminal defaults:

.. code-block:: cpp

    const auto inherited = Block{U'X', fg::Inherited, bg::Inherited};
    const auto defaults = Block{U'X', fg::Default, bg::Default};

    const auto sameOnScreen = inherited.renderedEquals(defaults);

Recoloring Characters ^^^^^^^^^^^^^^^^^^^^^

To adjust the colors of an existing ``Block``, choose the method that matches your intent:

.. code-block:: cpp

    const auto base = Block{U'X', fg::Green, bg::Blue};

    const auto overlay = base.withOverlay(BlockStyle{Color{fg::BrightWhite, bg::Inherited}});
    const auto replaced = base.withColorReplaced(Color{fg::White, bg::Black});
    const auto basedOnTheme = base.withBase(BlockStyle{Color{fg::Inherited, bg::BrightBlack}});

With ``withOverlay()``, any ``Inherited`` component keeps the existing color, while ``Default`` resets that component to
the terminal default.

Working with Character Attributes ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Use ``BlockAttributes`` when you want a character or string fragment to explicitly enable, disable, or inherit ANSI text
attributes.

.. code-block:: cpp

    auto emphasis = BlockAttributes{};
    emphasis.setBold(true);
    emphasis.setUnderline(true);

    const auto heading = Block{U'H', fg::BrightWhite, emphasis};
    const auto plain = heading.withAttributes(BlockAttributes::reset());

Unspecified attributes inherit from the surrounding writer or string context, while specified bits overwrite that base
state.

Working with Character Styles ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Use ``BlockStyle`` when you want to bundle color and character attributes into one reusable value.

.. code-block:: cpp

    auto emphasis = BlockAttributes{};
    emphasis.setBold(true);

    const auto headingStyle = BlockStyle{Color{fg::BrightWhite, bg::Blue}, emphasis};
    auto heading = Block{U'H', headingStyle};

    heading.setStyle(heading.style().withOverlay(BlockStyle{Color{fg::Inherited, bg::Black}}));

``withOverlay()`` keeps inherited color components and unspecified attributes from the existing style, while
``withBase()`` resolves a style against an underlying base theme.

Building Strings
----------------

``BlockString`` stores a sequence of ``Block`` values.
It is the right type for status bars, prompts, labels, and any text where characters may use different colors.

Building Colored BlockText Fragments ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Prefer ``append(...)`` when building mixed-style strings.
It keeps colors, attributes, and text in a single readable sequence.

.. code-block:: cpp

    auto footer = BlockString{};
    footer.append(
        bg::BrightBlack,
        fg::BrightYellow,
        "[Q]",
        fg::BrightWhite,
        " quit");

    auto highlighted = BlockAttributes{};
    highlighted.setBold(true);
    highlighted.setUnderline(true);

    footer.append(
        "  ",
        highlighted,
        "Save",
        BlockAttributes::reset(),
        fg::BrightBlack,
        " shortcut");

``append(...)`` accepts colors, ``BlockStyle``, ``Block`` values, ``BlockAttributes``, plain text, and other
``BlockString`` instances.
Colors and attributes remain active for subsequent elements within the same call.

You can also construct a ``BlockString`` directly from UTF-8 or UTF-32 text:

.. code-block:: cpp

    const auto utf8Text = BlockString{"Gruezi"};
    const auto utf32Text = BlockString{U"Gru\u0308ezi"};

Control codes are filtered out automatically, except for tab and newline, which are preserved for layout and splitting.

Implicit Sharing, Views, and Ranges
-----------------------------------

``BlockString`` is implicitly shared.
Copying a string is cheap because the character storage is shared until one copy is modified through a mutable API.
Once that happens, the modified instance detaches and continues with its own private copy of the visible character
range.

This keeps normal value semantics:

.. code-block:: cpp

    auto original = BlockString{"alpha"};
    auto copy = original;   // shared storage

    copy[0] = Block{U'A'};   // detaches here

    // original is still "alpha"
    // copy is now "Alpha"

Use ``BlockStringView`` when an API only needs read-only access.
It references the same shared data as ``BlockString`` but exposes only non-mutating operations.
A ``BlockString`` converts implicitly to ``BlockStringView`` so APIs can use ``BlockStringView`` ` to accepts both,
``BlockString`` and ``BlockStringView``.

.. code-block:: cpp

    void drawLabel(BlockStringView text);

    auto title = BlockString{"Status: ready"};
    drawLabel(title);  // implicit BlockString -> BlockStringView

Handling Invalid UTF-8 Input
----------------------------

``EncodingErrors`` controls how UTF-8-based constructors react when the input is malformed.

.. code-block:: cpp

    const auto strict = BlockString{"Gr\xC3", EncodingErrors::Throw};     // throws
    const auto lossy = BlockString{"Gr\xC3", EncodingErrors::Replace};    // inserts U+FFFD
    const auto compact = BlockString{"Gr\xC3", EncodingErrors::Ignore};   // drops the broken bytes when supported

Use ``EncodingErrors::Throw`` at the boundary where invalid data should fail fast.
Use ``EncodingErrors::Replace`` when the application should stay readable even with damaged input.

Searching, Slicing, and Measuring
---------------------------------

Use ``size()`` to count stored characters and ``displayWidth()`` to measure terminal cell width.
These differ for full-width and combining characters.

.. code-block:: cpp

    const auto text = BlockString{"A界e\xCC\x81"};

    const auto characterCount = text.size();         // 3 terminal characters
    const auto terminalWidth = text.displayWidth();  // 4 cells

For content-aware processing, the following helpers are commonly useful:

.. code-block:: cpp

    const auto warningCount = BlockString{"!?!!"}.count(U'!');
    const auto nextQuestion = BlockString{"abc?def"}.indexOf(U'?');
    const auto middle = BlockString{"AB界D"}.substr(1, 2);

``count(Block)`` and ``indexOf(Block)`` compare both character and color.
The ``char32_t`` overloads ignore color and match only the character.

Splitting, Wrapping, and Lines
------------------------------

``splitWords()`` separates text at spaces, tabs, carriage returns, and newlines.
``splitLines()`` keeps empty lines and splits only at newline characters.

.. code-block:: cpp

    const auto words = BlockString{"alpha  beta\ngamma"}.splitWords();
    const auto lines = BlockString{"first\n\nthird"}.splitLines();

When you want string-level wrapping without creating a ``BlockText`` object, use ``wrapIntoLines()``:

.. code-block:: cpp

    const auto wrapped = BlockString{"alpha beta\ngamma"}.wrapIntoLines(6);
    const auto wrappedWithSpacing = BlockString{"alpha beta\ngamma"}.wrapIntoLines(
        6,
        ParagraphSpacing::DoubleLine);

    const auto lineCount = BlockString{"AA\nBB"}.terminalLines(2);

This is useful for preprocessing text, estimating layout, or building buffers line by line.

``BlockStringLines`` is the line-based companion type returned by ``splitLines()`` and ``wrapIntoLines()``.
You can also rebuild a string from line data:

.. code-block:: cpp

    const auto text = BlockString::fromLines({"one", "two"}, Color{fg::BrightWhite, bg::Blue});

If you want to render these lines directly into a buffer, see ``Buffer::fromLines()`` and :doc:`drawing-text`.
