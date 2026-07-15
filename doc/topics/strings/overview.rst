..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Strings
    single: String
    single: StringView
    single: StringLiteral
    single: StringBuilder
    single: Char
    single: U8String
    single: U16String
    single: U32String
    single: U8StringView
    single: U16StringView
    single: U32StringView
    single: U8StringLiteral
    single: U16StringLiteral
    single: U32StringLiteral
    single: U8StringCharView
    single: U16StringCharView
    single: StringFormat
    single: Character Views
    single: Copy on Write
    single: Thread Safety
    single: Unicode Support
    single: UTF-8
    single: UTF-16
    single: UTF-32
    single: EncodingErrorMode

********************
Working with Strings
********************

This page gives you a compact overview of all string types in this library.
You will learn how the different string classes relate to each other, when to use each type in your own code, and why
the design differs from the C++ standard library.

String Encodings
================

The library supports three Unicode encodings, each represented by its own family of string types.

UTF-8
-----

UTF-8 encodes each Unicode code point as one to four bytes.
It is the most compact encoding for ASCII text and the de facto standard for file I/O, network protocols, and storage.

.. code-block:: text
    :caption: One code-point is encoded as one, two, three or four bytes in the data.

    e  l     r  í     o  🌊
    65 6c 20 72 c3 ad 6f f0 9f 8c 8a

Use UTF-8 when:

* You work with streamed text from files, the network, or user input.
* Memory efficiency matters and for typical ASCII content.
* If text access is sequential, not random.

The main trade-off is that character and byte counts differ, and random character access is slower than sequential
access because each character must be decoded.
A good compromise is often to store lines of UTF-8 text.
That allows fast access to any portion of the text while keeping the memory footprint low.

UTF-16
------

UTF-16 encodes each Unicode code point as one or two 16-bit code units.
It is the natural encoding for Windows APIs, JavaScript engines, and many cross-platform frameworks.

.. code-block:: text
    :caption: One code-point is encoded as one or two 16bit values.

    e    l         r    í    o    🌊
    0065 006c 0020 0072 00ed 006f d83c df0a

Use UTF-16 when:

* You interface with platform APIs that use UTF-16.
* You need fixed-size code units for indexing while still supporting the full Unicode range.

UTF-16 is larger than UTF-8 for ASCII content but smaller than UTF-32. Surrogate pairs complicate character indexing,
which is why the library provides a dedicated character view type.

UTF-32
------

UTF-32 encodes each Unicode code point as exactly one 32-bit value.
It is the simplest encoding for character-level operations.

.. code-block:: text
    :caption: One code-point is always encoded as one single 32bit value.

    e        l                 r        í        o        🌊
    00000065 0000006c 00000020 00000072 000000ed 0000006f 0001f30a

Use UTF-32 when:

* You need fast random access to characters by index.
* Your algorithm treats each code point uniformly.
* Memory size is not a primary concern.

UTF-32 is the most memory-intensive encoding but the easiest to reason about at the character level.

Library Default
---------------

The library uses UTF-8 as its default encoding.
The common aliases :cpp:type:`String <erbsland::text::String>`, :cpp:type:`StringView <erbsland::text::StringView>`,
:cpp:type:`StringLiteral <erbsland::text::StringView>`, and :cpp:type:`StringFormat <erbsland::text::StringFormat>`
are all UTF-8 types.
For most applications, you only ever need these aliases.

.. _string-types-overview:

String Types
============

This library provides four kinds of string types for each encoding: owning strings, views, character views, and
literals.
The table below shows the full set.

