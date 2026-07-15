
.. index::
    !single: Character Access and Parsing of Strings
    single: StringCharReader
    single: StringCharView
    single: StringView
    single: String
    single: StringLiteral
    single: U8StringView
    single: U8String
    single: U16StringView
    single: U16String
    single: U32StringView
    single: U32String
    single: Code-Point Access
    single: Byte Index
    single: Code-Point Index
    single: Sequential String Parsing

***************************************
Character Access and Parsing of Strings
***************************************

Most string-processing code should not care how text is encoded.

Use :cpp:type:`StringView <erbsland::text::StringView>` for ordinary read-only text.
It accepts UTF-8 strings through a single interface and allows callers to pass
:cpp:type:`String <erbsland::text::String>` and
:cpp:type:`StringLiteral <erbsland::text::StringLiteral>` without
copying.
Reach for
:cpp:class:`U8StringView <erbsland::text::U8StringView>`,
:cpp:class:`U16StringView <erbsland::text::U16StringView>`, or
:cpp:class:`U32StringView <erbsland::text::U32StringView>` only when the
storage encoding itself is relevant to the problem.

This page explains how to work with decoded Unicode characters rather than raw storage units.
You will learn when to use a parser cursor, when simple iteration is the best solution, and when indexed access is
appropriate.

Three access patterns cover almost all string-processing tasks:

- Use :cpp:class:`StringCharReader <erbsland::text::StringCharReader>`
  for parsers and sequential readers.
- Use :cpp:func:`forEach() <erbsland::text::U8StringView::forEach>` or
  range-based iteration when you inspect every decoded character.
- Use code-point indexed access only when an external API already provides an index,
  when you need direct access to the first or last character, or when working with small strings.

Choose an Access Pattern
========================

Choose the access pattern that matches the algorithm.

A parser naturally moves forward through text one character at a time.
A text inspection usually visits every character.
Indexed access is appropriate when positions already exist outside the algorithm, such as search results, diagnostics,
or user-interface coordinates.

In practice:

- Use :cpp:class:`StringCharReader <erbsland::text::StringCharReader>` for parsers.
  It provides efficient sequential access, lookahead, recovery, and code-point positions for diagnostics.
  It also provides capturing of strings and low-level integer parsing.
- Use :cpp:func:`forEach() <erbsland::text::U8StringView::forEach>` or a range-based ``for`` loop when
  every decoded character is visited.
- Use byte indexes together with :cpp:func:`find() <erbsland::text::U8StringView::find>` and similar functions,
  and use :cpp:func:`advance() <erbsland::text::U8StringView::advance>` and
  :cpp:func:`retreat() <erbsland::text::U8StringView::retreat>` to move the index.
  Do not manipulate UTF-8 byte positions manually.
- Use code-point indexes when character positions are part of the problem domain,
  such as diagnostics or editor positions.
- Use :cpp:type:`StringCharView <erbsland::text::StringCharView>` when your data model is naturally character-indexed,
  the additional scanning cost is acceptable and conversion to UTF-32 adds to much complexity.

Decoded Characters, Byte Indexes, and Code-Point Indexes
========================================================

The library exposes two fundamentally different position types.

A ``ByteIndex`` identifies a position in the encoded string representation.
For UTF-8 strings, byte indexes are the native and efficient way to reference locations inside the stored text.

A ``CpIndex`` identifies a decoded Unicode character position.
This is usually the position users expect in diagnostics and editor interfaces.

Most APIs that work with parsing and diagnostics use code-point positions.
Most APIs that work with string storage and slicing use byte positions.

Understanding this distinction makes it easier to choose the correct access pattern throughout the rest of this page.

Parse Text with ``StringCharReader``
====================================

:cpp:class:`StringCharReader <erbsland::text::StringCharReader>` accepts
:cpp:class:`U8String <erbsland::text::U8String>` /
:cpp:class:`U8StringView <erbsland::text::U8StringView>`,
:cpp:class:`U16String <erbsland::text::U16String>` /
:cpp:class:`U16StringView <erbsland::text::U16StringView>`, and
:cpp:class:`U32String <erbsland::text::U32String>` /
:cpp:class:`U32StringView <erbsland::text::U32StringView>`.
Parser helpers can therefore work on all string widths without templates or overloads.

Use the reader as a cursor:

