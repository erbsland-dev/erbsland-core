// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `CharSet` can be created directly from a single character or explicitly from an inclusive range.
///
/// Use `fromRange()` when two `Char` values describe bounds instead of two individual allowed characters.
void characterSetCharacters() {
    auto questionMark = el::CharSet{U'?'};
    auto asciiLowercase = el::CharSet::fromRange(U'a', U'z');

    const auto yesNo = el::BooleanFormat::yesNo();
    el::io::printLine("Question marker accepts '?' : "_el, yesNo, questionMark.contains(U'?'));
    el::io::printLine("Lowercase accepts 'm' ......: "_el, yesNo, asciiLowercase.contains(U'm'));
    el::io::printLine("Lowercase accepts 'M' ......: "_el, yesNo, asciiLowercase.contains(U'M'));
}

}
