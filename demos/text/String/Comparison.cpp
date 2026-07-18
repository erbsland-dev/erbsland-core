// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// This demo shows the various ways `String` instances can be compared.
void comparison() {
    // Here we create a set of different read-only strings.
    // For real code use the literals directly, like `const auto x = "abc"_el;`
    const auto titlecase = el::String{"Fichte"_el};
    const auto lowercase = el::String{"fichte"_el};
    const auto greek = el::String{"Σκιά"_el};
    const auto greekLower = el::String{"σκιά"_el};
    const auto camelCaseIdentifier = el::String{"Small Mountain River"_el};
    const auto normalizedIdentifier = el::String{"small_mountain_river"_el};

    el::io::printLine("titlecase ............: "_el, titlecase);
    el::io::printLine("lowercase ............: "_el, lowercase);
    el::io::printLine("greek ................: "_el, greek);
    el::io::printLine("greekLower ...........: "_el, greekLower);
    el::io::printLine("camelCaseIdentifier ..: "_el, camelCaseIdentifier);
    el::io::printLine("normalizedIdentifier .: "_el, normalizedIdentifier);
    el::io::printLine();

    // Comparison using the == operator always compares the two strings case-sensitive on a code-point basis.
    const auto booleanFormat = el::BooleanFormat::yesNo();
    auto boolResult = titlecase == lowercase;
    el::io::printLine("titlecase == lowercase → "_el, booleanFormat, boolResult);
    // For a Unicode case-insensitive comparison, use the function-based comparison.
    auto ordering = titlecase.compare(lowercase, el::Char::compareCaseFolded);
    el::io::printLine("titlecase.compare(lowercase, el::Char::compareCaseFolded) → "_el, el::toString(ordering));
    ordering = greek.compare(greekLower, el::Char::compareCaseFolded);
    el::io::printLine("greek.compare(greekLower, el::Char::compareCaseFolded) → "_el, el::toString(ordering));
    // If comparing ASCII upper/lower case is enough, use the function-based comparison.
    ordering = titlecase.compare(lowercase, el::Char::compareAsciiFolded);
    el::io::printLine("titlecase.compare(lowercase, el::Char::compareAsciiFolded) → "_el, el::toString(ordering));
    // Working with normalized configuration identifiers, like in the Erbsland Configuration Language
    ordering = camelCaseIdentifier.compare(normalizedIdentifier, el::Char::compareIdentifier);
    el::io::printLine(
        "camelCaseIdentifier.compare(normalizedIdentifier, el::Char::compareIdentifier) → "_el, el::toString(ordering));

    // Ordering uses decoded code points and works with the normal comparison operators.
    el::io::printLine("\nAlphabetic order by code point:"_el);
    // Comparing string literals directly is the most efficient way when possible.
    boolResult = titlecase < "Tanne"_el;
    el::io::printLine("  titlecase < \"Tanne\"_el → "_el, booleanFormat, boolResult);
    ordering = titlecase <=> "BIRKE"_el;
    el::io::printLine("  titlecase <=> \"BIRKE\"_el → "_el, el::toString(ordering));
    ordering = titlecase.compare("BIRKE"_el);
    el::io::printLine("  titlecase.compare(\"BIRKE\"_el) → "_el, el::toString(ordering));
    ordering = titlecase <=> "birke"_el;
    el::io::printLine("  titlecase <=> \"birke\"_el → "_el, el::toString(ordering));
    ordering = titlecase.compare("birke"_el);
    el::io::printLine("  titlecase.compare(\"birke\"_el) → "_el, el::toString(ordering));

    // Ordering also works with Unicode case-folded strings.
    el::io::printLine("\nUnicode case-folded order:"_el);
    ordering = titlecase.compare("birke"_el, el::Char::compareCaseFolded);
    el::io::printLine("  titlecase.compare(\"birke\"_el, el::Char::compareCaseFolded) → "_el, el::toString(ordering));
    ordering = titlecase.compare("BIRKE"_el, el::Char::compareCaseFolded);
    el::io::printLine("  titlecase.compare(\"BIRKE\"_el, el::Char::compareCaseFolded) → "_el, el::toString(ordering));
}

}
