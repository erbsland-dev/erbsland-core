// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// Byte-range slicing is the fast path for cutting `StringView` data into
/// smaller views.
///
/// Search operations such as `find()` return byte indexes. You can use these
/// indexes directly to build `ByteRange` values and pass them to `slice()`.
/// The resulting string views refer to the same backing text and do not copy
/// the selected bytes.
void byteRangeSlicing() {
    const auto journal = el::StringView{"dag=12|plats=Norrpasset|väder=klar|signal=stjärna"_el};

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
