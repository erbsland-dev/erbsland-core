// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/Capitalization.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringTransformations.hpp>

namespace demo {

/// Prepare an expedition label for display, a type name, and a machine-readable key.
/// Whitespace simplification keeps word boundaries; camel-case conversion joins or separates words.
/// Explicit capitalization applies to each word, including the first.
void wordTransformations() {
    // Clean spacing while keeping the original spelling of each word.
    const auto fieldLabel = el::String{"  temperatura\t del   mare  "_el};
    const auto label = el::toWhitespaceSimplified(fieldLabel);
    el::io::printLine("Label .......: "_el, label);

    // Derive names for a schema and its exported fields.
    const auto typeName = el::toCamelCaseFromWords(label, el::Capitalization::Capitalized);
    const auto heading = el::toWordsFromCamelCase(typeName);
    const auto fieldKey = el::toAsciiWordsFromCamelCase(typeName, U'_', el::Capitalization::Lowercase);
    el::io::printLine("Type name ...: "_el, typeName);
    el::io::printLine("Heading .....: "_el, heading);
    el::io::printLine("Field key ...: "_el, fieldKey);

    // Underscores and hyphens are separators only when the input format declares them.
    static const auto separators = el::CharSet{"_-"_el};
    const auto sensorKey = el::String{"sonda_CTD-mare"_el};
    el::io::printLine("Joined key ..: "_el, el::toCamelCaseFromWords(sensorKey, separators));
}

}
