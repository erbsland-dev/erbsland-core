// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

void countEmojis(const el::String &text);

/// `String` is the preferred parameter type for functions that read text.
///
/// It accepts common string inputs naturally:
/// - string literals are used directly without copying,
/// - existing strings share their storage,
/// - local editors convert safely to read-only strings.
///
/// From the caller's perspective, all variants behave the same.
void idealFunctionParameter() {
    // A string literal can be passed directly.
    countEmojis("🌲🌲 Waldkonzert mit Fuchs 🦊 und Eule 🦉"_el);

    // An existing string can be passed without copying.
    const auto string = el::String{"Pluie douce sur les fleurs 🌧️🌷🌼"_el};
    countEmojis(string);

    // A short value assembled with an editor is accepted as read-only input.
    auto constructed = el::StringEditor{"Bosque nocturno: "_el};
    constructed.append("luna 🌙 y grillos 🦗"_el);
    countEmojis(constructed);
}

/// Count all emoji-like symbols in `text` and print the result.
///
/// The function only needs read-only access to the text. It does not need to know
/// whether the caller passed a literal, a string, or a short constructed value.
void countEmojis(const el::String &text) {
    std::size_t emojiCount = 0;
    text.forEach([&](const el::Char character) mutable noexcept -> el::util::LoopStatus {
        if (character.isCategory(el::UnicodeCategory::OtherSymbol)) {
            ++emojiCount;
        }
        return el::LoopStatus::Continue;
    });

    el::io::printLine("Text ..........: \""_el, text, "\""_el);
    el::io::printLine("Emoji symbols .: ", emojiCount);
    el::io::printLine();
}

}
