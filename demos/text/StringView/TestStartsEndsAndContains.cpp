// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// This demo shows how to easily test if a string view starts, ends, or contains a specific substring.
void testStartsEndsAndContains() {
    // Creating the view is only for this demo, use the literals directly in your productive code.
    const auto thePond = el::StringView{"古池や 蛙飛びこむ 水の音"_el};
    const auto flowers = el::StringView{"İstanbul ormanında Işık ve çiçekler"_el};

    // This library is safe to work with incorrectly encoded UTF-8 data, encoding errors are replaced
    // with the Unicode replacement character (U+FFFD).
    el::io::printLine("thePond ...: \""_el, thePond, "\""_el);
    el::io::printLine("flowers ...: \""_el, flowers, "\""_el);

    // Test if a text starts with, ends with or contains a given text
    el::io::printLine("\nTest parts of the strings (case-sensitive):"_el);
    auto result = thePond.startsWith("古池"_el);
    el::io::printLine("  thePond.startsWith(\"古池\") → "_el, result);
    result = thePond.contains("蛙"_el);
    el::io::printLine("  thePond.contains(\"蛙\") → "_el, result);
    result = thePond.endsWith("水の音"_el);
    el::io::printLine("  thePond.endsWith(\"水の音\") → "_el, result);

    // Single-character tests use the same text API.
    result = thePond.startsWith("古"_el);
    el::io::printLine("  thePond.startsWith(\"古\"_el) → "_el, result);
    result = thePond.contains("⛵"_el);
    el::io::printLine("  thePond.contains(\"⛵\"_el) → "_el, result);
    result = thePond.endsWith("音"_el);
    el::io::printLine("  thePond.endsWith(\"音\"_el) → "_el, result);

    // Case-insensitive tests pass an explicit comparison function.
    el::io::printLine("\nTest parts of the strings (case-insensitive):"_el);
    result = flowers.startsWith("istanbul"_el, el::Char::compareCaseFolded);
    el::io::printLine("  flowers.startsWith(\"istanbul\"_el, el::Char::compareCaseFolded) → "_el, result);
    result = flowers.contains("ışık"_el, el::Char::compareCaseFolded);
    el::io::printLine("  flowers.contains(\"ışık\"_el, el::Char::compareCaseFolded) → "_el, result);
    result = flowers.endsWith("ÇİÇEKLER"_el, el::Char::compareCaseFolded);
    el::io::printLine("  flowers.endsWith(\"ÇİÇEKLER\"_el, el::Char::compareCaseFolded) → "_el, result);

    // Also the same case-folded tests work for single-character strings.
    result = flowers.startsWith("İ"_el, el::Char::compareCaseFolded);
    el::io::printLine("  flowers.startsWith(\"İ\"_el, el::Char::compareCaseFolded) → "_el, result);
    result = flowers.contains("ı"_el, el::Char::compareCaseFolded);
    el::io::printLine("  flowers.contains(\"ı\"_el, el::Char::compareCaseFolded) → "_el, result);
    result = flowers.endsWith("ç"_el, el::Char::compareCaseFolded);
    el::io::printLine("  flowers.endsWith(\"ç\"_el, el::Char::compareCaseFolded) → "_el, result);
}
