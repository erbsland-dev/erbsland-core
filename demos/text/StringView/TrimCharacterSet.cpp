// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `StringView::trimmed()` returns a view with selected characters removed from the front, back, or both sides.
///
/// With a custom `CharSet`, trimming is not limited to whitespace.
void trimCharacterSet() {
    const auto raw = el::StringView{"*** signal-orbite ;; "_el};
    static const auto border = el::CharSet{" *;"_el};

    auto clean = raw.trimmed(border);
    auto frontOnly = raw.trimmed(border, el::StringSide::Front);
    auto backOnly = raw.trimmed(border, el::StringSide::Back);

    el::io::printLine("Raw .......: \"", raw, "\""_el);
    el::io::printLine("Both sides : \"", clean, "\""_el);
    el::io::printLine("Front only : \"", frontOnly, "\""_el);
    el::io::printLine("Back only .: \"", backOnly, "\""_el);
}

}
