// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// `StringView::removedAll()` removes every decoded character from a selected `CharSet`.
///
/// This is useful for simple cleanup passes where unwanted characters may occur anywhere in the text.
void removeCharacters() {
    static const auto controlChars = el::CharSet::from(el::AsciiCategory::Control);

    const auto input = el::StringView{"rapport\torbite\nstable"_el};
    auto logLine = input.removedAll(controlChars);

    el::io::printLine("Original : "_el, input.toEscaped(el::EscapeFormat::Cpp));
    el::io::printLine("Cleaned  : "_el, logLine.toEscaped(el::EscapeFormat::Cpp));
}
