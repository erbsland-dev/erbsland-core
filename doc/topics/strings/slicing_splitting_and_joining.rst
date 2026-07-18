Copyright (c) 2026 Tobias Erbsland - Erbsland DEV.
https://erbsland.dev SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Slicing, Splitting, and Joining Strings
    single: StringEditor Slicing
    single: StringEditor Splitting
    single: StringEditor Joining
    single: slice
    single: kept
    single: fromSplit
    single: join
    single: ByteRange
    single: CpRange
    single: StringSide
    single: StringEditorList
    single: StringList

***************************************
Slicing, Splitting, and Joining Strings
***************************************

StringEditor processing often starts with a simple task: take a useful part from a larger text.
You may want to extract a field, split a document into lines, keep a small fragment from a large buffer, or combine many
strings into one result.

Erbsland Core provides dedicated APIs for these common tasks:

*   :cpp:func:`slice() <erbsland::text::U8String::slice>` selects a range without unnecessary work.
*   :cpp:func:`kept() <erbsland::text::U8String::kept>` copies the selected range into independent storage.
*   ``fromSplit()`` splits text into a list of parts.
*   ``join()`` combines list entries into a single string.

Most operations work with the native storage positions of a string.
For UTF-8 text, these are byte positions.
When search operations already return byte indexes, you can usually reuse them directly without converting them to
code-point positions.
Code-point-based operations are also available when a problem is naturally expressed in decoded characters.

This page explains how to choose the right operation, how slicing and ownership interact, and how to process text
efficiently without losing readability.

Choosing the Right Operation
============================

Choose the operation that matches the information you already have and the storage result you need.

.. list-table::
    :header-rows: 1
    :widths: 30 35 35

    *   - Task
        - Preferred API
        - Notes
    *   - Slice between positions returned by search functions.
        - :cpp:func:`slice(ByteRange) <erbsland::text::U8String::slice>`
        - Fast and copy-free for read-only strings.
    *   - Slice a small fixed-format label by decoded character positions.
        - :cpp:func:`slice(CpRange) <erbsland::text::U8String::slice>`
        - Readable for short text, but UTF-8 and UTF-16 must be scanned.
    *   - Take a prefix or suffix.
        - :cpp:func:`slice(StringSide, ByteLength) <erbsland::text::U8String::slice>` or
          :cpp:func:`slice(StringSide, CpLength) <erbsland::text::U8String::slice>`
        - The suffix code-point form scans backward from the end.
    *   - Keep a small part from a large editable string.
        - :cpp:func:`kept() <erbsland::text::U8String::kept>`
        - Copies the selected range so the large source can be released.
    *   - Split text into fields or lines.
        - ``StringList::fromSplit()``
        - Produces views into the original text.
    *   - Combine list entries into one text.
        - ``StringEditorList::join()``
        - Calculates the final size first, then copies once.

The examples on this page use :cpp:type:`String <erbsland::text::String>`,
:cpp:type:`StringEditor <erbsland::text::StringEditor>`, :cpp:type:`StringList <erbsland::text::StringList>`, and
:cpp:type:`StringEditorList <erbsland::text::StringEditorList>`.
The same usage patterns are available for the UTF-8, UTF-16, and UTF-32 width-specific types.

Slicing Parts Manually
======================

Byte Ranges
-----------

When you search inside a UTF-8 string, functions such as
:cpp:func:`find() <erbsland::text::U8String::find>`,
:cpp:func:`findFirstOf() <erbsland::text::U8String::findFirstOf>`, and
:cpp:func:`findLastOf() <erbsland::text::U8String::findLastOf>` return byte indexes.
You can pass these indexes directly to :cpp:class:`ByteRange <erbsland::unit::IntegerUnitRange>` and
:cpp:func:`slice(ByteRange) <erbsland::text::U8String::slice>`.

For :cpp:type:`String <erbsland::text::String>`, byte-range slicing creates another view into the same backing
text.
No text bytes are copied.
This makes byte ranges a good fit for parsing records, protocol fields, identifiers, and other text where separators
were found by the string API.

