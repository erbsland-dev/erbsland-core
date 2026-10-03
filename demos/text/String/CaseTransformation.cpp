// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringTransformations.hpp>

namespace demo {

/// String transformation helpers return Unicode or ASCII case variants without changing the source.
/// Simple Unicode mappings preserve the code-point count, while ASCII mappings leave non-ASCII letters unchanged.
/// The original storage can be reused when the transformation does not change the text.
void caseTransformation() {
    const auto title = el::String{"Forêt d'Été, Σκιερό Μονοπάτι"_el};

    // Derive case variants using simple Unicode mappings.
    el::io::printLine("Original ......: "_el, title);
    el::io::printLine("Lowercase .....: "_el, el::toLowercase(title));
    el::io::printLine("Uppercase .....: "_el, el::toUppercase(title));
    el::io::printLine("Case folded ...: "_el, el::toCaseFolded(title));

    // Using only ASCII methods can be faster and can avoid linking the Unicode database into the executable.
    const auto sensorName = el::String{"TEMP-ÄSTHETIK-07"_el};
    el::io::printLine("\nSensor name ...: "_el, sensorName);
    el::io::printLine("ASCII lower ...:  "_el, el::toAsciiLowercase(sensorName));
}

}
