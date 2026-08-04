// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `compare()` accepts character comparison functions for specialized matching rules.
/// Use `Char::compareCaseFolded` for Unicode-aware case-insensitive text, use
/// `Char::compareAsciiFolded` when ASCII-only folding is enough, and use
/// `Char::compareIdentifier` for identifier-like names where ASCII case and spaces
/// versus underscores should compare equally.
void comparisonFunctions() {
    const auto snowyOwl = el::String{"SOVA SNĚŽNÍ"_el};
    const auto snowyOwlLower = el::String{"sova sněžní"_el};
    const auto asciiLabel = el::String{"Polar Fox"_el};
    const auto asciiLabelLower = el::String{"polar fox"_el};
    const auto identifier = el::String{"Arctic Fox Trail"_el};
    const auto normalizedIdentifier = el::String{"arctic_fox_trail"_el};

    // Regular comparison is exact and uses decoded code points.
    el::io::printLine(
        "snowyOwl.compare(snowyOwlLower) .........................: "_el,
        el::toString(snowyOwl.compare(snowyOwlLower)));

    // Unicode case folding handles non-ASCII letters such as `Ě` and `ě`.
    el::io::printLine(
        "snowyOwl.compare(..., Char::compareCaseFolded) ..........: "_el,
        el::toString(snowyOwl.compare(snowyOwlLower, el::Char::compareCaseFolded)));

    // ASCII folding is small and fast, but only changes A-Z to a-z.
    el::io::printLine(
        "asciiLabel.compare(..., Char::compareAsciiFolded) .......: "_el,
        el::toString(asciiLabel.compare(asciiLabelLower, el::Char::compareAsciiFolded)));
    el::io::printLine(
        "snowyOwl.compare(..., Char::compareAsciiFolded) .........: "_el,
        el::toString(snowyOwl.compare(snowyOwlLower, el::Char::compareAsciiFolded)));

    // Identifier comparison is useful for normalized keys or configuration-style names.
    el::io::printLine(
        "identifier.compare(..., Char::compareIdentifier) ........: "_el,
        el::toString(identifier.compare(normalizedIdentifier, el::Char::compareIdentifier)));
}

}
