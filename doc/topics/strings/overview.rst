..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Strings
    single: StringEditor
    single: String
    single: StringLiteral
    single: AnyStringBuilder
    single: Char
    single: U8StringEditor
    single: U16StringEditor
    single: U32StringEditor
    single: U8String
    single: U16String
    single: U32String
    single: U8StringLiteral
    single: U16StringLiteral
    single: U32StringLiteral
    single: StringFormat
    single: Copy on Write
    single: Thread Safety
    single: Unicode Support
    single: UTF-8
    single: UTF-16
    single: UTF-32
    single: EncodingMode

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

UTF-16 is larger than UTF-8 for ASCII content but smaller than UTF-32. Surrogate pairs complicate character indexing, so
indexed code-point access may require scanning surrogate pairs.

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
The common aliases :cpp:type:`StringEditor <erbsland::text::StringEditor>`, :cpp:type:`String <erbsland::text::String>`,
:cpp:type:`StringLiteral <erbsland::text::String>`, and :cpp:type:`StringFormat <erbsland::text::StringFormat>`
are all UTF-8 types.
For most applications, you only ever need these aliases.

.. _string-types-overview:

String Types
============

This library provides a read-only owning value, a mutable editor and a literal type for each encoding.
The table below shows the full set.

.. list-table::
    :width: 100%
    :header-rows: 1

    *   -   –
        -   **UTF-8**
        -   **UTF-16**
        -   **UTF-32**
    *   -   **Read-only string**
        -   |   :cpp:type:`String <erbsland::text::String>`
            |   (:cpp:class:`U8String <erbsland::text::U8String>`)
        -   :cpp:class:`U16String <erbsland::text::U16String>`
        -   :cpp:class:`U32String <erbsland::text::U32String>`
    *   -   **Owning string editor**
        -   |   :cpp:type:`StringEditor <erbsland::text::StringEditor>`
            |   (:cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>`)
        -   :cpp:class:`U16StringEditor <erbsland::text::U16StringEditor>`
        -   :cpp:class:`U32StringEditor <erbsland::text::U32StringEditor>`
    *   -   **String literal**
        -   |   :cpp:type:`StringLiteral <erbsland::text::StringLiteral>`
            |   (:cpp:class:`U8StringLiteral <erbsland::text::U8StringLiteral>`)
        -   :cpp:class:`U16StringLiteral <erbsland::text::U16StringLiteral>`
        -   :cpp:class:`U32StringLiteral <erbsland::text::U32StringLiteral>`

The aliases :cpp:type:`StringEditor <erbsland::text::StringEditor>`, :cpp:type:`String <erbsland::text::String>` and
:cpp:type:`StringLiteral <erbsland::text::String>` refer to the UTF-8 types and are the ones you will use most often.

Owning Read-only Strings
------------------------

Read-only strings are owning values that either share string data or refer to static literal storage.
They are the primary type for storing, passing and returning completed text.

A string internally stores either shared string data or a reference to the read-only memory of a string literal.
Slices safely share their source storage because the value keeps that storage alive.

.. erbsland-demo::
    :source: text/String/IdealFunctionParameter.cpp
    :exec: text/string --demo IdealFunctionParameter
    :source-sha256: 1174794885cf3e3869b50c12bebb9246f83997dd0f2fa21ee42a3be804f58d28

.. code-block:: cpp

    /// `String` is the preferred parameter type for functions that read text.
    ///
    /// It accepts common string inputs naturally:
    /// - string literals are used directly without copying,
    /// - existing strings share their backend,
    /// - editors provide a read-only value to their contents.
    ///
    /// From the caller's perspective, all variants behave the same.
    void idealFunctionParameter() {
        // A string literal can be passed directly.
        countEmojis("🌲🌲 Waldkonzert mit Fuchs 🦊 und Eule 🦉"_el);

        // An existing string can be passed without copying.
        const auto string = el::String{"Pluie douce sur les fleurs 🌧️🌷🌼"_el};
        countEmojis(string);

        // An editable string is accepted as read-only input.
        const auto stringEdit = el::StringEditor{"Bosque nocturno: luna 🌙, estrellas ✨ y grillos 🦗"_el};
        countEmojis(stringEdit);
    }

    /// Count all emoji-like symbols in `text` and print the result.
    ///
    /// The function only needs read-only access to the text. It does not need to know
    /// whether the caller passed a literal, a string, or an editor.
    void countEmojis(const el::String &text) {
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

Read-only strings are the preferred type for parameters, stored text and return values in library APIs.
Unlike ``std::string_view``, they own or share the storage that guarantees their lifetime.

String Literals
---------------

String literals are wrappers around compile-time string literals, created with the ``""_el`` suffix.
They are zero-cost references to read-only memory and can be converted to strings without allocation.

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

Literals are primarily used as function arguments, constant expressions and as the source for strings or editors.
For most use cases, ``""_el`` is the recommended suffix.
Construct ``String{literal}`` or ``StringEditor{literal}`` explicitly when the literal type itself is insufficient.

Editable Strings
----------------

Owning strings are mutable, copy-on-write containers that own their data.
Use them when you need to modify, append, or store strings that outlive their source.

.. erbsland-demo::
    :source: text/StringEditor/EditingText.cpp
    :exec: text/string --demo EditingText
    :source-sha256: a07d080bec20b97c5a0d0da84850de1224168babb27b6c86c1b1861ee09c76b5

.. code-block:: cpp

    /// `StringEditor` is an owning, editable copy-on-write string type.
    /// Use it when you build text from scratch or modify existing text.
    /// Use `String` for parameters and stored read-only text.
    void editingText() {
        // Create an editable string from a string literal.
        auto story = el::StringEditor{"The frost lifts from the valley. A pale crocus opens beside the stone. "
                                "Der Wind trägt Blätter durch die Luft."_el};

        // Keep completed, unmodified text as a read-only value.
        auto intro = el::String{"A short alpine field note:"_el};

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

Creating an editor from a read-only string or literal creates editable copy-on-write storage.
Use editors only when you need to change text in place or build it incrementally.
For stored and completed text, use
:cpp:type:`String <erbsland::text::String>`.

If you need a more generic approach building strings, you may find the
:cpp:class:`AnyStringBuilder <erbsland::text::AnyStringBuilder>` class useful.
It has one interface to build all three string types.

Copy on Write
=============

All string types, except literals, use copy-on-write (COW) semantics.
When you copy a string or an editor, the new object shares the underlying data with the original.
The data is only copied when one of the objects is modified.

This design means that a copy behaves exactly like an independent string, even though no actual copying has taken place
yet.
You can pass strings around freely without worrying about performance.

.. erbsland-demo::
    :source: text/StringEditor/CopyOnWrite.cpp
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
        auto a = el::StringEditor{"The treasure is hidden under the old oak tree."_el};
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
    U8StringEditor:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414
    b: The treasure is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414
    c: The treasure is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414

    After modifying b
    a: The treasure is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414
    b: The secret is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570ea73c9bc2:0x1a0ad47c5bd2f496
    c: The treasure is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570ea73c9b42:0x1a0ad47c5bd2f414

    After modifying a
    a: The treasure is hidden under the old oak tree. Nobody has found it yet.
    U8StringEditor:
        backingStorageId: 0x2623570ea6bdd942:0x1a0ad47c5a53b67b
    b: The secret is hidden under the old oak tree.
    U8StringEditor:
        backingStorageId: 0x2623570ea73c9bc2:0x1a0ad47c5bd2f496
    c: The treasure is hidden under the old oak tree.
    U8StringEditor:
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
* **More readable code**: A method call like ``trimmed()`` or ``StringEditorList::fromSplit(...)`` communicates
  intent more clearly than a range of algorithm calls.
* **Easier for junior developers**: A consistent, self-contained API reduces the cognitive load of learning the library.

.. erbsland-demo::
    :source: text/String/StdVsCoreSplitAndJoin.cpp
    :exec: text/string --demo stdVsCoreSplitAndJoin
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
        const auto coreWords = el::StringList::fromSplit(elWordsText, noiseCharSet);
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

Every :cpp:class:`StringConverter <erbsland::text::StringConverter>` conversion accepts an
:cpp:enum:`EncodingMode <erbsland::text::EncodingMode>` parameter.
Use ``Strict`` to validate and reject malformed source text or keep the default ``Tolerant`` mode for fast conversion.
Once text is stored in an Erbsland string, string operations stay tolerant; call ``isValidUtf8()``, ``isValidUtf16()``,
or ``isValidUtf32()`` explicitly when an application needs to validate internal text.
