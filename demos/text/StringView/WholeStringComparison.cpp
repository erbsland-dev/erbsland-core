// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// `StringView` compares whole strings by decoded Unicode code point.
/// Use the comparison operators for ordinary equality and ordering. Use `compare()`
/// when you need the `std::strong_ordering` result explicitly or want to pass a
/// character comparison function. The common UTF-8 aliases compare with other UTF-8
/// strings, views, and `"_el"` literals; UTF-16 and UTF-32 variants follow the same
/// same-width pattern.
void wholeStringComparison() {
    const auto tag = el::StringView{"lišejník"_el};
    const auto sameView = el::StringView{"lišejník"_el};
    const auto editableTag = "lišejník"_els;
    const auto booleanFormat = el::BooleanFormat::yesNo();

    // Compare a view with another view, an editable string, and a literal.
    el::io::printLine("tag == sameView ...............: "_el, booleanFormat, tag == sameView);
    el::io::printLine("tag == editableTag ............: "_el, booleanFormat, tag == editableTag);
    el::io::printLine("tag == \"lišejník\"_el ..........: "_el, booleanFormat, tag == "lišejník"_el);
    el::io::printLine("tag != \"tuleň\"_el .............: "_el, booleanFormat, tag != "tuleň"_el);

    // Ordering uses decoded code points, so uppercase and lowercase letters differ.
    auto ordering = tag <=> "tuleň"_el;
    el::io::printLine("tag <=> \"tuleň\"_el ............: "_el, el::toString(ordering));
    ordering = tag.compare("LIŠEJNÍK"_el);
    el::io::printLine("tag.compare(\"LIŠEJNÍK\"_el) ....: "_el, el::toString(ordering));
    el::io::printLine("tag < \"tuleň\"_el ..............: "_el, booleanFormat, tag < "tuleň"_el);
    el::io::printLine("tag <= sameView ...............: "_el, booleanFormat, tag <= sameView);
    el::io::printLine("tag >= \"arka\"_el ..............: "_el, booleanFormat, tag >= "arka"_el);

    // Width-specific strings use the same operations with values of the same width.
    const auto u8Habitat = u8"ledová kra"_elv;
    const auto u16Habitat = u"ledová kra"_elv;
    const auto u32Habitat = U"ledová kra"_elv;

    el::io::printLine("u8Habitat == u8 literal .......: "_el, booleanFormat, u8Habitat == u8"ledová kra"_el);
    el::io::printLine("u16Habitat == u16 literal .....: "_el, booleanFormat, u16Habitat == u"ledová kra"_el);
    el::io::printLine("u32Habitat == u32 literal .....: "_el, booleanFormat, u32Habitat == U"ledová kra"_el);
}
