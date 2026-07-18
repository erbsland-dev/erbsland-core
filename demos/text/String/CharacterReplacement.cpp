// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SharedTools.hpp"

#include <DemoCommon.hpp>

namespace demo {

/// A `String` allows building a new string with text or characters replaced.
/// The replacement happens in one pass, without creating intermediate copies.
/// Also, if no text is replaced, the original string is returned.
void characterReplacement() {
    const auto note = el::String{"⛈️ Gewitter; 🍃 Wind; 🌫 Nebel; ☔️ Regen; 🍃 Wind"_el};
    const auto semicolon = el::CharSet{U';'};
    auto emojisAndSpace = el::CharSet::from(el::UnicodeCategory::OtherSymbol);
    emojisAndSpace.add(U' ');

    // Replace all semicolons with a comma.
    el::io::printLine("Original ....: \""_el, note, "\""_el);
    auto withComma = note.replacedAll(semicolon, U',');
    el::io::printLine("Compact .....: \""_el, withComma, "\""_el);

    // Replace the semicolon + space sequence with a longer ` → ` sequence.
    auto withArrow = note.replacedAll("; "_el, " → "_el);
    el::io::printLine("Arrows ......: \""_el, withArrow, "\""_el);

    // Remove the emojis and space from the text.
    auto plainCompact = note.replacedAll(emojisAndSpace, el::String{});
    el::io::printLine("Plain .......: \""_el, plainCompact, "\""_el);

    // Add a bit of sunshine by replacing all `🍃 Wind` with `☀️ Sun`
    auto sunnySpots = note.replacedAll("🍃 Wind"_el, "☀️ Sun"_el);
    el::io::printLine("Sunny .......: \""_el, sunnySpots, "\""_el);

    // When no text gets replaced, the original string is returned
    el::io::printLine("\nNo change, no copy:");
    el::io::printLine("Original ....: \""_el, note, "\""_el);

    el::io::printLine("Sunny .......: \""_el, sunnySpots, "\""_el);
}

}
