..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Finding Text Positions
    single: StringEditor Search
    single: Text Search
    single: Character Search
    single: find
    single: findFirstOf
    single: findFirstNotOf
    single: findLastOf
    single: findLastNotOf
    single: ByteIndex

**********************
Finding Text Positions
**********************

Finding text positions is the starting point for many parsing, slicing, validation, and reporting tasks.
Erbsland Core keeps these operations efficient by returning native storage indexes from the search functions.

For :cpp:type:`String <erbsland::text::String>`, these positions are byte indexes.
You can pass them directly to :cpp:func:`slice() <erbsland::text::U8String::slice>`, continue searching from them,
or move them with :cpp:func:`advance() <erbsland::text::U8String::advance>` and
:cpp:func:`retreat() <erbsland::text::U8String::retreat>` when a result must be shifted by decoded characters.

This page explains which search operation to choose, how to handle missing matches, and how to combine searches into
larger text-processing patterns.

Choose a Search Function
========================

Use a character-set search when the boundary is any one of several characters.
Use text search when the boundary or marker is a complete string.

.. list-table::
    :header-rows: 1
    :widths: 32 34 34

    *   - Task
        - Preferred API
        - Typical follow-up
    *   - Find the next delimiter, marker character, or line break.
        - :cpp:func:`findFirstOf() <erbsland::text::U8String::findFirstOf>`
        - Slice before the delimiter or advance past it.
    *   - Skip padding, separators, or other ignored characters.
        - :cpp:func:`findFirstNotOf() <erbsland::text::U8String::findFirstNotOf>`
        - Start the next token or field at the returned position.
    *   - Find the last delimiter before an end position.
        - :cpp:func:`findLastOf() <erbsland::text::U8String::findLastOf>`
        - Split a path-like or record-like string from the right.
    *   - Ignore trailing padding or separators.
        - :cpp:func:`findLastNotOf() <erbsland::text::U8String::findLastNotOf>`
        - Trim a search range before slicing.
    *   - Find a complete string.
        - :cpp:func:`find() <erbsland::text::U8String::find>`
        - Continue with a start position, or pass a comparison function.

All search functions return the matching position, or the no-index state of the matching index type when there is no
match.
Check this state with :cpp:func:`isNoIndex() <erbsland::unit::IntegerUnitIndex::isNoIndex>` before slicing or reading
the character at the result.

Search Positions Are Storage Positions
======================================

Search functions on UTF-8 strings return :cpp:type:`ByteIndex <erbsland::unit::ByteIndex>`.
This is deliberate.
The returned position can be reused without rescanning the text, and it is already on a decoded character boundary.

When you need the character at a found position, call
:cpp:func:`charAt(ByteIndex) <erbsland::text::U8String::charAt>`.
When you need the text before, after, or between positions, construct a
:cpp:type:`ByteRange <erbsland::unit::ByteRange>` and pass it to
:cpp:func:`slice(ByteRange) <erbsland::text::U8String::slice>`.

Do not add raw byte numbers to move through UTF-8 text unless the value is a byte length that came from the string API
itself.
Use :cpp:func:`advance() <erbsland::text::U8String::advance>` and
:cpp:func:`retreat() <erbsland::text::U8String::retreat>` to move by decoded characters.

Find Characters from Sets
=========================

:cpp:class:`CharSet <erbsland::text::CharSet>` lets one search operation cover a complete group of characters.
For example, a delimiter set can contain several punctuation marks, and a padding set can contain all ASCII whitespace.

The forward functions start at the given position and include that position in the search.
The reverse functions search before the given end position.
This makes it easy to ask for the last delimiter before a known marker.

.. erbsland-demo::
    :source: text/String/FindingCharacterSets.cpp
    :exec: text/string --demo FindingCharacterSets
    :source-sha256: f2fc26c14791713aff9b48332a07da7742d2bfbd5b5a3d66b35487a74fc2ca39

