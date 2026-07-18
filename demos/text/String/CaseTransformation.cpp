// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `String::transformed()` maps decoded characters into a new string.
/// You can use Unicode-aware operations, ASCII-only operations, or a custom mapping function.
/// The original storage can be reused when the transformation does not change the text.
void caseTransformation() {
    const auto title = el::String{"Forêt d'Été, Σκιερό Μονοπάτι"_el};

    // Normalize display text with Unicode-aware operations.
    el::io::printLine("Original ......: "_el, title);
    el::io::printLine("Lowercase .....: "_el, title.transformed(el::Char::toLowercase));
    el::io::printLine("Uppercase .....: "_el, title.transformed(el::Char::toUppercase));
    el::io::printLine("Case folded ...: "_el, title.transformed(el::Char::caseFolded));

    // Using only ASCII methods can be faster and can avoid linking the Unicode database into the executable.
    const auto sensorName = el::String{"TEMP-ÄSTHETIK-07"_el};
    el::io::printLine("\nSensor name ...: "_el, sensorName);
    el::io::printLine("ASCII lower ...:  "_el, sensorName.transformed(el::Char::toAsciiLowercase));

    // A custom transform can map individual decoded code points.
    const auto quietLabel = el::String{"wind: leise, regen: sanft"_el};
    const auto highlighted = quietLabel.transformed(
        [](const el::Char character) noexcept -> el::Char { return character == U':' ? U'→' : character; });
    el::io::printLine("\nCustom map ..:   "_el, highlighted);
}

}
