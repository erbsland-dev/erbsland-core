// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

void countEmojis(const el::StringView &text);

/// `StringView` is the preferred parameter type for functions that read text.
///
/// It accepts common string inputs naturally:
/// - string literals are used directly without copying,
/// - existing views share their backend,
/// - editable strings provide a read-only view to their contents.
///
/// From the caller's perspective, all variants behave the same.
void idealFunctionParameter() {
    // A string literal can be passed directly.
    countEmojis("🌲🌲 Waldkonzert mit Fuchs 🦊 und Eule 🦉"_el);

    // An existing view can be passed without copying.
    const auto stringView = el::StringView{"Pluie douce sur les fleurs 🌧️🌷🌼"_el};
    countEmojis(stringView);

    // An editable string is accepted as read-only input.
    const auto stringEdit = el::String{"Bosque nocturno: luna 🌙, estrellas ✨ y grillos 🦗"_el};
    countEmojis(stringEdit);
}

/// Count all emoji-like symbols in `text` and print the result.
///
/// The function only needs read-only access to the text. It does not need to know
/// whether the caller passed a literal, a view, or an editable string.
void countEmojis(const el::StringView &text) {
    std::size_t emojiCount = 0;
    text.forEach([&](const el::Char character) mutable noexcept -> el::util::LoopStatus {
        if (character.isCategory(el::UnicodeCategory::OtherSymbol)) {
            ++emojiCount;
        }
        return el::LoopStatus::Continue;
    });

    el::io::printLine("Text ..........: \"", text, "\"");
    el::io::printLine("Emoji symbols .: ", emojiCount);
    el::io::printLine();
}

}
