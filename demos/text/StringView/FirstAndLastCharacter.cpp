// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <tuple>

/// StringView has side-based methods for accessing the first and last characters.
/// You can also slice the front and back efficiently.
///
/// Note: For writing a character by character parser, please use either `StringCharReader` or `String::forEach()`.
/// While slicing strings is efficient in this library, it is still slower than using the dedicated
/// `StringCharReader` or `String::forEach()`.
void firstAndLastCharacter() {
    const auto trail = el::StringView{"🌲 Pfad am Bach"_el};

    // Read edge characters without calculating indexes.
    el::io::printLine("Trail: "_el, trail);
    el::io::printLine("  first: "_el, trail.charAt(el::StringSide::Front));
    el::io::printLine("  last: "_el, trail.charAt(el::StringSide::Back));

    // Side-based access is convenient for small parser like tasks,
    // but use `StringCharReader` if you write a character-based parser.
    const auto [firstMarker, afterFirst] = trail.slice(el::StringSide::Front);
    const auto [lastLetter, beforeLast] = trail.slice(el::StringSide::Back);
    el::io::printLine("\nAfter slicing first "_el, firstMarker, ": "_el, afterFirst);
    el::io::printLine("Before slicing last "_el, lastLetter, ": "_el, beforeLast);

    el::io::printLine("\nSlicing first in a loop:");
    auto remaining = trail;
    el::Char slicedChar;
    while (!remaining.isEmpty()) {
        std::tie(slicedChar, remaining) = remaining.slice(el::StringSide::Front);
        el::io::printLine("  sliced: '"_el, slicedChar, "' from \""_el, remaining, "\""_el);
    }

    el::io::printLine("\nSlicing last in a loop:");
    remaining = trail;
    while (!remaining.isEmpty()) {
        std::tie(slicedChar, remaining) = remaining.slice(el::StringSide::Back);
        el::io::printLine("  sliced: '"_el, slicedChar, "' from \""_el, remaining, "\""_el);
    }
}
