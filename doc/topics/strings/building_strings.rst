
.. index::
    !single: Building Strings
    single: StringBuilder
    single: StringBuilderStream
    single: StringFormat
    single: String
    single: U8String
    single: U16String
    single: U32String
    single: TextOutputStream
    single: String Construction
    single: String Formatting
    single: String Encoding
    single: Incremental String Building
    single: Stream String Capture
    single: Reusable Formatting Patterns
    single: String Editing

****************
Building Strings
****************

Erbsland Core offers several ways to create and modify strings.
The best choice depends on how you generate the text and whether you need formatting, streaming, or direct string
manipulation.

This page introduces the four most common approaches:

- :cpp:class:`StringBuilder <erbsland::text::StringBuilder>` for efficient incremental string construction.
- :cpp:class:`StringBuilderStream <erbsland::stream::StringBuilderStream>` when existing code writes to a text output stream.
- :cpp:type:`StringFormat <erbsland::text::StringFormat>` for reusable formatting patterns.
- :cpp:type:`String <erbsland::text::String>` when you want to directly modify an editable string.

As a general guideline:

- Use :cpp:class:`StringBuilder <erbsland::text::StringBuilder>` when you generate text piece by piece.
- Use :cpp:class:`StringBuilderStream <erbsland::stream::StringBuilderStream>` when your code already works with output streams.
- Use :cpp:type:`StringFormat <erbsland::text::StringFormat>` for structured text with placeholders and formatting rules.
- Use :cpp:type:`String <erbsland::text::String>` when you need to edit, insert, remove, or replace existing text.

Create Any Kind of String with ``StringBuilder``
================================================

:cpp:class:`StringBuilder <erbsland::text::StringBuilder>` is the
fundamental tool for efficiently constructing strings.

Use it when you generate text incrementally, append many fragments, or want to write helper functions that can build
strings in different encodings without changing their implementation.

A string builder can create UTF-8, UTF-16, and UTF-32 strings.
You may append text from any supported encoding and the builder automatically converts it to the configured target
encoding.

The API intentionally focuses on a small set of operations: appending text, appending formatted values, and finally
creating the resulting string.
This makes it efficient and easy to use in generic code.

.. erbsland-demo::
    :source: text/StringBuilder/FieldGuideCards.cpp
    :exec: string_builder --demo FieldGuideCards
    :source-sha256: 2dd7a5a2e4f69410b9408f64c51563c06de7260bd103e2f0d011f09ef51ea790