.. list-table::
    :width: 100%
    :header-rows: 1

    *   -   –
        -   **UTF-8**
        -   **UTF-16**
        -   **UTF-32**
    *   -   **Owning string view**
        -   |   :cpp:type:`StringView <erbsland::text::StringView>`
            |   (:cpp:class:`U8StringView <erbsland::text::U8StringView>`)
        -   :cpp:class:`U16StringView <erbsland::text::U16StringView>`
        -   :cpp:class:`U32StringView <erbsland::text::U32StringView>`
    *   -   **Owning character view**
        -   :cpp:class:`U8StringCharView <erbsland::text::U8StringCharView>`
        -   :cpp:class:`U16StringCharView <erbsland::text::U16StringCharView>`
        -   —
    *   -   **Owning string editor**
        -   |   :cpp:type:`String <erbsland::text::String>`
            |   (:cpp:class:`U8String <erbsland::text::U8String>`)
        -   :cpp:class:`U16String <erbsland::text::U16String>`
        -   :cpp:class:`U32String <erbsland::text::U32String>`
    *   -   **String literal**
        -   |   :cpp:type:`StringLiteral <erbsland::text::StringLiteral>`
            |   (:cpp:class:`U8StringLiteral <erbsland::text::U8StringLiteral>`)
        -   :cpp:class:`U16StringLiteral <erbsland::text::U16StringLiteral>`
        -   :cpp:class:`U32StringLiteral <erbsland::text::U32StringLiteral>`

The aliases :cpp:type:`String <erbsland::text::String>`, :cpp:type:`StringView <erbsland::text::StringView>` and
:cpp:type:`StringLiteral <erbsland::text::StringView>` refer to the UTF-8 types and are the ones you will use most often.

Owning String Views
-------------------

String views are read-only, owning references to string data.
They are the most efficient type for storing and passing strings because they never copy data when possible.

A view internally stores either a reference to shared string data or a reference to the read-only memory of a string
literal.
Operations on views are fast because the underlying memory is never modified or copied.

.. erbsland-demo::
    :source: text/StringView/IdealFunctionParameter.cpp
    :exec: text/string_view --demo IdealFunctionParameter
    :source-sha256: 1174794885cf3e3869b50c12bebb9246f83997dd0f2fa21ee42a3be804f58d28

