// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/AsciiCategory.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/EscapeFormat.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

namespace demo {

/// `String::removedAll()` removes every decoded character from a selected `CharSet`.
///
/// This is useful for simple cleanup passes where unwanted characters may occur anywhere in the text.
/// Inverting an allowed set removes every character outside that policy.
void removeCharacters() {
    static const auto controlChars = el::CharSet::from(el::AsciiCategory::Control);

    const auto input = el::String{"rapport\torbite\nstable"_el};
    auto logLine = input.removedAll(controlChars);

    static const auto allowedChars = el::CharSet::fromPattern(" a-z"_el);
    const auto stationName = el::String{"orbite! 07"_el};
    el::io::printLine("Allowed  : "_el, stationName.removedAll(~allowedChars));

    el::io::printLine("Original : "_el, input.toEscaped(el::EscapeFormat::Cpp));
    el::io::printLine("Cleaned  : "_el, logLine.toEscaped(el::EscapeFormat::Cpp));
}

}