.. code-block:: cpp

    /// `StringBuilder` efficiently builds strings in memory.
    /// It can be reused, moved out with `takeString()`, and target UTF-8, UTF-16,
    /// or UTF-32 while generic helper functions keep the same signature.
    void fieldGuideCards() {
        const auto fern = Observation{
            .symbol = "🌿"_el,
            .species = "Farn im Moos"_el,
            .place = "Bachufer · 水辺"_el,
            .count = "7 fronds"_el,
            .note = "New leaves curl like tiny green clocks."_el,
        };
        const auto oak = Observation{
            .symbol = "🌳"_el,
            .species = "chêne ancien"_el,
            .place = "Forêt claire · północ"_el,
            .count = "3 seedlings"_el,
            .note = "Acorns found beside warm limestone."_el,
        };
        const auto cypress = Observation{
            .symbol = "🌲"_el,
            .species = "κυπαρίσσι"_el,
            .place = "Sun trail · camino del sol"_el,
            .count = "12 cones"_el,
            .note = "Resin scent after noon rain."_el,
        };

        // Build a UTF-8 field guide page for display in the terminal.
        auto builder = el::StringBuilder{};

        appendFieldGuideCard(builder, fern);
        el::io::printLine("First card preview:"_el);
        el::io::print(builder.toString());
        el::io::printLine("Length after first card: "_el, builder.length());

        appendFieldGuideCard(builder, oak);
        el::io::printLine("Length after second card: "_el, builder.length());

        // Move the completed page out, then continue with the same builder.
        auto guidePage = builder.takeString();
        el::io::printLine();
        el::io::printLine("Taken field guide draft:"_el);
        el::io::print(guidePage);
        el::io::printLine("After take: "_el, builder.length());

        appendFieldGuideCard(builder, cypress);
        el::io::printLine();
        el::io::printLine("Reused builder:"_el);
        el::io::print(builder.toString());

        // Use the same helper with builders that create UTF-16 and UTF-32 strings.
        auto u16Builder = el::StringBuilder{el::StringKind::U16};
        auto u32Builder = el::StringBuilder{el::StringKind::U32};
        appendFieldGuideCard(u16Builder, cypress);
        appendFieldGuideCard(u32Builder, cypress);

        const auto u16Export = u16Builder.toU16String();
        const auto u32Export = u32Builder.toU32String();

        el::io::printLine();
        el::io::printLine("UTF-16 export:"_el);
        el::io::printLine(u16Export);
        el::io::printLine("UTF-32 export:"_el);
        el::io::printLine(u32Export);
    }

    void appendFieldGuideCard(el::StringBuilder &builder, const Observation &observation) {
        if (!builder.isEmpty()) {
            builder.append(U'\n');
        }

        builder.append("╭─ "_el)
            .append(observation.symbol)
            .append(U' ')
            .append(observation.species)
            .append(U' ')
            .append(U'─', el::CpLength{53} - observation.species.characterLength() - observation.symbol.characterLength())
            .append("╮\n"_el);
        builder.append("│ Place : "_el).append(observation.place).append(U'\n');
        builder.append("│ Count : "_el).append(observation.count).append(U'\n');
        builder.append("│ Note  : "_el).append(observation.note).append(U'\n');
        builder.append(U'╰').append(U'─', el::CpLength{58}).append("╯\n"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    First card preview:
    ╭─ 🌿 Farn im Moos ────────────────────────────────────────╮
    │ Place : Bachufer · 水辺
    │ Count : 7 fronds
    │ Note  : New leaves curl like tiny green clocks.
    ╰──────────────────────────────────────────────────────────╯
    Length after first card: 214
    Length after second card: 436

    Taken field guide draft:
    ╭─ 🌿 Farn im Moos ────────────────────────────────────────╮
    │ Place : Bachufer · 水辺
    │ Count : 7 fronds
    │ Note  : New leaves curl like tiny green clocks.
    ╰──────────────────────────────────────────────────────────╯

    ╭─ 🌳 chêne ancien ────────────────────────────────────────╮
    │ Place : Forêt claire · północ
    │ Count : 3 seedlings
    │ Note  : Acorns found beside warm limestone.
    ╰──────────────────────────────────────────────────────────╯
    After take: 0

    Reused builder:
    ╭─ 🌲 κυπαρίσσι ───────────────────────────────────────────╮
    │ Place : Sun trail · camino del sol
    │ Count : 12 cones
    │ Note  : Resin scent after noon rain.
    ╰──────────────────────────────────────────────────────────╯

    UTF-16 export:
    ╭─ 🌲 κυπαρίσσι ───────────────────────────────────────────╮
    │ Place : Sun trail · camino del sol
    │ Count : 12 cones
    │ Note  : Resin scent after noon rain.
    ╰──────────────────────────────────────────────────────────╯

    UTF-32 export:
    ╭─ 🌲 κυπαρίσσι ───────────────────────────────────────────╮
    │ Place : Sun trail · camino del sol
    │ Count : 12 cones
    │ Note  : Resin scent after noon rain.
    ╰──────────────────────────────────────────────────────────╯

.. erbsland-demo-end::

Stream into a String Builder with ``StringBuilderStream``
=========================================================

:cpp:class:`StringBuilderStream <erbsland::stream::StringBuilderStream>`
combines a text output stream with a
:cpp:class:`StringBuilder <erbsland::text::StringBuilder>`.

Use it when text is already produced through stream-based APIs.
Instead of writing to a file or terminal, the output is captured in memory and can later be retrieved as a string.

This makes it easy to reuse existing functions that operate on
:cpp:class:`TextOutputStream <erbsland::stream::TextOutputStream>`
without creating separate implementations for string generation.

Internally, the class uses a string builder and therefore provides the same encoding flexibility and string extraction
methods.

.. erbsland-demo::
    :source: stream/StringBuilderStream/CaptureOutput.cpp
    :exec: string_builder_stream --demo CaptureOutput
    :source-sha256: d7fb6f07143457ea45c06a6edc3a98ccce0a0b20809119ad441a4b787cb4ea53

.. code-block:: cpp

    /// `StringBuilderStream` is the combination of a `TextOutputStream` and a `StringBuilder`.
    /// It allows you to write text output directly into an in-memory string, using the same interface
    /// as other text output streams.
    /// This is useful for generic stream writing functions: a caller can pass a string builder stream
    /// instead of a file or terminal stream to capture the output in memory.
    void captureOutput() {
        const auto stringBuilderStream = el::StringBuilderStream::create();
        for (auto i = 0; i < 3; ++i) {
            printMagicSquare(stringBuilderStream);
        }
        el::io::print(stringBuilderStream->takeString());
    }

    void printMagicSquare(const el::TextOutputStreamPtr &outputStream) {
        MagicSquare square{};
        buildMagicSquare(square);
        const auto intFormat = el::IntegerFormat::decimal().setFieldWidth(el::CpLength{3});
        outputStream->printLine("┌─────┬─────┬─────┐ Sum: "_el, square.magicSum);
        outputStream->printLine(
            intFormat, "│ "_el, square.values[0], " │ "_el, square.values[1], " │ "_el, square.values[2], " │"_el);
        outputStream->printLine("├─────┼─────┼─────┤"_el);
        outputStream->printLine(
            intFormat, "│ "_el, square.values[3], " │ "_el, square.values[4], " │ "_el, square.values[5], " │"_el);
        outputStream->printLine("├─────┼─────┼─────┤"_el);
        outputStream->printLine(
            intFormat, "│ "_el, square.values[6], " │ "_el, square.values[7], " │ "_el, square.values[8], " │"_el);
        outputStream->printLine("└─────┴─────┴─────┘"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    ┌─────┬─────┬─────┐ Sum: 90
    │  51 │   2 │  37 │
    ├─────┼─────┼─────┤
    │  16 │  30 │  44 │
    ├─────┼─────┼─────┤
    │  23 │  58 │   9 │
    └─────┴─────┴─────┘
    ┌─────┬─────┬─────┐ Sum: 126
    │  63 │  14 │  49 │
    ├─────┼─────┼─────┤
    │  28 │  42 │  56 │
    ├─────┼─────┼─────┤
    │  35 │  70 │  21 │
    └─────┴─────┴─────┘
    ┌─────┬─────┬─────┐ Sum: 60
    │  11 │  32 │  17 │
    ├─────┼─────┼─────┤
    │  26 │  20 │  14 │
    ├─────┼─────┼─────┤
    │  23 │   8 │  29 │
    └─────┴─────┴─────┘

.. erbsland-demo-end::

Create Reusable Formatting Patterns with ``StringFormat``
=========================================================

When the structure of a string remains the same and only the values change,
:cpp:type:`StringFormat <erbsland::text::StringFormat>` is often the most convenient solution.

A format object stores a parsed formatting pattern that can be reused many times.
This avoids repeatedly parsing the same format string and keeps formatting logic separate from application code.

You can either create a new string from formatted values or append the result directly to a
:cpp:class:`StringBuilder <erbsland::text::StringBuilder>`.

See :doc:`formatting_strings` for a complete description of the format language and all supported formatting options.

.. erbsland-demo::
    :source: text/StringFormat/FormattingPatterns.cpp
    :exec: string_format --demo FormattingPatterns
    :source-sha256: cf7ed864226075dee1b9dbb92cecc7bf0759eeb5d61e9f65cc2249e9d3952dfb

.. code-block:: cpp

    /// `StringFormat` stores a reusable formatting pattern.
    /// The pattern uses the same placeholder syntax as `std::format`.
    ///
    /// Use `build()` to create a new string from formatted values.
    /// Use `appendTo()` to add formatted text to an existing `StringBuilder` without
    /// creating temporary strings.
    void formattingPatterns() {
        // Create a reusable pattern for ISO 8601 date-time values.
        const auto isoDateTime = el::StringFormat{"{:04}-{:02}-{:02}T{:02}:{:02}:{:02}"};

        auto timestamp = isoDateTime.build(2026, 5, 30, 21, 41, 56);
        el::io::printLine("ISO date-time: "_el, timestamp);

        // Create a pattern to for simple HTML tags.
        const auto htmlTag = el::StringFormat{"<{0}>{1:/html}</{0}>\n"};

        el::StringBuilder htmlOutput;
        htmlTag.appendTo(htmlOutput, "h1"_el, "Hello World"_el);
        htmlTag.appendTo(htmlOutput, "p"_el, "This paragraph was appended to a string builder."_el);
        htmlTag.appendTo(htmlOutput, "p"_el, "We add another <p> tag with \"useful\" text."_el);

        el::io::printLine("HTML output:"_el);
        el::io::print(htmlOutput);
    }

.. erbsland-ansi::
    :escape-char: ␛

    ISO date-time: 2026-05-30T21:41:56
    HTML output:
    <h1>Hello World</h1>
    <p>This paragraph was appended to a string builder.</p>
    <p>We add another &lt;p&gt; tag with &quot;useful&quot; text.</p>

.. erbsland-demo-end::

Build Strings Directly with ``String``
======================================

The simplest way to build or modify text is to use an editable
:cpp:type:`String <erbsland::text::String>` (or
:cpp:class:`U8String <erbsland::text::U8String>`,
:cpp:class:`U16String <erbsland::text::U16String>`, and
:cpp:class:`U32String <erbsland::text::U32String>`).

These classes provide operations such as appending, inserting, removing, replacing, and transforming text.

This approach is often the easiest to understand because you work directly with the resulting string.
However, when constructing large strings from many fragments, a
:cpp:class:`StringBuilder <erbsland::text::StringBuilder>` is usually
more efficient because it is specifically designed for incremental construction.

.. erbsland-demo::
    :source: text/String/EditingText.cpp
    :exec: string --demo EditingText
    :source-sha256: c22d511e71c1bdb50b5d13618e77bd50d83e588671ec2bcf7bf814a6a009d5d4

.. code-block:: cpp

    /// `String` is an owning, editable copy-on-write string type.
    /// Use it when you build text from scratch or modify existing text.
    /// Use `StringView` for parameters and stored read-only text.
    void editingText() {
        // Create an editable string from a string literal.
        auto story = el::String{"The forest wakes up at sunrise. A small fox follows the river. "
                                "Der Wind trägt Blätter durch die Luft."_el};

        // Alternatively, create an editable string directly with the `""_els` literal.
        auto intro = "A short forest story:"_els;

        // Find the insertion position after the first sentence.
        auto firstFullStopIndex = story.findFirstOf({U'.'});
        story.advance(firstFullStopIndex, el::CpLength{2});

        // Insert a new sentence after the first sentence.
        story.insert(firstFullStopIndex, "Birds start singing in the old oak tree. "_el);

        // Replace the main character.
        story.replaceAll("small fox"_el, "curious hedgehog"_el);

        // Remove one sentence from the story.
        constexpr auto sentenceBeginText = "Der Wind"_el;
        auto sentenceBeginIndex = story.find(sentenceBeginText);
        constexpr auto sentenceEndText = "Luft."_el;
        auto sentenceEndIndex = story.find(sentenceEndText, sentenceBeginIndex) + sentenceEndText.length();
        story.remove({sentenceBeginIndex, sentenceEndIndex});

        // Append a final sentence.
        story.append(" 🌿 The day begins quietly."_el);

        // Add a line break after each mid-sentence.
        story.replaceAll(". "_el, ".\n"_el);

        el::io::printLine(intro);
        el::io::printLine(story);
    }

.. erbsland-ansi::
    :escape-char: ␛

    A short forest story:
    The forest wakes up at sunrise.
    Birds start singing in the old oak tree.
    A curious hedgehog follows the river.
     🌿 The day begins quietly.

.. erbsland-demo-end::