.. code-block:: cpp

    /// `StringView` is the preferred parameter type for functions that read text.
    ///
    /// It accepts common string inputs naturally:
    /// - string literals are used directly without copying,
    /// - existing views share their backend,
    /// - editable strings provide a read-only view to their contents.
    ///
    /// From the caller's perspective, all variants behave the same.
    void idealFunctionParameter() {
        // A string literal can be passed directly.
        countEmojis("🌲🌲 Waldkonzert mit Fuchs 🦊 und Eule 🦉"_el);

        // An existing view can be passed without copying.
        const auto stringView = el::StringView{"Pluie douce sur les fleurs 🌧️🌷🌼"_el};
        countEmojis(stringView);

        // An editable string is accepted as read-only input.
        const auto stringEdit = el::String{"Bosque nocturno: luna 🌙, estrellas ✨ y grillos 🦗"_el};
        countEmojis(stringEdit);
    }

    /// Count all emoji-like symbols in `text` and print the result.
    ///
    /// The function only needs read-only access to the text. It does not need to know
    /// whether the caller passed a literal, a view, or an editable string.
    void countEmojis(const el::StringView &text) {
        std::size_t emojiCount = 0;
        text.forEach([&](const el::Char character) mutable noexcept -> el::util::LoopStatus {
            if (character.isCategory(el::UnicodeCategory::OtherSymbol)) {
                ++emojiCount;
            }
            return el::LoopStatus::Continue;
        });

        el::io::printLine("Text ..........: \"", text, "\"");
        el::io::printLine("Emoji symbols .: ", emojiCount);
        el::io::printLine();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Text ..........: "🌲🌲 Waldkonzert mit Fuchs 🦊 und Eule 🦉"
    Emoji symbols .: 4

    Text ..........: "Pluie douce sur les fleurs 🌧️🌷🌼"
    Emoji symbols .: 3

    Text ..........: "Bosque nocturno: luna 🌙, estrellas ✨ y grillos 🦗"
    Emoji symbols .: 3

.. erbsland-demo-end::

Views are the preferred type for function parameters and return values in library APIs.
They can be implicitly constructed from owning strings and literals, making them a natural drop-in replacement for
``std::string_view`` in most contexts.

String Literals
---------------

String literals are wrappers around compile-time string literals, created with the ``""_el`` suffix.
They are zero-cost references to read-only memory and can be converted to views without any allocation.

.. erbsland-demo::
    :source: text/StringLiteral/ZeroCost.cpp
    :exec: text/string_literal --demo ZeroCost
    :source-sha256: 4a98e5b2c72e216725444dac86c354166cb6a0c9e44bd6774222addb7c8e90d7

.. code-block:: cpp

    /// Import the `_el` suffix for convenient string literals.
    using namespace el::text::literals;

    /// String literals are lightweight read-only references.
    constexpr auto cForestStory = "🌲 Im Wald rauscht der Wind."_el;
    constexpr auto cRiverStory = el::StringLiteral{"川の水は静かに流れます。"};

    /// String literals are created entirely at compile time.
    ///
    /// Both `""_el` and `el::StringLiteral{"..."}` produce lightweight references
    /// to read-only memory. Copying them is inexpensive because no character data
    /// is duplicated.
    void zeroCost() {
        // Inspect a literal created with the `_el` suffix.
        el::io::printLine("Forest story .: "_el, cForestStory);
        el::io::printLine("Valid UTF-8 ..: "_el, cForestStory.isValidUtf8());
        el::io::printLine("Length .......: "_el,
            cForestStory.length(),
            " bytes / "_el,
            cForestStory.characterLength(),
            " code-points"_el);

        // The same functionality using `el::StringLiteral`.
        el::io::printLine("\nRiver story ..: "_el, cRiverStory);
        el::io::printLine("Valid UTF-8 ..: "_el, cRiverStory.isValidUtf8());
        el::io::printLine(
            "Length .......: "_el, cRiverStory.length(), " bytes / "_el, cRiverStory.characterLength(), " code-points"_el);

        // Copying a literal only copies the lightweight reference.
        auto storyCopy = cForestStory;
        el::io::printLine("\nCopy .........: "_el, storyCopy);
        el::io::printLine("Length .......: "_el, storyCopy.length(), " bytes"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Forest story .: 🌲 Im Wald rauscht der Wind.
    Valid UTF-8 ..: true
    Length .......: 30 bytes / 27 code-points

    River story ..: 川の水は静かに流れます。
    Valid UTF-8 ..: true
    Length .......: 36 bytes / 12 code-points

    Copy .........: 🌲 Im Wald rauscht der Wind.
    Length .......: 30 bytes

.. erbsland-demo-end::

Literals are primarily used as function arguments, constant expressions, and as the source for views.
For most use cases, ``""_el`` is the recommended suffix.
The suffixes ``""_elv`` and ``""_els`` exist for completeness and create a view or an owning string copy, respectively.

Editable Strings
----------------

Owning strings are mutable, copy-on-write containers that own their data.
Use them when you need to modify, append, or store strings that outlive their source.

.. erbsland-demo::
    :source: text/String/EditingText.cpp
    :exec: text/string --demo EditingText
    :source-sha256: a07d080bec20b97c5a0d0da84850de1224168babb27b6c86c1b1861ee09c76b5

.. code-block:: cpp

    /// `String` is an owning, editable copy-on-write string type.
    /// Use it when you build text from scratch or modify existing text.
    /// Use `StringView` for parameters and stored read-only text.
    void editingText() {
        // Create an editable string from a string literal.
        auto story = el::String{"The frost lifts from the valley. A pale crocus opens beside the stone. "
                                "Der Wind trägt Blätter durch die Luft."_el};

        // Alternatively, create an editable string directly with the `""_els` literal.
        auto intro = "A short alpine field note:"_els;

        // Find the insertion position after the first sentence.
        auto firstFullStopIndex = story.findFirstOf({U'.'});
        story.advance(firstFullStopIndex, el::CpLength{2});

        // Insert a new sentence after the first sentence.
        story.insert(firstFullStopIndex, "Sunlight reaches the wet limestone. "_el);

        // Replace the main subject.
        story.replaceAll("pale crocus"_el, "violet gentian"_el);

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

    A short alpine field note:
    The frost lifts from the valley.
    Sunlight reaches the wet limestone.
    A violet gentian opens beside the stone.
     🌿 The day begins quietly.

.. erbsland-demo-end::

Creating an owning string from a view or literal always copies the data.
While you *can* pass them around because of the copy-on-write feature, you should only use them when you need to edit
text in place, or building strings from scratch.
For text storage, passing parameters or just interpreting strings, use the more efficient
:cpp:type:`StringView <erbsland::text::StringView>`.

If you need a more generic approach building strings, you may find the
:cpp:class:`StringBuilder <erbsland::text::StringBuilder>` class useful.
It has one interface to build all three string types.

Character Views
---------------

Character views provide a specialized, character-index-based access to UTF-8 and UTF-16 data.
Accessing index positions by code-points can be very slow, especially if you work with large texts and randomly
accessing positions in this text.
For that reason, the code-point views are a separate interface that can be accessed using the ``toCharView()`` method.
It is only available for UTF-8 and UTF-16 strings.

Character views are useful, if you work with short strings, and the cost for converting them into UTF-32 would be larger
than the cost for scanning code-point positions.

.. erbsland-demo::
    :source: text/StringCharView/CharacterGrid.cpp
    :exec: text/string_char_view --demo CharacterGrid
    :source-sha256: 005674346673a841ca9e58bda504a51cddb0eef5e14385cc78ec9d0be77bc014

.. code-block:: cpp

    /// `StringCharView` is a specialized interface for code-point indexed access.
    /// It is useful when text is organized by character positions, for example in
    /// grids, terminal layouts, diagnostics, or editor columns.
    /// Prefer byte indexes for general parsing, searching, and slicing UTF-8 text.
    void characterGrid() {

        const auto grid = el::StringView{"AΩBçDÉFGH×\n"
                                         "IJKLMNÖPQR\n"
                                         "STÜVWXYZ01\n"
                                         "23456789ab\n"
                                         "cdefghijkl\n"
                                         "mnopqrstuv\n"
                                         "wxyzäöüß+-\n"
                                         "∑∏√∞≈≠≤≥÷·\n"
                                         "ABCDEFGHIJ\n"
                                         "UVWXYZ!?._\n"_el};

        auto gridCV = grid.toCharView();

        constexpr auto sourceWidth = el::CpLength{11};
        constexpr auto targetWidth = el::CpLength{10};
        constexpr auto height = el::CpLength{10};

        // Rotate the 10x10 character grid clockwise.
        auto rotatedGrid = el::String{};
        for (auto y = el::CpIndex{0}; y.isWithin(height); ++y) {
            for (auto x = el::CpIndex{0}; x.isWithin(targetWidth); ++x) {
                const auto sourceIndex = el::CpIndex::fromGrid(y, x.flipped(targetWidth), sourceWidth, height);
                rotatedGrid.append(gridCV.charAt(sourceIndex));
            }
            rotatedGrid.append(U'\n');
        }

        el::io::printLine("Original 10x10 character grid:"_el);
        el::io::printLine(grid);

        el::io::printLine("Rotated clockwise:"_el);
        el::io::printLine(rotatedGrid);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Original 10x10 character grid:
    AΩBçDÉFGH×
    IJKLMNÖPQR
    STÜVWXYZ01
    23456789ab
    cdefghijkl
    mnopqrstuv
    wxyzäöüß+-
    ∑∏√∞≈≠≤≥÷·
    ABCDEFGHIJ
    UVWXYZ!?._

    Rotated clockwise:
    UA∑wmc2SIA
    VB∏xnd3TJΩ
    WC√yoe4ÜKB
    XD∞zpf5VLç
    YE≈äqg6WMD
    ZF≠örh7XNÉ
    !G≤üsi8YÖF
    ?H≥ßtj9ZPG
    .I÷+uka0QH
    _J·-vlb1R×

.. erbsland-demo-end::

Copy on Write
=============

All string types, except literals, use copy-on-write (COW) semantics.
When you copy a string or a view, the new object shares the underlying data with the original.
The data is only copied when one of the objects is modified.

This design means that a copy behaves exactly like an independent string, even though no actual copying has taken place
yet.
You can pass strings around freely without worrying about performance.

.. erbsland-demo::
    :source: text/String/CopyOnWrite.cpp
    :exec: text/string --demo CopyOnWrite
    :source-sha256: be0c1ab709b406e5188d847f1ee31c8c782d4136f893fe0a0e00cc031cd79abf

.. code-block:: cpp

    /// Copy-on-write allows strings to be copied at almost no cost.
    /// Multiple string objects can share the same backing store until one of them
    /// is modified. At that point, only the modified string receives its own copy
    /// of the data.
    ///
    /// From the user's perspective, every string behaves like an independent value.
    /// The sharing and copying happens automatically in the background.
    ///
    /// This demo visualizes how the backing store changes as strings are copied and
    /// modified.
    void copyOnWrite() {

        // Create the original string and two copies.
        auto a = el::String{"The treasure is hidden under the old oak tree."_el};
        auto b = a;
        auto c = b;

        const auto printAll = [&]() -> void {
            el::io::printLine("a: ", a);
            el::io::printLine(el::toDebugString(a, cDebugFlags));

            el::io::printLine("b: ", b);
            el::io::printLine(el::toDebugString(b, cDebugFlags));

            el::io::printLine("c: ", c);
            el::io::printLine(el::toDebugString(c, cDebugFlags));

            el::io::printLine();
        };

        // All three strings share the same backing store.
        el::io::printLine("After copying a -> b and b -> c"_el);
        printAll();

        // Modifying b causes it to detach from the shared data.
        b.replaceAll("treasure"_el, "secret"_el);

        el::io::printLine("After modifying b"_el);
        printAll();

        // a and c still share the original backing store.
        // Modifying a creates another independent copy.
        a.append(" Nobody has found it yet."_el);

        el::io::printLine("After modifying a"_el);
        printAll();
    }

.. erbsland-ansi::
    :escape-char: ␛

    After copying a -> b and b -> c
    a: The treasure is hidden under the old oak tree.
    U8String:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414
    b: The treasure is hidden under the old oak tree.
    U8String:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414
    c: The treasure is hidden under the old oak tree.
    U8String:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414

    After modifying b
    a: The treasure is hidden under the old oak tree.
    U8String:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414
    b: The secret is hidden under the old oak tree.
    U8String:
        backingStorageId: 0x2623570ea73c9bc2:0x1a0ad47c5bd2f496
    c: The treasure is hidden under the old oak tree.
    U8String:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414

    After modifying a
    a: The treasure is hidden under the old oak tree. Nobody has found it yet.
    U8String:
        backingStorageId: 0x2623570ea6bdd942:0x1a0ad47c5a53b67b
    b: The secret is hidden under the old oak tree.
    U8String:
        backingStorageId: 0x2623570ea73c9bc2:0x1a0ad47c5bd2f496
    c: The treasure is hidden under the old oak tree.
    U8String:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414

.. erbsland-demo-end::

Thread Safety
-------------

The copy-on-write mechanism is thread-safe.
You can safely copy strings across thread boundaries.
However, concurrent access to the same string from multiple threads must be protected by the caller, just as with
``std::string``.

Rich API Design
===============

This library takes a different approach from the C++ standard library.
Instead of providing a minimal set of operations and leaving the rest to algorithms, the library offers a rich set of
methods directly on each string type.

The reasons for this design choice are:

* **Less boilerplate**: Common operations like trimming, list-based splitting,
  escaping, and case conversion are available as single method calls.
* **More readable code**: A method call like ``trimmed()`` or ``StringList::fromSplit(...)`` communicates
  intent more clearly than a range of algorithm calls.
* **Easier for junior developers**: A consistent, self-contained API reduces the cognitive load of learning the library.

.. erbsland-demo::
    :source: text/StringView/StdVsCoreSplitAndJoin.cpp
    :exec: text/string_view --demo stdVsCoreSplitAndJoin
    :source-sha256: 4aa65130bd09146bdd3332b670fc23bf80093c8f944551c424abb85f273ec3e6

.. code-block:: cpp

    /// Erbsland Core uses a rich API approach to keep boilerplate to a minimum.
    /// The common case stays short, readable, and maintainable.
    /// More dangerous and error-prone code is intentionally more explicit.
    /// This demo compares Erbsland Core code with equivalent standard-library code.
    void stdVsCoreSplitAndJoin() {
        el::io::printLine("=== Splitting text into words ==="_el);

        // Erbsland Core: Split by a character set and ignore empty noise.
        constexpr auto elWordsText = "a, b, d; e; f g h, i,, j"_el;
        const auto noiseCharSet = el::CharSet::fromPattern(" ,;"_el);
        const auto coreWords = el::StringViewList::fromSplit(elWordsText, noiseCharSet);
        el::io::printLine("Core: "_el, coreWords.join("|"_el));

        // Standard library: Express the same behavior manually.
        constexpr auto stdWordsText = std::string_view{"a, b, d; e; f g h, i,, j"};
        auto isNoise = [](const char character) -> bool {
            return character == ' ' || character == ',' || character == ';';
        };
        std::vector<std::string> stdWords;
        auto wordStart = std::string_view::npos;
        for (std::size_t index = 0; index <= stdWordsText.size(); ++index) {
            if (index < stdWordsText.size() && !isNoise(stdWordsText[index])) {
                if (wordStart == std::string_view::npos) {
                    wordStart = index;
                }
                continue;
            }

            if (wordStart != std::string_view::npos) {
                stdWords.emplace_back(stdWordsText.substr(wordStart, index - wordStart));
                wordStart = std::string_view::npos;
            }
        }
        std::string stdJoin;
        for (const auto &value : stdWords) {
            if (!stdJoin.empty()) {
                stdJoin += "|";
            }
            stdJoin += value;
        }
        el::io::printLine("std:  "_el, stdJoin);
    }

.. erbsland-ansi::
    :escape-char: ␛

    === Splitting text into words ===
    Core: a|b|d|e|f|g|h|i|j
    std:  a|b|d|e|f|g|h|i|j

.. erbsland-demo-end::

Unicode Support
===============

All string types operate on Unicode code points rather than encoded byte sequences.

The :cpp:class:`Char <erbsland::text::Char>` type represents a single 32-bit Unicode code point and provides utilities
for character classification, case conversion, and Unicode-related queries.

UTF-8 and UTF-16 input is decoded using a fault-tolerant strategy.
Malformed encoding sequences are automatically replaced with the Unicode replacement character
(:cpp:func:`Char::replacement <erbsland::text::Char::replacement>`), allowing text processing to continue without
interruption.

As a result, string operations always produce deterministic results, even when the input contains damaged or partially
invalid Unicode data.
This behaviour is especially useful when processing untrusted input, imported files, or text received from external
systems where encoding errors may occur.

When you need stricter control, string conversion functions provide an
:cpp:enum:`EncodingErrorMode <erbsland::text::EncodingErrorMode>` parameter. This allows you to select
different error-handling strategies, ranging from automatic replacement to more restrictive validation modes.
