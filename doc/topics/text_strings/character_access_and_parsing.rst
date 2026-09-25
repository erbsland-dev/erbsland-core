
.. index::
    !single: Character Access and Parsing of Strings
    single: StringCharReader
    single: String
    single: StringEditor
    single: StringLiteral
    single: U8String
    single: U8StringEditor
    single: U16String
    single: U16StringEditor
    single: U32String
    single: U32StringEditor
    single: Code-Point Access
    single: Byte Index
    single: Code-Point Index
    single: Sequential String Reading

***************************************
Character Access and Parsing of Strings
***************************************

Reading text one character at a time sounds straightforward until the text uses a variable-width encoding.
An index into UTF-8 storage counts bytes, while the position shown in a diagnostic usually counts decoded code points.
A parser, meanwhile, rarely wants random access at all; it wants to move forward, look ahead, and retain a slice of what
it just consumed.

This page approaches character access through those real algorithms.
You will see when ordinary iteration is enough, when :cpp:class:`StringCharReader <erbsland::text::StringCharReader>`
provides useful parser state, when a native index is the right lightweight sequential cursor, and when an existing byte
or code-point position makes direct access appropriate.
The examples begin with the common UTF-8 :cpp:type:`String <erbsland::text::String>`, then explain where the explicit
UTF-16 and UTF-32 types change the cost model.

Start with the Way Your Algorithm Reads Text
============================================

A parser naturally moves from left to right and occasionally looks ahead.
``StringCharReader`` matches that flow and can capture shared slices while retaining positions for diagnostics.
An inspection that visits every character is simpler as a range-based loop or ``forEach()`` call.
An algorithm that must resume from a stored position or walk backward can carry one native index without becoming a
parser.

Direct indexing belongs to the less common case where a meaningful position already exists: perhaps a search returned a
``ByteIndex``, a diagnostic stores a code-point position, or a short fixed-format value needs its first or last
character.
Keeping that origin in mind prevents an algorithm from repeatedly translating positions or rescanning from the start.

Understand the Coordinate Systems
=================================

The library exposes two fundamentally different position types.

A ``ByteIndex`` identifies a position in the encoded string representation.
For UTF-8 strings, byte indexes are the native and efficient way to reference locations inside the stored text.

A ``CpIndex`` identifies a decoded Unicode character position.
This is usually the position users expect in diagnostics and editor interfaces.

Most APIs that work with parsing and diagnostics use code-point positions.
Most APIs that work with string storage and slicing use byte positions.

Understanding this distinction makes it easier to choose the correct access pattern throughout the rest of this page.

Keep Parser State in ``StringCharReader``
=========================================

