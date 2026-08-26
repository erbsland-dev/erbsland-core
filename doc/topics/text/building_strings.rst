..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Building Strings
    single: Joining Strings
    single: StringEditor append
    single: String fromJoined
    single: StringList join
    single: AnyStringBuilder
    single: StringFormat

****************
Building Strings
****************

Building a short label and building a document assembled from hundreds of fragments may look similar in source code, but
they place very different demands on memory.
If a growing string repeatedly runs out of capacity, each growth step can allocate a larger block and copy everything
that has already been written.
That cost is easy to miss because every individual ``append()`` still looks harmless.

This page helps you choose a construction strategy from the information your algorithm already has.
You will see how a fixed set of fragments differs from a dynamically collected set, when reserving an editor is enough,
and what ``AnyStringBuilder`` and ``StringFormat`` add beyond concatenation.
The table is a quick map; the following sections explain the reasoning behind each choice.

.. list-table::
    :header-rows: 1
    :widths: 27 31 42

    *   -   Situation
        -   Preferred approach
        -   Allocation behavior
    *   -   A fixed set of text fragments
        -   :cpp:func:`String::fromJoined() <erbsland::text::U8String::fromJoined>`
        -   Calculates the complete native length and allocates final character storage once.
    *   -   A dynamic collection of fragments
        -   :cpp:type:`StringList <erbsland::text::StringList>` followed by ``join()``
        -   The list may grow, but final character storage is sized once when joined.
    *   -   A local editor already being mutated
        -   :cpp:type:`StringEditor <erbsland::text::StringEditor>` with one exact ``reserve()``
        -   The reservation prevents append growth from repeatedly reallocating the accumulated text.
    *   -   Width-independent or directly formatted output
        -   :cpp:class:`AnyStringBuilder <erbsland::text::AnyStringBuilder>`
        -   Growth has the same concern unless pre-sized, but values and encodings append without temporary strings.
    *   -   Structured text with placeholders
        -   :cpp:type:`StringFormat <erbsland::text::StringFormat>`
        -   ``appendTo()`` writes into a builder without creating an intermediate formatted string.

When an Editor Is Already the Working Value
===========================================

Appending to an editor is simple, but an editor with insufficient capacity must obtain a larger allocation and copy its
existing character data before continuing.
As the text grows, this can happen repeatedly.

When the exact final native length is known, calculate it first and call ``reserve()`` once.
For ``StringEditor`` and ``U8StringEditor``, capacity is measured in bytes.
For ``U16StringEditor``, it is measured in UTF-16 data units.
For ``U32StringEditor``, it is measured in code points.
Do not call ``reserve()`` before every append; that defeats the purpose of planning capacity once.

.. erbsland-demo::
    :source: text/StringBuilding/JoiningStrategies.cpp
    :function-blocks: joinWithStringEditor
    :function-blocks-sha256: dfb975176de36f2ff47515fabfd64a9a7de8294bf655f27844cf7d418b175d10
    :source-sha256: 730f48e8e18d7122cf315d6e4bc9f70d3cd04d8813454e6a05e205ab74ac132b

.. code-block:: cpp

    auto joinWithStringEditor() -> el::String {
        const auto fragments = std::array{
            el::String{"元素: "_el},
            el::String{"金"_el},
            el::String{" ("_el},
            el::String{"Au"_el},
            el::String{"), 原子番号 "_el},
            el::String{"79"_el},
        };

        auto finalLength = el::ByteLength{};
        for (const auto &fragment : fragments) {
            finalLength += fragment.length();
        }

        auto result = el::StringEditor{};
        result.reserve(finalLength);
        for (const auto &fragment : fragments) {
            result.append(fragment);
        }
        return el::String{result};
    }

.. erbsland-demo-end::

This approach is appropriate when the editor is already needed for later in-place editing.
For pure concatenation of fixed fragments, ``String::fromJoined()`` expresses the operation directly and performs the
same size planning internally.

When Every Fragment Is Known
============================