- :cpp:func:`peek() <erbsland::text::StringCharReader::peek>` looks at the next decoded character.
- :cpp:func:`read() <erbsland::text::StringCharReader::read>` reads and advances.
- :cpp:func:`advance() <erbsland::text::StringCharReader::advance>` skips decoded characters.
- :cpp:func:`readIf() <erbsland::text::StringCharReader::readIf>` and
  :cpp:func:`advanceIf() <erbsland::text::StringCharReader::advanceIf>` consume optional grammar characters while
  keeping the cursor unchanged on a non-match.
- :cpp:func:`isAtEnd() <erbsland::text::StringCharReader::isAtEnd>` and
  :cpp:func:`canRead() <erbsland::text::StringCharReader::canRead>` make end checks explicit.
- ``OrThrow`` variants are useful when the grammar has already proven that a character must exist and invalid encoding
  should be an exception.
- :cpp:func:`readWhile() <erbsland::text::StringCharReader::readWhile>` or
  :cpp:func:`readUntil() <erbsland::text::StringCharReader::readUntil>` to read text blocks that match certain
  characters.
- :cpp:func:`startCapture() <erbsland::text::StringCharReader::startCapture>` and
  :cpp:func:`takeCapture() <erbsland::text::StringCharReader::takeCapture>` to efficiently capture slices of the
  original text.
- :cpp:func:`parseInteger() <erbsland::text::StringCharReader::parseInteger>` or
  :cpp:func:`readIntegerOrThrow() <erbsland::text::StringCharReader::readIntegerOrThrow>` reads integer tokens
  and restores the original position on failure.

When a grammar branch is optional, call :cpp:func:`save() <erbsland::text::StringCharReader::save>` before trying it and
:cpp:func:`restore() <erbsland::text::StringCharReader::restore>` if the branch fails.
When a helper only needs lookahead, pass the reader by value.
Reader copies share the immutable source but keep independent cursor positions.

Use :cpp:func:`position() <erbsland::text::StringCharReader::position>` for parse errors.
It returns a decoded code-point index, which is the unit users expect when they count characters in a message.

.. erbsland-demo::
    :source: text/StringCharReader/StudyPlanParser.cpp
    :exec: text/string_char_reader --demo StudyPlanParser
    :source-sha256: d6144db7031b549d50a2dcf2cd454f99081227a6be107adeea30913074b95ee8

