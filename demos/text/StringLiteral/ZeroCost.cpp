// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// Import the `_el` suffix for convenient string literals.
using namespace el::text::literals;

/// String literals are lightweight read-only references.
constexpr auto cForestStory = "🌲 Im Wald rauscht der Wind."_el;
constexpr auto cRiverStory = el::StringLiteral{"川の水は静かに流れます。"};

/// String literals are created entirely at compile time.
///
/// Both `""_el` and `el::StringLiteral{"..."}` produce lightweight references
/// to read-only memory. Copying them is inexpensive because no character data
/// is duplicated.
void zeroCost() {
    // Inspect a literal created with the `_el` suffix.
    el::io::printLine("Forest story .: "_el, cForestStory);
    el::io::printLine("Valid UTF-8 ..: "_el, cForestStory.isValidUtf8());
    el::io::printLine("Length .......: "_el,
        cForestStory.length(),
        " bytes / "_el,
        cForestStory.characterLength(),
        " code-points"_el);

    // The same functionality using `el::StringLiteral`.
    el::io::printLine("\nRiver story ..: "_el, cRiverStory);
    el::io::printLine("Valid UTF-8 ..: "_el, cRiverStory.isValidUtf8());
    el::io::printLine(
        "Length .......: "_el, cRiverStory.length(), " bytes / "_el, cRiverStory.characterLength(), " code-points"_el);

    // Copying a literal only copies the lightweight reference.
    auto storyCopy = cForestStory;
    el::io::printLine("\nCopy .........: "_el, storyCopy);
    el::io::printLine("Length .......: "_el, storyCopy.length(), " bytes"_el);
}