Use ``String::fromJoined({...})`` when all fragments are present at the call site.
The method adds their native lengths, allocates the final result once, and copies each fragment into that allocation.
It avoids both a growable editor and a separate fragment container.

.. erbsland-demo::
    :source: text/StringBuilding/JoiningStrategies.cpp
    :function-blocks: joinWithFromJoined
    :function-blocks-sha256: 02f43c4041b1172b86dde43fb4a9f900813a243ef553605d4424720338f8b91f
    :source-sha256: 730f48e8e18d7122cf315d6e4bc9f70d3cd04d8813454e6a05e205ab74ac132b

.. code-block:: cpp

    auto joinWithFromJoined() -> el::String {
        return el::String::fromJoined({"元素: "_el, "金"_el, " ("_el, "Au"_el, "), 原子番号 "_el, "79"_el});
    }

.. erbsland-demo-end::

The initializer-list form is intentionally best for a fixed number of fragments.
If a loop discovers fragments at run time, collect them in a ``StringList`` instead of rebuilding a larger result after
every iteration.

When Fragments Arrive Dynamically
=================================

``StringList`` stores ``String`` values.
Those values are normally lightweight handles to static, shared, or sliced storage, so collecting them does not copy the
referenced character data.
The list's own array can reallocate as it grows, but that moves small handles rather than the accumulated text.

``join()`` first calculates the complete output length, including separators, then allocates and fills the result.
This makes it a strong choice for fragments discovered by parsing, filtering, or iteration.

.. erbsland-demo::
    :source: text/StringBuilding/JoiningStrategies.cpp
    :function-blocks: joinWithStringList
    :function-blocks-sha256: e85d497ff230a8e5ff53b6791c64898d09899500c2e67e04a020b5abccf4ed87
    :source-sha256: 730f48e8e18d7122cf315d6e4bc9f70d3cd04d8813454e6a05e205ab74ac132b

.. code-block:: cpp

    auto joinWithStringList() -> el::String {
        const auto discoveredFragments = std::array{
            el::String{"元素: "_el},
            el::String{"金"_el},
            el::String{" ("_el},
            el::String{"Au"_el},
            el::String{"), 原子番号 "_el},
            el::String{"79"_el},
        };

        auto fragments = el::StringList{};
        for (const auto &fragment : discoveredFragments) {
            fragments.append(fragment);
        }
        return fragments.join();
    }

.. erbsland-demo-end::

If the approximate fragment count is known and list growth itself matters, reserve list capacity before collecting.
That optimization concerns the handle array, not the final character allocation.

When One Builder Must Serve Every String Width
==============================================

``AnyStringBuilder`` lets one function target UTF-8, UTF-16, or UTF-32. It accepts input from every supported string
width and converts directly into the configured output.
It can also append integers, floating-point values, and byte blocks using their format objects, avoiding a temporary
formatted string for each value.

The builder caches its current decoded code-point length.
That is useful while constructing layouts because repeatedly asking a ``StringEditor`` for its character length can
require scanning variable-width UTF-8 or UTF-16 data.

.. erbsland-demo::
    :source: text/StringBuilding/JoiningStrategies.cpp
    :function-blocks: appendElementLabel joinWithAnyStringBuilder
    :function-blocks-sha256: 2e9634c31d60e8572bbee27270b847ac6ea96653319359375afc1944baced02e
    :source-sha256: 730f48e8e18d7122cf315d6e4bc9f70d3cd04d8813454e6a05e205ab74ac132b

.. code-block:: cpp

    void appendElementLabel(el::AnyStringBuilder &builder) {
        builder.append(u"元素: "_el)
            .append(U'金')
            .append(" ("_el)
            .append("Au"_el)
            .append("), 原子番号 "_el)
            .appendInteger(79);
    }

    auto joinWithAnyStringBuilder() -> el::String {
        auto builder = el::AnyStringBuilder::u8();
        appendElementLabel(builder);
        return builder.takeString();
    }

.. erbsland-demo-end::