.. code-block:: cpp

    /// Character-set search functions find positions of decoded characters.
    ///
    /// Use findFirstOf and findLastOf to locate delimiters from a set. Use
    /// findFirstNotOf and findLastNotOf to skip padding or other characters that
    /// are not part of the useful text. All positions returned by String are
    /// byte indexes that can be reused for slicing or further searches.
    void findingCharacterSets() {
        static const auto padding = el::CharSet::from(el::AsciiCategory::Whitespace);
        static const auto separators = el::CharSet{U':', U'=', U';'};

        const auto logLine = el::String{"  Dyklogg: mål=Kosterhavet; djup=240m; status=redo  "_el};

        const auto contentStart = logLine.findFirstNotOf(padding);
        const auto contentEnd = logLine.findLastNotOf(padding);
        const auto firstSeparator = logLine.findFirstOf(separators);
        const auto lastSeparator = logLine.findLastOf(separators);
        const auto statusStart = logLine.find("status"_el);
        const auto separatorBeforeStatus = logLine.findLastOf(separators, statusStart);

        el::io::printLine("Log line: '"_el, logLine, "'"_el);
        el::io::printLine("First content byte ........: "_el, contentStart);
        el::io::printLine("Last content byte .........: "_el, contentEnd);
        el::io::printLine(
            "First separator ...........: "_el, firstSeparator, " ('"_el, logLine.charAt(firstSeparator), "')"_el);
        el::io::printLine(
            "Last separator ............: "_el, lastSeparator, " ('"_el, logLine.charAt(lastSeparator), "')"_el);
        el::io::printLine("Separator before status ...: "_el,
            separatorBeforeStatus,
            " ('"_el,
            logLine.charAt(separatorBeforeStatus),
            "')"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Log line: '  Dyklogg: mål=Kosterhavet; djup=240m; status=redo  '
    First content byte ........: 2
    Last content byte .........: 50
    First separator ...........: 9 (':')
    Last separator ............: 46 ('=')
    Separator before status ...: 38 (';')

.. erbsland-demo-end::

Extract Runs of Matching Characters
===================================

A common parser pattern is to define the characters that belong to a token and treat everything else as separation.
Use :cpp:func:`findFirstOf() <erbsland::text::U8String::findFirstOf>` to find the next token start, then
:cpp:func:`findFirstNotOf() <erbsland::text::U8String::findFirstNotOf>` to find its end.

The end position may be missing when the final token reaches the end of the string.
In that case, use :cpp:func:`indexAt(StringSide::Back) <erbsland::text::U8String::indexAt>` as the end position.

.. erbsland-demo::
    :source: text/String/TokenRuns.cpp
    :exec: text/string --demo TokenRuns
    :source-sha256: 5c45fcac1e95f26bd98d4a32a09cee326179ecb40051c7ade4582c078d06c582

.. code-block:: cpp

    /// Combining findFirstOf and findFirstNotOf extracts runs of matching characters.
    ///
    /// Define the characters that belong to a token, then search for the next
    /// matching character and the next non-matching character. Everything outside
    /// the token set acts as a separator, so punctuation, spaces, and symbols do not
    /// need separate handling.
    void tokenRuns() {
        static const auto tokenCharacters =
            el::CharSet::from(el::UnicodeCategoryGroup::Letter) |
            el::CharSet::from(el::UnicodeCategory::DecimalNumber) |
            el::CharSet{U'-'};

        const auto route = el::String{"Rutt: Havsörn-7 går mot djupzon Ålvik; prov=A12; temp=4°C"_el};

        el::io::printLine("Route note: "_el, route);
        el::io::printLine("Tokens:"_el);

        auto tokenStart = route.findFirstOf(tokenCharacters);
        while (!tokenStart.isNoIndex()) {
            auto tokenEnd = route.findFirstNotOf(tokenCharacters, tokenStart);
            if (tokenEnd.isNoIndex()) {
                tokenEnd = route.indexAt(el::StringSide::Back);
            }

            const auto token = route.slice(el::ByteRange{tokenStart, tokenEnd});
            el::io::printLine("  ["_el, tokenStart, ", "_el, tokenEnd, "): "_el, token);

            tokenStart = route.findFirstOf(tokenCharacters, tokenEnd);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Route note: Rutt: Havsörn-7 går mot djupzon Ålvik; prov=A12; temp=4°C
    Tokens:
      [0, 4): Rutt
      [6, 16): Havsörn-7
      [17, 21): går
      [22, 25): mot
      [26, 33): djupzon
      [34, 40): Ålvik
      [42, 46): prov
      [47, 50): A12
      [52, 56): temp
      [57, 58): 4
      [60, 61): C

.. erbsland-demo-end::

Find Text Repeatedly
====================

:cpp:func:`find() <erbsland::text::U8String::find>` searches for a complete string.
Use the overload with a start position to continue after a previous result.

When a loop must find overlapping occurrences, move the start position by one decoded character with
:cpp:func:`advance() <erbsland::text::U8String::advance>`.
When only non-overlapping occurrences are useful, move the start position by the native length of the text being
searched.

Guard repeated searches against an empty needle.
Searching for empty text succeeds at the start position, so a loop must either reject it or handle it as a special case.

Pass a :cpp:type:`CharCompareFn <erbsland::text::CharCompareFn>` when matching should use a different character
comparison rule.
For example, :cpp:func:`Char::compareCaseFolded() <erbsland::text::Char::compareCaseFolded>` performs Unicode simple
case folding while the original text stays unchanged.

.. erbsland-demo::
    :source: text/String/FindingAllOccurrences.cpp
    :exec: text/string --demo FindingAllOccurrences
    :source-sha256: 3740f243bdf602bea71a5e11104a13b01d11c44b462c2cb2563bece8935ac68c

.. code-block:: cpp

    /// String::find can be called repeatedly to collect every text position.
    ///
    /// Pass a start position to continue the search after a previous match. Move
    /// the start position with advance when overlapping matches should remain
    /// possible, or add the needle length when only non-overlapping matches are
    /// useful. A character comparison function can adapt the matching rule without
    /// first transforming the source text.
    void findingAllOccurrences() {
        const auto missionLog = el::String{
            "ROV Freja såg ljus; rov freja markerade ljus; ROV Freja sparade karta"_el};
        const auto needle = el::String{"rov freja"_el};

        el::io::printLine("Mission log: "_el, missionLog);
        el::io::printLine("Needle: "_el, needle);
        el::io::printLine("Case-folded matches:"_el);

        auto searchStart = el::ByteIndex::zero();
        auto matchIndex = missionLog.find(needle, searchStart, el::Char::compareCaseFolded);
        while (!matchIndex.isNoIndex()) {
            const auto match = missionLog.slice(el::ByteRange{matchIndex, needle.length()});
            el::io::printLine("  byte "_el, matchIndex, ": "_el, match);

            searchStart = matchIndex;
            if (!missionLog.advance(searchStart)) {
                break;
            }
            matchIndex = missionLog.find(needle, searchStart, el::Char::compareCaseFolded);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Mission log: ROV Freja såg ljus; rov freja markerade ljus; ROV Freja sparade karta
    Needle: rov freja
    Case-folded matches:
      byte 0: ROV Freja
      byte 21: rov freja
      byte 47: ROV Freja

.. erbsland-demo-end::

Use Found Positions for Slicing
===============================

Found positions compose naturally with slicing.
The common pattern is:

*   Search for the start boundary.
*   Move past the boundary with :cpp:func:`advance() <erbsland::text::U8String::advance>` if the boundary itself
    should be excluded.
*   Search for the end boundary from the adjusted start position.
*   Slice the :cpp:type:`ByteRange <erbsland::unit::ByteRange>` between the two positions.

When a boundary is optional, handle the no-index state before constructing the range.
When the end boundary is optional and the slice should continue to the end of the text, use
:cpp:func:`indexAt(StringSide::Back) <erbsland::text::U8String::indexAt>` or an infinite
:cpp:type:`ByteLength <erbsland::unit::ByteLength>`.

This approach avoids conversions to character indexes and keeps parsing code close to the data the string stores.
