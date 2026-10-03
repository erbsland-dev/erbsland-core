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

Slicing with Byte and Code-Point Ranges
=======================================

Use byte coordinates for UTF-8 positions returned by search, parsing, or iteration APIs.
They address the native storage directly and avoid rescanning text to translate a code-point position.

Use code-point coordinates when the position is naturally counted in decoded characters, such as a short fixed-format
identifier.
For UTF-8 and UTF-16, converting a code-point index can require scanning from a known boundary.

.. erbsland-demo::
    :source: text/String/ByteRangeSlicing.cpp
    :exec: text/string --demo ByteRangeSlicing
    :sha256: 815977e2b0bc17dab052431c1ac7c806d8bd51f9af51a81eb0a8fcdf8171d857

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mJournal: dag=12|plats=Norrpasset|väder=klar|signal=stjärna
        First separator at byte: 6
        Day: dag=12
        Place: plats=Norrpasset
        Weather: väder=klar␛[0m

.. erbsland-demo-end::

.. erbsland-demo::
    :source: text/String/CodePointRangeSlicing.cpp
    :exec: text/string --demo CodePointRangeSlicing
    :sha256: c1a066613c7dde4b0ccbd6334b884c77d6a182d6ac6e09822d440bd974163904

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
        const auto mapName = label.slice(el::CpRange{el::CpIndex{5U}, el::CpCount{5U}});
        const auto number = label.slice(el::CpRange{el::CpIndex{11U}, el::CpCount{2U}});

        el::io::printLine("Label: "_el, label);
        el::io::printLine("Map name: "_el, mapName);
        el::io::printLine("Map number: "_el, number);
        el::io::printLine("Map name byte length: "_el, mapName.length());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mLabel: Färd-Karta-07
        Map name: Karta
        Map number: 07
        Map name byte length: 5␛[0m

.. erbsland-demo-end::

Prefix, Suffix, and Middle Slices
=================================

Use ``slice()``, ``first()``, and ``last()`` when the result should remain a shared read-only value.
The slice retains ownership of the backing store, so it remains valid independently of the source object.

.. erbsland-demo::
    :source: text/String/FrontBackSlicing.cpp
    :exec: text/string --demo FrontBackSlicing
    :sha256: 168e409d5c38329713943eee58e42ee18b99407bd888c090cf99c91108f05c9a

.. code-block:: cpp

    /// Side-based slicing takes a prefix or suffix without spelling out a full
    /// range.
    ///
    /// Use `slice(StringSide::Front, ByteCount)` when a previous byte-index search
    /// already told you how long the prefix is. Use
    /// `slice(StringSide::Back, CpCount)` when the suffix is naturally counted in
    /// decoded characters. The byte length of that suffix can then be used to take
    /// the remaining prefix efficiently.
    void frontBackSlicing() {
        const auto route = el::String{"Rutt: Åsleden -> Nordljus"_el};

        // A byte index from `find()` can become the prefix byte length.
        const auto separator = route.find(" -> "_el);
        const auto origin = route.slice(el::StringSide::Front, separator.distanceFromZero());

        // A destination name is user-visible text, so take it as code points.
        const auto destination = route.slice(el::StringSide::Back, el::CpCount{8U});
        const auto withoutDestination = route.slice(el::StringSide::Front, route.length() - destination.length());

        el::io::printLine("Route: "_el, route);
        el::io::printLine("Origin: "_el, origin);
        el::io::printLine("Destination: "_el, destination);
        el::io::printLine("Without destination: ["_el, withoutDestination, "]"_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRoute: Rutt: Åsleden -> Nordljus
        Origin: Rutt: Åsleden
        Destination: Nordljus
        Without destination: [Rutt: Åsleden -> ]␛[0m

.. erbsland-demo-end::

Splitting Strings into Shared Slices
====================================

``split()`` produces a ``StringList`` whose entries normally reference ranges in the source.
This makes tokenization inexpensive, but a retained token can keep the complete source allocation alive.
Create a compact independent value only when that retention matters at a longer-lived storage boundary.

Control the maximum split count when only a fixed number of fields is required.
Choose deliberately whether empty fields are preserved, because adjacent separators and separators at the boundaries can
otherwise disappear from the logical record.

.. erbsland-demo::
    :source: text/String/SplittingText.cpp
    :exec: text/string --demo SplittingText
    :sha256: 896253a63bcfbf156c807a48e1c21a38e680dddceb23d30ab991d284859ccc38

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRows: 4
          dag | plats | väder
          12 | Åsleden | klar
          13 |  | dimma
          14 | Nordljus | stjärnklart
        Limited split: 14 / Nordljus;stjärnklart␛[0m

.. erbsland-demo-end::

Joining belongs to construction rather than slicing.
See :doc:`/topics/text_formatting/building_strings` for the allocation differences between ``el::join()``,
``StringList::join()``, a reserved editor, and ``AnyStringBuilder``.
