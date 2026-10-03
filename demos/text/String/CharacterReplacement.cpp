// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SharedTools.hpp"

#include <DemoCommon.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/UnicodeCategory.hpp>

namespace demo {

/// A `String` allows building a new string with text or characters replaced.
/// A character set selects individual characters; search text selects complete sequences.
/// Unchanged results share the source storage, including repeated identical replacements.
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
    auto plainCompact = note.removedAll(emojisAndSpace);
    el::io::printLine("Plain .......: \""_el, plainCompact, "\""_el);

    // Add a bit of sunshine by replacing all `🍃 Wind` with `☀️ Sun`
    auto sunnySpots = note.replacedAll("🍃 Wind"_el, "☀️ Sun"_el);
    el::io::printLine("Sunny .......: \""_el, sunnySpots, "\""_el);

    // Repeat the replacement after all wind entries have already been replaced.
    auto unchanged = sunnySpots.replacedAll("🍃 Wind"_el, "☀️ Sun"_el);
    el::io::printLine("\nNo change, no copy:"_el);
    el::io::printLine("Before ......: \""_el, sunnySpots, "\""_el);
    el::io::printLine("After .......: \""_el, unchanged, "\""_el);
    el::io::printLine("Same storage : "_el, unchanged.storageId() == sunnySpots.storageId());
}

}
