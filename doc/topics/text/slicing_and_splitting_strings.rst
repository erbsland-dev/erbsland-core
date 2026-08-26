..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Slicing Strings
    !single: Splitting Strings
    single: ByteRange
    single: CpRange
    single: StringList

*****************************
Slicing and Splitting Strings
*****************************

A parser often needs only one field from a record, while a command line or delimited row needs to be divided into many
parts.
In both cases, copying every selected character would be needless work when the source text can safely remain alive.

Slices and split results are ordinary read-only :cpp:type:`String <erbsland::text::String>` values that can share the
source backing store.
This page shows how to describe their boundaries correctly, when byte coordinates are the efficient choice, and what it
means for the lifetime of a small result that still refers to a much larger source.

Describe a Boundary in the Right Coordinate System
==================================================

Use byte coordinates for UTF-8 positions returned by search, parsing, or iteration APIs.
They address the native storage directly and avoid rescanning text to translate a code-point position.

Use code-point coordinates when the position is naturally counted in decoded characters, such as a short fixed-format
identifier.
For UTF-8 and UTF-16, converting a code-point index can require scanning from a known boundary.

.. erbsland-demo::
    :source: text/String/ByteRangeSlicing.cpp
    :exec: text/string --demo ByteRangeSlicing
    :source-sha256: 3b261de38fe8da028c25bda08d4b4b688314de3607cade86fc2fa1c4fb7767db

.. code-block:: cpp

    /// Byte-range slicing is the fast path for cutting `String` data into
    /// smaller views.
    ///
    /// Search operations such as `find()` return byte indexes. You can use these
    /// indexes directly to build `ByteRange` values and pass them to `slice()`.
    /// The resulting read-only strings refer to the same backing text and do not copy
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

.. erbsland-demo::
    :source: text/String/CodePointRangeSlicing.cpp
    :exec: text/string --demo CodePointRangeSlicing
    :source-sha256: 7007230c12e1371594584f0cea5a1d1c9ffbccac6b15fcdda1a01d31264821ac

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

Keep the Beginning, End, or Middle
==================================

Use ``slice()``, ``first()``, and ``last()`` when the result should remain a shared read-only value.
The slice retains ownership of the backing store, so it remains valid independently of the source object.

.. erbsland-demo::
    :source: text/String/FrontBackSlicing.cpp
    :exec: text/string --demo FrontBackSlicing
    :source-sha256: 6fa6339565a2f53e43d9dc5b9ba7e0fc67a0b4f9f4ce629190e2a9a52ef94b18

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

Turn One Value into Shared Fields
=================================

``split()`` produces a ``StringList`` whose entries normally reference ranges in the source.
This makes tokenization inexpensive, but a retained token can keep the complete source allocation alive.
Create a compact independent value only when that retention matters at a longer-lived storage boundary.

Control the maximum split count when only a fixed number of fields is required.
Choose deliberately whether empty fields are preserved, because adjacent separators and separators at the boundaries can
otherwise disappear from the logical record.

.. erbsland-demo::
    :source: text/String/SplittingText.cpp
    :exec: text/string --demo SplittingText
    :source-sha256: 5ab83c6021b051c04328fd2ce99b1dd404a8a52aa0f1e8346a667ad679fcb9d9

.. code-block:: cpp

    /// `StringList::fromSplit()` splits text into a list of read-only strings.
    ///
    /// The split parts refer to the original text and are therefore cheap to
    /// create. Empty parts are dropped by default; set `keepEmpty` to true when an
    /// empty field is meaningful. The split limit is the maximum number of split
    /// points to apply, so a limit of two produces at most three parts.
    void splittingText() {
        const auto table = el::String{"dag;plats;väder\n"
                                      "12;Åsleden;klar\n"
                                      "13;;dimma\n"
                                      "14;Nordljus;stjärnklart"_el};
        const auto rows = el::StringList::fromSplit(table, el::CharSet{"\n"_el});

        el::io::printLine("Rows: "_el, rows.count());
        for (const auto &row : rows) {
            const auto fields = el::StringList::fromSplit(row, el::CharSet{";"_el}, el::ItemCount::infinite(), true);
            el::io::printLine("  "_el, fields.join(" | "_el));
        }

        // A split limit leaves the unsplit remainder in the last part.
        const auto limited = el::StringList::fromSplit(rows.last(), el::CharSet{";"_el}, el::ItemCount{1U}, true);
        el::io::printLine("Limited split: "_el, limited.join(" / "_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Rows: 4
      dag | plats | väder
      12 | Åsleden | klar
      13 |  | dimma
      14 | Nordljus | stjärnklart
    Limited split: 14 / Nordljus;stjärnklart

.. erbsland-demo-end::

Joining belongs to construction rather than slicing.
See :doc:`building_strings` for the allocation differences between ``String::fromJoined()``, ``StringList::join()``, a
reserved editor, and ``AnyStringBuilder``.
