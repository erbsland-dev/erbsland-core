// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

/// Erbsland Core uses a rich API approach to keep boilerplate to a minimum.
/// The common case stays short, readable, and maintainable.
/// More dangerous and error-prone code is intentionally more explicit.
/// This demo compares Erbsland Core code with equivalent standard-library code.
void stdVsCoreCaseFold() {
    el::io::printLine("=== Unicode-aware case folding ==="_el);

    // Erbsland Core: Case folding works on Unicode characters.
    constexpr auto elForestText = "Straße İSTANBUL CamelCaseText"_el;
    const auto coreCaseFolded = el::StringView{elForestText}.transformed(el::Char::caseFolded);
    el::io::printLine("Core: "_el, coreCaseFolded);

    // Standard library: std::tolower works on bytes/chars, not Unicode text.
    constexpr auto stdForestText = std::string_view{"Straße İSTANBUL CamelCaseText"};
    auto stdLower = std::string{stdForestText};
    std::ranges::transform(stdLower, stdLower.begin(), [](unsigned char character) -> char {
        return static_cast<char>(std::tolower(character));
    });
    el::io::printLine("std:  "_el, stdLower, "  (ASCII-oriented)"_el);
}