.. erbsland-demo::
    :source: text/String/ByteRangeSlicing.cpp
    :exec: text/string --demo ByteRangeSlicing
    :source-sha256: cb4d8150296a80b753ef17448b611cbca6b77f8e8ac8dd6cb2892f011ed4b499

.. code-block:: cpp

    /// Byte-range slicing is the fast path for cutting `String` data into
    /// smaller views.
    ///
    /// Search operations such as `find()` return byte indexes. You can use these
    /// indexes directly to build `ByteRange` values and pass them to `slice()`.
    /// The resulting strings refer to the same backing text and do not copy
    /// the selected bytes.
    void byteRangeSlicing() {
        const auto journal = el::String{"dag=12|plats=Norrpasset|väder=klar|signal=stjärna"_el};

        // Find separator positions once, then slice the fields between them.
        const auto firstSeparator = journal.find("|"_el);
        auto placeStart = firstSeparator;
        journal.advance(placeStart);
        const auto secondSeparator = journal.find("|"_el, placeStart);
        auto weatherStart = secondSeparator;
        journal.advance(weatherStart);
        const auto thirdSeparator = journal.find("|"_el, weatherStart);

        const auto day = journal.slice(el::ByteRange{el::ByteIndex::zero(), firstSeparator});
        const auto place = journal.slice(el::ByteRange{placeStart, secondSeparator});
        const auto weather = journal.slice(el::ByteRange{weatherStart, thirdSeparator});

        el::io::printLine("Journal: "_el, journal);
        el::io::printLine("First separator at byte: "_el, firstSeparator);
        el::io::printLine("Day: "_el, day);
        el::io::printLine("Place: "_el, place);
        el::io::printLine("Weather: "_el, weather);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Journal: dag=12|plats=Norrpasset|väder=klar|signal=stjärna
    First separator at byte: 6
    Day: dag=12
    Place: plats=Norrpasset
    Weather: väder=klar

.. erbsland-demo-end::

Keep byte indexes on character boundaries.
The safest indexes are the ones returned by string operations, or indexes moved with
:cpp:func:`advance() <erbsland::text::U8String::advance>` and
:cpp:func:`retreat() <erbsland::text::U8String::retreat>`.

Boundaries and Invalid UTF
--------------------------

Slicing is bounds-safe.
If a range starts outside the string, the result is empty.
If a finite range is longer than the remaining text, it is clamped to the available range.
The special :cpp:func:`ByteRange::noRange() <erbsland::unit::IntegerUnitRange::noRange>` also produces an empty result.

Byte slicing does not validate whether the start and end positions are UTF-8 character boundaries.
If you cut through a multi-byte sequence, the resulting view is still safe to store and pass around, but it contains
invalid UTF-8. When decoded, malformed sequences are represented with the Unicode replacement character.

.. erbsland-demo::
    :source: text/String/SliceBoundaries.cpp
    :exec: text/string --demo SliceBoundaries
    :source-sha256: 6a3f96ae3f612254a4dd6fca7931e1b51601a3c6e10721d62055f60a19143829

.. code-block:: cpp

    /// Byte slicing is bounds-safe, but it does not validate UTF-8 boundaries for
    /// you.
    ///
    /// A range outside the string becomes an empty view, and an overly long range
    /// is clamped to the available text. If a range starts or ends inside a UTF-8
    /// sequence, the resulting view contains invalid UTF-8 and later decoding
    /// yields replacement characters. Keep byte indexes on character boundaries by
    /// using indexes returned by the string API, or by moving them with
    /// `advance()` and `retreat()`.
    void sliceBoundaries() {
        const auto text = el::String{"AåB"_el};

        // Out-of-range slices are safe and simply produce an empty view.
        const auto outside = text.slice(el::ByteRange{el::ByteIndex{99U}, el::ByteLength{5U}});

        // Overly long ranges are clamped to the available storage range.
        const auto clamped = text.slice(el::ByteRange{el::ByteIndex{1U}, el::ByteLength{99U}});

        // Bad code: byte index 2 is in the middle of the UTF-8 sequence for "å".
        const auto broken = text.slice(el::ByteRange{el::ByteIndex{2U}, el::ByteLength{1U}});

        el::io::printLine("Text: "_el, text);
        el::io::printLine("Outside slice is empty: "_el, outside.isEmpty());
        el::io::printLine("Clamped slice: "_el, clamped);
        el::io::printLine("Broken slice: "_el, broken);
        el::io::printLine("Broken slice is valid UTF-8: "_el, broken.isValidUtf8());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Text: AåB
    Outside slice is empty: true
    Clamped slice: åB
    Broken slice: �
    Broken slice is valid UTF-8: false

.. erbsland-demo-end::

Code-Point Ranges
-----------------

:cpp:func:`slice(CpRange) <erbsland::text::U8String::slice>` selects text by decoded code-point positions.
This is useful when the text is short and the positions are naturally counted in user-visible characters, for example in
small labels, fixed-format tokens, and tests.

For UTF-8 and UTF-16 strings, a code-point index is not a native storage position.
The string has to be scanned to find the matching byte or code-unit position.
Avoid code-point range slicing inside loops over large text.
When you already have byte indexes from :cpp:func:`find() <erbsland::text::U8String::find>` or related operations,
use byte ranges instead.

.. erbsland-demo::
    :source: text/String/CodePointRangeSlicing.cpp
    :exec: text/string --demo CodePointRangeSlicing
    :source-sha256: 97ff97116df3ab62470737d882c8b75f39a3350b7d6204d4dd142fbf1ff6756b

.. code-block:: cpp

    /// Code-point range slicing is useful for short, fixed-shape text where the
    /// positions are naturally counted in decoded characters.
    ///
    /// For UTF-8 and UTF-16 strings, a code-point range requires scanning from the
    /// beginning of the text to find the matching storage positions. Use it for
    /// small identifiers and labels, not as an inner-loop strategy for large
    /// documents.
    void codePointRangeSlicing() {
        const auto label = el::String{"Färd-Karta-07"_el};

        // The label is short and fixed-shape, so code-point positions are readable.
        const auto mapName = label.slice(el::CpRange{el::CpIndex{5U}, el::CpLength{5U}});
        const auto number = label.slice(el::CpRange{el::CpIndex{11U}, el::CpLength{2U}});

        el::io::printLine("Label: "_el, label);
        el::io::printLine("Map name: "_el, mapName);
        el::io::printLine("Map number: "_el, number);
        el::io::printLine("Map name byte length: "_el, mapName.length());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Label: Färd-Karta-07
    Map name: Karta
    Map number: 07
    Map name byte length: 5

.. erbsland-demo-end::

Slicing Front and Back Parts
============================

Use the side-based overloads when you need a prefix or suffix and do not want to spell out a full range.

*   :cpp:func:`slice(StringSide::Front, ByteLength) <erbsland::text::U8String::slice>` takes an initial byte range.
*   :cpp:func:`slice(StringSide::Back, ByteLength) <erbsland::text::U8String::slice>` takes a trailing byte range.
*   :cpp:func:`slice(StringSide::Front, CpLength) <erbsland::text::U8String::slice>` takes an initial decoded
    code-point range.
*   :cpp:func:`slice(StringSide::Back, CpLength) <erbsland::text::U8String::slice>` takes a trailing decoded
    code-point range.

The code-point suffix form is especially useful for large strings because it scans backward from the end.
This avoids scanning from the beginning just to find a distant suffix start.
After you have the suffix, you can subtract its native length from the original length to get the remaining prefix.

.. erbsland-demo::
    :source: text/String/FrontBackSlicing.cpp
    :exec: text/string --demo FrontBackSlicing
    :source-sha256: 768f43f9084249d6f7435c882896e635b87623b0f0a3a5ea592db5d642a5c7db

.. code-block:: cpp

    /// Side-based slicing takes a prefix or suffix without spelling out a full
    /// range.
    ///
    /// Use `slice(StringSide::Front, ByteLength)` when a previous byte-index search
    /// already told you how long the prefix is. Use
    /// `slice(StringSide::Back, CpLength)` when the suffix is naturally counted in
    /// decoded characters. The byte length of that suffix can then be used to take
    /// the remaining prefix efficiently.
    void frontBackSlicing() {
        const auto route = el::String{"Rutt: Åsleden -> Nordljus"_el};

        // A byte index from `find()` can become the prefix byte length.
        const auto separator = route.find(" -> "_el);
        const auto origin = route.slice(el::StringSide::Front, separator.distanceFromZero());

        // A destination name is user-visible text, so take it as code points.
        const auto destination = route.slice(el::StringSide::Back, el::CpLength{8U});
        const auto withoutDestination = route.slice(el::StringSide::Front, route.length() - destination.length());

        el::io::printLine("Route: "_el, route);
        el::io::printLine("Origin: "_el, origin);
        el::io::printLine("Destination: "_el, destination);
        el::io::printLine("Without destination: ["_el, withoutDestination, "]"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Route: Rutt: Åsleden -> Nordljus
    Origin: Rutt: Åsleden
    Destination: Nordljus
    Without destination: [Rutt: Åsleden -> ]

.. erbsland-demo-end::

Slice or Kept
=============

:cpp:func:`slice() <erbsland::text::U8String::slice>` and
:cpp:func:`kept() <erbsland::text::U8String::kept>` select the same text when called with the same range.
The difference is ownership.

For :cpp:type:`String <erbsland::text::String>`,
:cpp:func:`slice() <erbsland::text::U8String::slice>` returns another view.
It is copy-free and keeps the backing text alive.
:cpp:func:`kept() <erbsland::text::U8String::kept>` returns an editable
:cpp:type:`StringEditor <erbsland::text::StringEditor>` that contains a copy of the selected range.

For :cpp:type:`StringEditor <erbsland::text::StringEditor>`, :cpp:func:`slice() <erbsland::text::U8String::slice>` narrows the
string to the selected range and can keep sharing the same backing store.
:cpp:func:`kept() <erbsland::text::U8String::kept>` materializes the selected range into independent storage.

This distinction matters when you load a large text and only need a small part of it.
If you keep a slice, the large backing store may remain alive.
If you keep a copy, the large source can be released after the call.

.. erbsland-demo::
    :source: text/String/SliceAndKept.cpp
    :exec: text/string --demo SliceAndKept
    :source-sha256: 8e9fdb406bc1e7a823c7982bec198ef3da9cddab1845c1ceea5a98c59518fd63

.. code-block:: cpp

    /// `slice()` and `kept()` select the same text but make different storage
    /// choices.
    ///
    /// `slice()` returns a narrowed string that can continue to share the original
    /// backing store. This is ideal for temporary parsing and for lists of views.
    /// `kept()` materializes the selected range as an editable string, which lets a
    /// large source string be released after the interesting part has been copied.
    void sliceAndKept() {
        auto journal = el::StringEditor{"rubrik=Norrpasset|väder=klar|anteckning=Stjärnklart över sjön"_el};
        auto noteStart = journal.findLastOf(el::CharSet{U'|'});
        journal.advance(noteStart);

        // `slice()` keeps the range as a narrow view into compatible string storage.
        const auto noteSlice = journal.slice(el::ByteRange{noteStart, el::ByteLength::infinite()});

        // `kept()` copies just the selected range into an independent string.
        const auto noteCopy = journal.kept(el::ByteRange{noteStart, el::ByteLength::infinite()});

        el::io::printLine("Slice result: "_el, noteSlice);
        el::io::printLine("Kept result:  "_el, noteCopy);
        el::io::printLine("Slice byte length: "_el, noteSlice.length());
        el::io::printLine("Kept byte length:  "_el, noteCopy.length());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Slice result: anteckning=Stjärnklart över sjön
    Kept result:  anteckning=Stjärnklart över sjön
    Slice byte length: 35
    Kept byte length:  35

.. erbsland-demo-end::

Splitting Strings
=================

Use ``StringList::fromSplit()`` to split text into parts.
For the common UTF-8 aliases, :cpp:type:`StringList <erbsland::text::StringList>` stores views, while
:cpp:type:`StringEditorList <erbsland::text::StringEditorList>` stores editable strings.

The separator argument is a :cpp:class:`CharSet <erbsland::text::CharSet>`.
Every character in the set is a split point.
Empty parts are dropped by default, which is convenient for word splitting and whitespace cleanup.
Pass ``keepEmpty = true`` when empty fields are meaningful, for example in table data.

The split limit is the maximum number of split points to apply.
``ElementCount::infinite()`` is the default and uses all split points.
``ElementCount::zero()`` applies no split points and returns one unsplit
element.
A finite limit of ``n`` produces at most ``n + 1`` parts.

.. erbsland-demo::
    :source: text/String/SplittingText.cpp
    :exec: text/string --demo SplittingText
    :source-sha256: 94da9770e88894f9a73902f11bfae21ba40ae35df8d4d2d33cdeadd965beb7f3

.. code-block:: cpp

    /// `StringList::fromSplit()` splits text into a list of read-only strings.
    ///
    /// The split parts refer to the original text and are therefore cheap to
    /// create. Empty parts are dropped by default; set `keepEmpty` to true when an
    /// empty field is meaningful. The split limit is the maximum number of split
    /// points to apply, so a limit of two produces at most three parts.
    void splittingText() {
        const auto table = el::String{
            "dag;plats;väder\n"
            "12;Åsleden;klar\n"
            "13;;dimma\n"
            "14;Nordljus;stjärnklart"_el};
        const auto rows = el::StringList::fromSplit(table, el::CharSet{"\n"_el});

        el::io::printLine("Rows: "_el, rows.count());
        for (const auto &row : rows) {
            const auto fields = el::StringList::fromSplit(
                row, el::CharSet{";"_el}, el::ElementCount::infinite(), true);
            el::io::printLine("  "_el, fields.join(" | "_el));
        }

        // A split limit leaves the unsplit remainder in the last part.
        const auto limited = el::StringList::fromSplit(
            rows.last(), el::CharSet{";"_el}, el::ElementCount{1U}, true);
        el::io::printLine("Limited split: "_el, limited.join(" / "_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Rows: 4
      dag | plats | väder
      12 | Åsleden | klar
      13 |  | dimma
      14 | Nordljus | stjärnklart

.. erbsland-demo-end::

Joining Strings
===============

Use ``join()`` to combine a list of strings.
The separator is optional.
An empty separator joins entries directly.

``join()`` is the natural counterpart to
``fromSplit()``.
It calculates the final size, reserves the required storage, and then copies the pieces into the result.
This is both clearer and more efficient than appending repeatedly in application code.

This pattern works especially well for line-based documents.
You can split a document into views, transform or sort the list, and join it back into a single text.

.. erbsland-demo::
    :source: text/String/JoiningText.cpp
    :exec: text/string --demo JoiningText
    :source-sha256: 53eebd6e083f404e6cf693f763b365d1ad769129468654da079aab1a99b78b07

.. code-block:: cpp

    /// `join()` combines all entries from a string list with an optional separator.
    ///
    /// This is more efficient and clearer than appending in a manual loop. The
    /// final size is calculated first, then the result storage is reserved once and
    /// filled from the list entries.
    void joiningText() {
        const auto notes = el::StringEditorList{
            el::StringEditor{"14 Nordljus: stjärnklart"_el},
            el::StringEditor{"12 Åsleden: klar sikt"_el},
            el::StringEditor{"13 Norrpasset: dimma"_el},
        };

        // Lists can be transformed first, then joined into the final document.
        const auto sortedNotes = notes.sorted(el::Char::compareCaseFolded);
        const auto document = sortedNotes.join("\n"_el);

        el::io::printLine("Sorted journal page:"_el);
        el::io::printLine(document);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Sorted journal page:
    12 Åsleden: klar sikt
    13 Norrpasset: dimma
    14 Nordljus: stjärnklart

.. erbsland-demo-end::