An unreserved builder can still reallocate and copy its accumulated character data while it grows.
Use ``AnyStringBuilder::u8(ByteLength)``, ``u16(U16DataLength)``, or ``u32(CpLength)`` when the exact native capacity is
known.
Use ``withCapacity()`` when only a conservative code-point capacity is known for a runtime-selected width.

The complete builder example below demonstrates direct numeric and byte-block formatting, cross-width input and output,
and the cached code-point length.

.. erbsland-demo::
    :source: text/AnyStringBuilder/FieldGuideCards.cpp
    :exec: text/any_string_builder --demo FieldGuideCards
    :source-sha256: 5cd5031951ee8c6f77d58a4eb74d2d9f9d3539c9ad56b38fafe2dafb025bf04d

.. code-block:: cpp

    /// `AnyStringBuilder` efficiently builds strings in memory.
    /// It appends integers, floating-point values, byte blocks, and differently
    /// encoded text directly. Its cached `length()` avoids rescanning the built
    /// text to count decoded code points.
    void fieldGuideCards() {
        const auto fern = Observation{
            .symbol = "🌿"_el,
            .species = "Farn im Moos"_el,
            .place = "Bachufer · 水辺"_el,
            .count = 7,
            .moisture = 62.5,
            .sampleId = el::ByteBlock{el::Byte{0x0fU}, el::Byte{0xa7U}},
            .note = "New leaves curl like tiny green clocks."_el,
        };
        const auto oak = Observation{
            .symbol = "🌳"_el,
            .species = "chêne ancien"_el,
            .place = "Forêt claire · północ"_el,
            .count = 3,
            .moisture = 48.25,
            .sampleId = el::ByteBlock{el::Byte{0x10U}, el::Byte{0x3cU}},
            .note = "Acorns found beside warm limestone."_el,
        };
        const auto cypress = Observation{
            .symbol = "🌲"_el,
            .species = "κυπαρίσσι"_el,
            .place = "Sun trail · camino del sol"_el,
            .count = 12,
            .moisture = 31.75,
            .sampleId = el::ByteBlock{el::Byte{0x12U}, el::Byte{0xceU}},
            .note = "Resin scent after noon rain."_el,
        };

        // Build a UTF-8 field guide page for display in the terminal. The helper
        // also receives UTF-16 input and converts it directly into the target.
        auto builder = el::AnyStringBuilder{};

        appendFieldGuideCard(builder, fern);
        el::io::printLine("First card preview:"_el);
        el::io::print(builder.toString());
        el::io::printLine("Cached code-point length after first card: "_el, builder.length());

        appendFieldGuideCard(builder, oak);
        el::io::printLine("Cached code-point length after second card: "_el, builder.length());

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
        auto u16Builder = el::AnyStringBuilder{el::StringKind::U16};
        auto u32Builder = el::AnyStringBuilder{el::StringKind::U32};
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

    void appendFieldGuideCard(el::AnyStringBuilder &builder, const Observation &observation) {
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
        builder.append(u"│ Place : "_el).append(observation.place).append(U'\n');
        builder.append("│ Count : "_el).appendInteger(observation.count).append(U'\n');
        builder.append("│ Moist.: "_el).appendFloat(observation.moisture).append(" %\n"_el);
        builder.append("│ Sample: "_el).appendByteBlock(observation.sampleId, el::ByteFormat::compact()).append(U'\n');
        builder.append("│ Note  : "_el).append(observation.note).append(U'\n');
        builder.append(U'╰').append(U'─', el::CpLength{58}).append("╯\n"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    First card preview:
    ╭─ 🌿 Farn im Moos ────────────────────────────────────────╮
    │ Place : Bachufer · 水辺
    │ Count : 7
    │ Moist.: 62.5 %
    │ Sample: 0fa7
    │ Note  : New leaves curl like tiny green clocks.
    ╰──────────────────────────────────────────────────────────╯
    Cached code-point length after first card: 239
    Cached code-point length after second card: 484

    Taken field guide draft:
    ╭─ 🌿 Farn im Moos ────────────────────────────────────────╮
    │ Place : Bachufer · 水辺
    │ Count : 7
    │ Moist.: 62.5 %
    │ Sample: 0fa7
    │ Note  : New leaves curl like tiny green clocks.
    ╰──────────────────────────────────────────────────────────╯

    ╭─ 🌳 chêne ancien ────────────────────────────────────────╮
    │ Place : Forêt claire · północ
    │ Count : 3
    │ Moist.: 48.25 %
    │ Sample: 103c
    │ Note  : Acorns found beside warm limestone.
    ╰──────────────────────────────────────────────────────────╯
    After take: 0

    Reused builder:
    ╭─ 🌲 κυπαρίσσι ───────────────────────────────────────────╮
    │ Place : Sun trail · camino del sol
    │ Count : 12
    │ Moist.: 31.75 %
    │ Sample: 12ce
    │ Note  : Resin scent after noon rain.
    ╰──────────────────────────────────────────────────────────╯

    UTF-16 export:
    ╭─ 🌲 κυπαρίσσι ───────────────────────────────────────────╮
    │ Place : Sun trail · camino del sol
    │ Count : 12
    │ Moist.: 31.75 %
    │ Sample: 12ce
    │ Note  : Resin scent after noon rain.
    ╰──────────────────────────────────────────────────────────╯

    UTF-32 export:
    ╭─ 🌲 κυπαρίσσι ───────────────────────────────────────────╮
    │ Place : Sun trail · camino del sol
    │ Count : 12
    │ Moist.: 31.75 %
    │ Sample: 12ce
    │ Note  : Resin scent after noon rain.
    ╰──────────────────────────────────────────────────────────╯

.. erbsland-demo-end::

Seeing the Trade-offs Side by Side
==================================

All four functions in the central example produce the same visible value.
Their differences are the time at which the final size is known and whether the abstraction provides editing,
collection, or formatted-width-independent behavior.

.. erbsland-demo::
    :source: text/StringBuilding/JoiningStrategies.cpp
    :function-blocks: joiningStrategies
    :function-blocks-sha256: 9c7c06f8ed950ea83c0c6bd423f4d720777b416babc05d1adb04f91d6818db03
    :exec: text/string_building --demo JoiningStrategies
    :source-sha256: 730f48e8e18d7122cf315d6e4bc9f70d3cd04d8813454e6a05e205ab74ac132b

.. code-block:: cpp

    void joiningStrategies() {
        el::io::printLine("StringEditor ......: "_el, joinWithStringEditor());
        el::io::printLine("String::fromJoined : "_el, joinWithFromJoined());
        el::io::printLine("StringList::join ..: "_el, joinWithStringList());
        el::io::printLine("AnyStringBuilder ..: "_el, joinWithAnyStringBuilder());
    }

.. erbsland-ansi::
    :escape-char: ␛

    StringEditor ......: 元素: 金 (Au), 原子番号 79
    String::fromJoined : 元素: 金 (Au), 原子番号 79
    StringList::join ..: 元素: 金 (Au), 原子番号 79
    AnyStringBuilder ..: 元素: 金 (Au), 原子番号 79

.. erbsland-demo-end::

Let a Format Describe Structured Output
=======================================

Prefer :cpp:type:`StringFormat <erbsland::text::StringFormat>` when the result has a stable structure with placeholders.
The format object makes alignment, numeric formatting, and escaping explicit and can reuse its parsed pattern.
Use ``build()`` when a standalone result is required.
Use ``appendTo()`` when adding formatted output to an existing ``AnyStringBuilder``; this avoids creating a temporary
formatted string before appending it.

See :doc:`using_string_format` for the format language.
When an existing API writes to a :cpp:class:`TextOutputStream <erbsland::stream::TextOutputStream>`, use
:cpp:class:`AnyStringBuilderStream <erbsland::stream::AnyStringBuilderStream>` as described in
:doc:`../stream/text_input_and_output` instead of adapting that stream workflow here.
