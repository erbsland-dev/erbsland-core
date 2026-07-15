// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

auto describeCharacter(const el::Char character) -> el::String {
    if (character.isEndOfData()) {
        return el::String{"end-of-data"_el};
    }
    if (character.isNoCodePoint()) {
        return el::String{"no-code-point"_el};
    }
    auto result = el::String{};
    result.append(U'\'');
    result.append(character);
    result.append(U'\'');
    return result.toEscaped(el::EscapeFormat::Cpp);
}

/// `StringView` gives fast access to UTF-8 byte positions and explicit tools for
/// moving between decoded code points.
///
/// Use `charAt(StringSide)` for the first or last character, use byte indexes
/// when a previous string operation already returned one, and use
/// `advance()`/`retreat()` to keep byte indexes on character boundaries.
/// Code-point indexes are convenient but require scanning UTF-8 text; reserve
/// them for small strings or specialized code.
void characterAccess() {
    const auto text = el::StringView{"ödev📚:matematik"_el};

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

    // Slicing one character from a side is efficient and keeps the rest as a view.
    const auto [firstCharacter, withoutFirst] = text.slice(el::StringSide::Front);
    const auto [lastCharacter, withoutLast] = text.slice(el::StringSide::Back);
    el::io::printLine("Sliced front: "_el, describeCharacter(firstCharacter), " | rest: "_el, withoutFirst);
    el::io::printLine("Sliced back: "_el, describeCharacter(lastCharacter), " | rest: "_el, withoutLast);

    // `StringCharView` exposes character-indexed helpers for specialized small-text work.
    auto charView = text.toCharView();
    el::io::printLine("Char view prefix: "_el, charView.slice(el::StringSide::Front, el::CpLength{4U}));
}

}