:cpp:class:`StringCharReader <erbsland::text::StringCharReader>` accepts
:cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>` /
:cpp:class:`U8String <erbsland::text::U8String>`,
:cpp:class:`U16StringEditor <erbsland::text::U16StringEditor>` /
:cpp:class:`U16String <erbsland::text::U16String>`, and
:cpp:class:`U32StringEditor <erbsland::text::U32StringEditor>` /
:cpp:class:`U32String <erbsland::text::U32String>`.
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
    :source-sha256: a7a1164a22aee500ded309400ff7f5ed22498d8c823d58bc2816057f1f3704f8

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
        const auto utf8Plan = el::String{"matematik:45;fen:30;müzik"_el};
        printPlan("UTF-8 plan"_el, el::StringCharReader{utf8Plan});

        const auto utf16Plan = el::U16String{u"geometri:25;şiir:15"_el};
        printPlan("UTF-16 plan"_el, el::StringCharReader{utf16Plan});

        const auto utf32Plan = el::U32String{U"astronomi:40;çizim:20"_el};
        printPlan("UTF-32 plan"_el, el::StringCharReader{utf32Plan});

        const auto draftWithError = el::String{"tarih:25;kimya:x"_el};
        printPlan("plan with diagnostic"_el, el::StringCharReader{draftWithError});
    }

    void printPlan(const el::String &label, el::StringCharReader reader) {
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
      - çizim: 20 min
    plan with diagnostic:
      - tarih: 25 min
      - kimya: no duration
      error at code point 15: unexpected 'x'

.. erbsland-demo-end::

Walk Through Every Decoded Character
====================================

If your code only needs to visit decoded characters, use the iteration APIs instead of creating a parser.
They are shorter, make the intent clearer, and avoid accidental byte-index mistakes.

Use :cpp:func:`forEach() <erbsland::text::U8String::forEach>` as a convenience method, visiting every character in the
string.
If your callback returns ``LoopStatus::Stop``, the iteration stops early and returns ``LoopResult::Stopped``.
Especially if you test for a condition, using :cpp:func:`forEach() <erbsland::text::U8String::forEach>` is often the
most efficient way of implementation.

A simple ``for (Char character : myString) { ... }`` loop is effective too, compared to the ``forEach`` call it works
with iterators that need to do a bit of extra work in sake of safety.

Both forms decode invalid UTF-8 as :cpp:func:`Char::replacement() <erbsland::text::Char::replacement>` for UTF-8
``String`` operations.
Validate input first when malformed encoding should be rejected rather than tolerated.

.. erbsland-demo::
    :source: text/String/IteratingCharacters.cpp
    :exec: text/string --demo IteratingCharacters
    :source-sha256: 45e4a67ce026432e3876467fb87b7f3b4c9b63f58f83be11d16226bc828a0891

.. code-block:: cpp

    /// `String::forEach()` and range-based `for` loops decode text as Unicode
    /// code points without exposing UTF-8 byte boundaries.
    ///
    /// Use `forEach()` when the callback may stop early with `LoopStatus::Stop`.
    /// Use a range-based `for` loop when all decoded characters should be visited
    /// and the loop body is clearer than a callback.
    void iteratingCharacters() {
        const auto plan = el::String{"matematik:45;fen:30;müzik:20"_el};

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

        auto uppercased = el::StringEditor{};

        // Range-based iteration is compact when every decoded character is needed.
        // Note: Use `String::transformed` to uppercase/lowercase transformations in production code.
        for (const auto character : el::String{"ödev: çizim"_el}) {
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

Read Sequentially Without Parser State
======================================

Some algorithms need more control than a complete iteration but none of the capture, lookahead, or diagnostic state of
``StringCharReader``.
They may already carry a native index, stop and resume at that position, or need to walk the text from the end toward
the beginning.

``readCharAndAdvance(index)`` decodes the character at a native index and moves the index to the following character.
For UTF-8, begin with ``ByteIndex::zero()``; UTF-16 uses ``U16DataIndex`` and UTF-32 uses ``CpIndex``.
``readCharAndRetreat(index)`` performs the reverse operation: initialize the index with ``indexAt(StringSide::Back)``,
read the character before it, and leave the index at that character's start.
This reverse traversal is not provided by ``StringCharReader`` or ``forEach()``.

Both calls return a signal character at a boundary and leave the index unchanged when no character can be read.
Testing ``isEndOfData()`` therefore gives a natural loop condition.
Because the index advances in the encoding's native units, each code point is decoded once without translating repeated
code-point positions.

.. erbsland-demo::
    :source: text/String/SequentialCharacterReading.cpp
    :exec: text/string --demo SequentialCharacterReading
    :source-sha256: e1e00942e468e52254b94658bf65f164b6fcb57627619d4f720123fc36d9701e

.. code-block:: cpp

    /// Read UTF-8 sequentially in either direction while keeping one native byte index.
    void sequentialCharacterReading() {
        const auto label = el::String{"veld🌱"_el};

        auto forwardIndex = el::ByteIndex::zero();
        auto forward = el::StringEditor{};
        while (true) {
            const auto character = label.readCharAndAdvance(forwardIndex);
            if (character.isEndOfData()) {
                break;
            }
            forward.append(character);
        }

        auto reverseIndex = label.indexAt(el::StringSide::Back);
        auto reverse = el::StringEditor{};
        while (true) {
            const auto character = label.readCharAndRetreat(reverseIndex);
            if (character.isEndOfData()) {
                break;
            }
            reverse.append(character);
        }

        el::io::printLine("Text ...........: "_el, label);
        el::io::printLine("Forward ........: "_el, forward);
        el::io::printLine("Reverse ........: "_el, reverse);
        el::io::printLine("Forward index ..: "_el, forwardIndex);
        el::io::printLine("Reverse index ..: "_el, reverseIndex);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Text ...........: veld🌱
    Forward ........: veld🌱
    Reverse ........: 🌱dlev
    Forward index ..: 8
    Reverse index ..: 0

.. erbsland-demo-end::

Use Indexed Access When a Position Already Matters
==================================================

Index-based access is sometimes the clearest solution, but each index kind has different costs and guarantees.
The most important rule is simple: byte indexes are native for UTF-8 storage; code-point indexes are semantic but may
require a scan.

Fast Side Access
----------------

Use :cpp:func:`charAt(StringSide) <erbsland::text::U8String::charAt>` when you need the first or last character.
This avoids spelling out the index and lets the string implementation choose the efficient path.

Use :cpp:func:`indexAt(StringSide) <erbsland::text::U8String::indexAt>` to get the first byte index or the byte index
after the last byte.
The back index is an end position, not the last character's start position.

Byte-Index Access
-----------------

Use :cpp:func:`charAt(ByteIndex) <erbsland::text::U8String::charAt>` when the byte index came from another ``String``
operation, from a saved byte position, or from
:cpp:func:`advance() <erbsland::text::U8String::advance>` /
:cpp:func:`retreat() <erbsland::text::U8String::retreat>`.

The operation is safe:

- If the byte index points to a valid character start, the decoded character is returned.
- If the byte index points inside a malformed or misplaced UTF-8 sequence, a replacement character is returned.
- If the byte index is exactly at the end, ``Char::endOfData()`` is returned.
- If the byte index is outside the visible range, ``Char::noCodePoint()`` is returned.

If you do no own length validation, always test signal characters before treating the result as regular text.

Code-Point Index Access
-----------------------

Use :cpp:func:`charAt(CpIndex) <erbsland::text::U8String::charAt>` when the position is already a code-point position
and the string is small enough that scanning is acceptable.
The same guidance applies to :cpp:func:`indexAt(CpIndex) <erbsland::text::U8String::indexAt>` and
:cpp:func:`toCharIndex(ByteIndex) <erbsland::text::U8String::toCharIndex>`.

Do not parse a UTF-8 string with a loop like "for each code-point index, call ``charAt(CpIndex)`` ".
That repeats a scan for every character.
Use :cpp:class:`StringCharReader <erbsland::text::StringCharReader>`,
:cpp:func:`forEach() <erbsland::text::U8String::forEach>`, or range-based iteration instead.

Slicing and Moving Indexes
--------------------------

Use :cpp:func:`slice(StringSide) <erbsland::text::U8String::slice>` when you need to peel one decoded character from the
front or back and continue with the remaining view.
This is useful for rules such as "the first character has different requirements than the rest".

Use :cpp:func:`advance() <erbsland::text::U8String::advance>` and
:cpp:func:`retreat() <erbsland::text::U8String::retreat>` to move byte indexes by decoded characters.
They keep index movement in the string API, where malformed encoding and boundary conditions can be handled
consistently.

.. erbsland-demo::
    :source: text/String/CharacterAccess.cpp
    :exec: text/string --demo CharacterAccess
    :source-sha256: 2016111cbdbfe7d8a5594ba296cc4f2467f3573a79b86ba8c99861f96bc4cf7a

.. code-block:: cpp

    /// `String` gives fast access to UTF-8 byte positions and explicit tools for
    /// moving between decoded code points.
    ///
    /// Use `charAt(StringSide)` for the first or last character, use byte indexes
    /// when a previous string operation already returned one, and use
    /// `advance()`/`retreat()` to keep byte indexes on character boundaries.
    /// Code-point indexes are convenient but require scanning UTF-8 text; reserve
    /// them for small strings or specialized code.
    void characterAccess() {
        const auto text = el::String{"ödev📚:matematik"_el};

        el::io::printLine("Text: "_el, text);
        el::io::printLine("First character: "_el, describeCharacter(text.charAt(el::StringSide::Front)));
        el::io::printLine("Last character: "_el, describeCharacter(text.charAt(el::StringSide::Back)));

        // Byte indexes are fast, but the index must point to the start of a UTF-8 sequence.
        auto byteIndex = el::ByteIndex::zero();
        el::io::printLine("Byte index 0: "_el, describeCharacter(text.charAt(byteIndex)));
        el::io::printLine("Byte index 1: "_el, describeCharacter(text.charAt(el::ByteIndex{1U})));
        el::io::printLine("End byte index: "_el, describeCharacter(text.charAt(text.indexAt(el::StringSide::Back))));
        el::io::printLine(
            "Outside byte range: "_el,
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

        // Slicing one character from a side is efficient and keeps the rest as an owning read-only value.
        const auto [firstCharacter, withoutFirst] = text.slice(el::StringSide::Front);
        const auto [lastCharacter, withoutLast] = text.slice(el::StringSide::Back);
        el::io::printLine("Sliced front: "_el, describeCharacter(firstCharacter), " | rest: "_el, withoutFirst);
        el::io::printLine("Sliced back: "_el, describeCharacter(lastCharacter), " | rest: "_el, withoutLast);

        // Code-point lengths can be used directly with regular string slicing.
        el::io::printLine("Code-point prefix: "_el, text.slice(el::StringSide::Front, el::CpLength{4U}));
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
    Code-point prefix: ödev

.. erbsland-demo-end::

Why UTF-32 Changes the Cost of Indexed Access
=============================================

UTF-32 makes code-point-index access cheap because one storage unit is one code point.
That does not automatically make conversion to :cpp:class:`U32StringEditor <erbsland::text::U32StringEditor>`
worthwhile.
Convert only when the data will be accessed by code-point index often enough to justify the extra memory, conversion
cost, and API width change.

Choose the Simplest Access Pattern That Fits
============================================

For ordinary read-only input, a :cpp:type:`String <erbsland::text::String>` parameter keeps the function pleasant to
call from values, editors, and literals.
Inside a parser, ``StringCharReader`` preserves that width independence and gives diagnostics a natural code-point
position.
Inside a simple inspection, a range-based loop keeps the intent visible without introducing reader state.
When an algorithm must pause at a native position or walk backward, ``readCharAndAdvance()`` and
``readCharAndRetreat()`` provide a small sequential cursor without the features of a parser.

Native byte indexes remain valuable when a search or slice already uses them.
Move such an index with ``advance()`` and ``retreat()`` so that it stays on a decoded character boundary; incrementing a
UTF-8 byte index by hand cannot provide that guarantee.
Likewise, repeated ``charAt(CpIndex)`` calls turn a sequential UTF-8 scan into repeated work from the beginning.

UTF-32 is a useful representation when direct code-point indexing is central to the data structure.
It is rarely worth converting an otherwise UTF-8 workflow merely to make one or two indexed accesses cheaper.

Boolean and numeric conversion do not require any character-reading strategy at all when the complete string is one
value; use the dedicated operations in :doc:`/topics/text_parsing_and_encoding/converting_text_and_scalar_values`
instead.
