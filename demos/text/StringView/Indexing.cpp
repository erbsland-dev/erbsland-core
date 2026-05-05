// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

void printCharacterAtByByteIndex(const el::StringView &text, const el::ByteIndex byteIndex) {
    auto character = text.charAt(byteIndex);
    auto charText = el::String{};
    if (!character.isSignal()) {
        charText.append(U'\'');
        charText.append(character);
        charText.append(U'\'');
        charText = charText.toEscaped(el::EscapeFormat::Cpp);
    } else {
        charText.append("signal "_el);
        if (character.isEndOfData()) {
            charText.append("end-of-data"_el);
        } else if (character.isNoCodePoint()) {
            charText.append("no-code-point"_el);
        } else {
            charText.append("unknown"_el);
        }
    }
    el::io::printLine("  Character at byte index "_el, byteIndex, ": ", charText);
}

/// `StringView` stores text as UTF-8. This means byte positions and character
/// positions are not the same thing.
///
/// This demo shows how to:
/// - move safely between UTF-8 characters using byte indexes,
/// - convert between byte indexes and code-point indexes,
/// - handle invalid and out-of-range character access.
///
/// Note: For sequential parsing, prefer `el::StringCharReader` or `String::forEach()`.
/// Do not walk a UTF-8 string by manually incrementing indexes.
///
/// Note: Random character access in UTF-8 is inherently expensive. If you need it
/// frequently, consider `el::U32StringView` or `el::U32String`.
void indexing() {
    // Create a view for the demo. In regular code, prefer using the literal directly.
    const auto text = el::StringView{"🌲 Forêt 森"_el};

    el::io::printLine("Text: \""_el, text, "\""_el);

    // `charAt()` expects a byte index. The index should point to the start of
    // a UTF-8 sequence.
    el::io::printLine("\nCharacter access by byte index:"_el);

    // A byte index is still safe when it points into the middle of a UTF-8 sequence.
    // In that case, `charAt()` returns a replacement character.
    printCharacterAtByByteIndex(text, el::ByteIndex{1U}); // inside the first UTF-8 sequence

    // Use `advance()` and `retreat()` to move between valid character boundaries.
    auto byteIndex = el::ByteIndex::zero();
    text.advance(byteIndex);
    printCharacterAtByByteIndex(text, byteIndex);
    text.advance(byteIndex, el::CpLength{4});
    printCharacterAtByByteIndex(text, byteIndex);
    text.retreat(byteIndex);
    printCharacterAtByByteIndex(text, byteIndex);
    text.retreat(byteIndex, el::CpLength{2});
    printCharacterAtByByteIndex(text, byteIndex);

    // If you need to parse a string character by character, use `el::StringCharReader` or `String::forEach()`.
    // Never use indexes to read a string character by character, as it is inefficient and error-prone.

    // Convert a byte index to a code-point index.
    // This requires scanning the string up to the target position.
    auto cpIndex = text.toCharIndex(byteIndex);
    el::io::printLine("\nByte index "_el, byteIndex, " is code-point index "_el, cpIndex, "."_el);

    // Convert a code-point index back to a byte index.
    // This also requires scanning and can be expensive for large strings.
    cpIndex = el::CpIndex{8};
    byteIndex = text.indexAt(cpIndex);
    el::io::printLine("Code-point index "_el, cpIndex, " is byte index "_el, byteIndex, "."_el);

    // For convenience, `indexAt()` gets the first byte index and the one after the last byte.
    el::io::printLine("\nByte index range:"_el);
    el::io::printLine("  Begin: "_el, text.indexAt(el::StringSide::Front));
    el::io::printLine("  End: "_el, text.indexAt(el::StringSide::Back));

    // Out-of-range access is safe and returns a signal character.
    el::io::printLine("\nOut-of-range access:"_el);
    printCharacterAtByByteIndex(text, text.indexAt(el::StringSide::Back));
    printCharacterAtByByteIndex(text, text.indexAt(el::StringSide::Back) + el::ByteLength{5000});

    // Strict callers validate the returned signal before continuing.
    el::io::printLine("\nSignal validation:"_el);
    const auto outOfRangeChar = text.charAt(el::ByteIndex{1024});
    el::io::printLine("  out-of-range access returned signal: "_el, outOfRangeChar.isSignal() ? "yes"_el : "no"_el);
}