.. code-block:: cpp

    /// `StringCharReader` is the preferred tool for character-by-character parsers.
    ///
    /// - It decodes UTF-8, UTF-16, and UTF-32 through the same API.
    /// - It keeps an efficient sequential cursor.
    /// - It can save or restore its state when a parser tries an optional grammar branch.
    /// - It can capture strings as efficient views to slices of the original text.
    ///
    /// This demo parses a small Turkish study plan.
    /// Helper functions receive `StringCharReader` by reference when they consume input.
    void studyPlanParser() {
        const auto utf8Plan = el::StringView{"matematik:45;fen:30;müzik"_el};
        printPlan("UTF-8 plan"_el, el::StringCharReader{utf8Plan});

        const auto utf16Plan = el::U16StringView{u"geometri:25;şiir:15"_el};
        printPlan("UTF-16 plan"_el, el::StringCharReader{utf16Plan});

        const auto utf32Plan = el::U32StringView{U"astronomi:40;çizim:20"_el};
        printPlan("UTF-32 plan"_el, el::StringCharReader{utf32Plan});

        const auto draftWithError = el::StringView{"tarih:25;kimya:x"_el};
        printPlan("plan with diagnostic"_el, el::StringCharReader{draftWithError});
    }

    void printPlan(const el::StringView &label, el::StringCharReader reader) {
        el::io::printLine(label, ":"_el);
        while (!reader.isAtEnd()) {
            const auto lessonStart = reader.position();
            const auto lessonName = readLessonName(reader);
            if (lessonName.isEmpty()) {
                el::io::printLine("  error at index "_el, lessonStart, ": expected lesson name"_el);
                return;
            }

            int minutes = 0;
            if (readOptionalMinutes(reader, minutes)) {
                el::io::printLine("  - "_el, lessonName, ": ", minutes, " min"_el);
            } else {
                el::io::printLine("  - "_el, lessonName, ": no duration"_el);
            }

            if (reader.readIf(U';')) {
                continue;
            }
            if (!reader.isAtEnd()) {
                el::io::printLine(
                    "  error at code point "_el, reader.position(), ": unexpected '"_el, reader.peek(), "'"_el);
                return;
            }
        }
    }

    auto readLessonName(el::StringCharReader &reader) -> el::String {
        const static auto separatorChars = el::CharSet{U':', U';'};
        reader.startCapture();
        reader.readUntil({}, separatorChars);
        return reader.takeCapture().toU8String();
    }

    auto readOptionalMinutes(el::StringCharReader &reader, int &minutes) -> bool {
        if (!reader.readIf(U':')) {
            return false;
        }
        try {
            minutes = reader.readIntegerOrThrow<int>(decMax3Digits);
            return true;
        } catch (const el::Exception &) {
            return false;
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    UTF-8 plan:
      - matematik: 45 min
      - fen: 30 min
      - müzik: no duration
    UTF-16 plan:
      - geometri: 25 min
      - şiir: 15 min
    UTF-32 plan:
      - astronomi: 40 min

.. erbsland-demo-end::

Iterate Over Decoded Characters
===============================

If your code only needs to visit decoded characters, use the iteration APIs instead of creating a parser.
They are shorter, make the intent clearer, and avoid accidental byte-index mistakes.

Use :cpp:func:`forEach() <erbsland::text::U8StringView::forEach>` as a convenience method, visiting every character in
the string.
If your callback returns ``LoopStatus::Stop``, the iteration stops early and returns ``LoopResult::Stopped``.
Especially if you test for a condition, using :cpp:func:`forEach() <erbsland::text::U8StringView::forEach>` is often the
most efficient way of implementation.

A simple ``for (Char character : myString) { ... }`` loop is effective too, compared to the ``forEach`` call it works
with iterators that need to do a bit of extra work in sake of safety.

Both forms decode invalid UTF-8 as :cpp:func:`Char::replacement() <erbsland::text::Char::replacement>` for UTF-8
``StringView`` operations.
Validate input first when malformed encoding should be rejected rather than tolerated.

.. erbsland-demo::
    :source: text/StringView/IteratingCharacters.cpp
    :exec: text/string_view --demo IteratingCharacters
    :source-sha256: d8bfa0a20835d7c2de1d48e3b262e17db979c735d2e876be2db93f74c9b37d16

.. code-block:: cpp

    /// `StringView::forEach()` and range-based `for` loops decode text as Unicode
    /// code points without exposing UTF-8 byte boundaries.
    ///
    /// Use `forEach()` when the callback may stop early with `LoopStatus::Stop`.
    /// Use a range-based `for` loop when all decoded characters should be visited
    /// and the loop body is clearer than a callback.
    void iteratingCharacters() {
        const auto plan = el::StringView{"matematik:45;fen:30;müzik:20"_el};

        auto lettersBeforeBreak = el::CpLength::zero();
        auto digitsBeforeBreak = el::CpLength::zero();

        // Stop as soon as the first lesson entry ends.
        plan.forEach([&](const el::Char character) -> el::LoopStatus {
            if (character == U';') {
                return el::LoopStatus::Stop;
            }
            if (character.isAsciiLetter()) {
                ++lettersBeforeBreak;
            } else if (character.isAsciiDigit()) {
                ++digitsBeforeBreak;
            }
            return el::LoopStatus::Continue;
        });

        el::io::printLine("Before the first separator:"_el);
        el::io::printLine("  ASCII letters: "_el, lettersBeforeBreak);
        el::io::printLine("  ASCII digits: "_el, digitsBeforeBreak);

        auto uppercased = el::String{};

        // Range-based iteration is compact when every decoded character is needed.
        // Note: Use `String::transformed` to uppercase/lowercase transformations in production code.
        for (const auto character : el::StringView{"ödev: çizim"_el}) {
            uppercased.append(character.toUppercase());
        }

        el::io::printLine("Uppercase walk: "_el, uppercased);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Before the first separator:
      ASCII letters: 9
      ASCII digits: 2
    Uppercase walk: ÖDEV: ÇIZIM

.. erbsland-demo-end::

Use Indexed Access When Positions Matter
========================================

Index-based access is sometimes the clearest solution, but each index kind has different costs and guarantees.
The most important rule is simple: byte indexes are native for UTF-8 storage; code-point indexes are semantic but may
require a scan.

Fast Side Access
----------------

Use :cpp:func:`charAt(StringSide) <erbsland::text::U8StringView::charAt>` when you need the first or last character.
This avoids spelling out the index and lets the string implementation choose the efficient path.

Use :cpp:func:`indexAt(StringSide) <erbsland::text::U8StringView::indexAt>` to get the first byte index or the byte
index after the last byte.
The back index is an end position, not the last character's start position.

Byte-Index Access
-----------------

Use :cpp:func:`charAt(ByteIndex) <erbsland::text::U8StringView::charAt>` when the byte index came from another
``StringView`` operation, from a saved byte position, or from
:cpp:func:`advance() <erbsland::text::U8StringView::advance>` /
:cpp:func:`retreat() <erbsland::text::U8StringView::retreat>`.

The operation is safe:

- If the byte index points to a valid character start, the decoded character is returned.
- If the byte index points inside a malformed or misplaced UTF-8 sequence, a replacement character is returned.
- If the byte index is exactly at the end, ``Char::endOfData()`` is returned.
- If the byte index is outside the visible range, ``Char::noCodePoint()`` is returned.

If you do no own length validation, always test signal characters before treating the result as regular text.

Code-Point Index Access
-----------------------

Use :cpp:func:`charAt(CpIndex) <erbsland::text::U8StringView::charAt>` when the position is already a code-point
position and the string is small enough that scanning is acceptable.
The same guidance applies to :cpp:func:`indexAt(CpIndex) <erbsland::text::U8StringView::indexAt>` and
:cpp:func:`toCharIndex(ByteIndex) <erbsland::text::U8StringView::toCharIndex>`.

Do not parse a UTF-8 string with a loop like "for each code-point index, call ``charAt(CpIndex)`` ".
That repeats a scan for every character.
Use :cpp:class:`StringCharReader <erbsland::text::StringCharReader>`,
:cpp:func:`forEach() <erbsland::text::U8StringView::forEach>`, or range-based iteration instead.

Slicing and Moving Indexes
--------------------------

Use :cpp:func:`slice(StringSide) <erbsland::text::U8StringView::slice>` when you need to peel one decoded character from
the front or back and continue with the remaining view.
This is useful for rules such as "the first character has different requirements than the rest".

Use :cpp:func:`advance() <erbsland::text::U8StringView::advance>` and
:cpp:func:`retreat() <erbsland::text::U8StringView::retreat>` to move byte indexes by decoded characters.
They keep index movement in the string API, where malformed encoding and boundary conditions can be handled
consistently.

.. erbsland-demo::
    :source: text/StringView/CharacterAccess.cpp
    :exec: text/string_view --demo CharacterAccess
    :source-sha256: 0bd10d5e3e2893fc5b4956f21bafd1d20194462e042283ca04e0b4005c7b5837

.. code-block:: cpp

    /// `StringView` gives fast access to UTF-8 byte positions and explicit tools for
    /// moving between decoded code points.
    ///
    /// Use `charAt(StringSide)` for the first or last character, use byte indexes
    /// when a previous string operation already returned one, and use
    /// `advance()`/`retreat()` to keep byte indexes on character boundaries.
    /// Code-point indexes are convenient but require scanning UTF-8 text; reserve
    /// them for small strings or specialized code.
    void characterAccess() {
        const auto text = el::StringView{"ödev📚:matematik"_el};

        el::io::printLine("Text: "_el, text);
        el::io::printLine("First character: "_el, describeCharacter(text.charAt(el::StringSide::Front)));
        el::io::printLine("Last character: "_el, describeCharacter(text.charAt(el::StringSide::Back)));

        // Byte indexes are fast, but the index must point to the start of a UTF-8 sequence.
        auto byteIndex = el::ByteIndex::zero();
        el::io::printLine("Byte index 0: "_el, describeCharacter(text.charAt(byteIndex)));
        el::io::printLine("Byte index 1: "_el, describeCharacter(text.charAt(el::ByteIndex{1U})));
        el::io::printLine("End byte index: "_el, describeCharacter(text.charAt(text.indexAt(el::StringSide::Back))));
        el::io::printLine("Outside byte range: "_el,
            describeCharacter(text.charAt(text.indexAt(el::StringSide::Back) + el::ByteLength{8U})));

        // Move byte indexes by decoded code points instead of incrementing raw bytes.
        text.advance(byteIndex, el::CpLength{4U});
        el::io::printLine(
            "After advancing four characters: byte "_el, byteIndex, " -> ", describeCharacter(text.charAt(byteIndex)));
        text.retreat(byteIndex);
        el::io::printLine(
            "After retreating one character: byte "_el, byteIndex, " -> ", describeCharacter(text.charAt(byteIndex)));

        // Code-point indexes are useful for diagnostics and tiny strings, but they scan UTF-8 text.
        const auto cpIndex = text.toCharIndex(byteIndex);
        el::io::printLine("Current byte index is code-point index "_el, cpIndex);
        el::io::printLine("Code-point index 5 starts at byte "_el, text.indexAt(el::CpIndex{5U}));
        el::io::printLine("Character at code-point index 5: "_el, describeCharacter(text.charAt(el::CpIndex{5U})));

        // Slicing one character from a side is efficient and keeps the rest as a view.
        const auto [firstCharacter, withoutFirst] = text.slice(el::StringSide::Front);
        const auto [lastCharacter, withoutLast] = text.slice(el::StringSide::Back);
        el::io::printLine("Sliced front: "_el, describeCharacter(firstCharacter), " | rest: "_el, withoutFirst);
        el::io::printLine("Sliced back: "_el, describeCharacter(lastCharacter), " | rest: "_el, withoutLast);

        // `StringCharView` exposes character-indexed helpers for specialized small-text work.
        auto charView = text.toCharView();
        el::io::printLine("Char view prefix: "_el, charView.slice(el::StringSide::Front, el::CpLength{4U}));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Text: ödev📚:matematik
    First character: 'ö'
    Last character: 'k'
    Byte index 0: 'ö'
    Byte index 1: '�'
    End byte index: end-of-data
    Outside byte range: no-code-point
    After advancing four characters: byte 5 -> '📚'
    After retreating one character: byte 4 -> 'v'
    Current byte index is code-point index 3
    Code-point index 5 starts at byte 9
    Character at code-point index 5: ':'
    Sliced front: 'ö' | rest: dev📚:matematik
    Sliced back: 'k' | rest: ödev📚:matemati
    Char view prefix: ödev

.. erbsland-demo-end::

Use ``StringCharView`` Only for Character-Position Models
=========================================================

:cpp:type:`StringCharView <erbsland::text::StringCharView>` is created with
:cpp:func:`toCharView() <erbsland::text::U8StringView::toCharView>`.
It exposes a character-indexed interface with code-point indexes and character-indexed find, slice, and transform
helpers.

Use it when your algorithm is naturally expressed in character positions, for example a small text grid, editor column,
diagnostic marker, or fixed-width display calculation.
Do not use it to make parsers look simpler.
For UTF-8 and UTF-16, character-indexed access is still implemented by scanning from the start or from nearby known
boundaries.

Code-Point Access is Cheap for UTF-32 Strings
=============================================

UTF-32 makes code-point-index access cheap because one storage unit is one code point.
That does not automatically make conversion to :cpp:class:`U32String <erbsland::text::U32String>` worthwhile.
Convert only when the data will be accessed by code-point index often enough to justify the extra memory, conversion
cost, and API width change.

Dos and Don'ts
==============

Do accept :cpp:type:`StringView <erbsland::text::StringView>` for ordinary read-only text.
It lets callers pass :cpp:type:`String <erbsland::text::String>`,
:cpp:type:`StringLiteral <erbsland::text::StringLiteral>`, and UTF-8 literals without copying.

Do write parsers around :cpp:class:`StringCharReader <erbsland::text::StringCharReader>`.
Keep parser functions independent of UTF-8, UTF-16, and UTF-32 unless the grammar itself depends on encoding units.

Do use :cpp:func:`position() <erbsland::text::StringCharReader::position>` for user-facing parse diagnostics, and keep
byte indexes for internal string operations.

Do use :cpp:func:`forEach() <erbsland::text::U8StringView::forEach>` or a range-based ``for`` loop for simple character
walks.

Do use :cpp:func:`advance() <erbsland::text::U8StringView::advance>` and
:cpp:func:`retreat() <erbsland::text::U8StringView::retreat>` to move byte indexes by decoded code points.

Don't increment a UTF-8 byte index and assume the next byte starts the next character.

Don't use repeated :cpp:func:`charAt(CpIndex) <erbsland::text::U8StringView::charAt>` calls as a parser or scanner for
large UTF-8 text.

Don't convert to :cpp:class:`U32String <erbsland::text::U32String>` just to make one or two character-indexed accesses.
Use UTF-32 when code-point indexing is central to the data structure.
