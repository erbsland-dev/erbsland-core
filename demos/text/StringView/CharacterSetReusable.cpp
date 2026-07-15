// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Reuse named `CharSet` objects for validation policies that are applied repeatedly.
///
/// Building a set once makes the policy easier to read and avoids reconstructing category or pattern based sets in hot
/// paths.
void characterSetReusable() {
    static const auto optionNameChars = el::CharSet::fromPattern("-_a-zA-Z0-9"_el);

    const auto optionNames = el::StringViewList{
        "orbite-07"_el,
        "antenne_nord"_el,
        "équipe-science"_el,
        "module solaire"_el,
    };

    const auto yesNo = el::BooleanFormat::yesNo();
    optionNames.forEach([&](const el::StringView &optionName) -> void {
        el::io::printLine(optionName, " -> "_el, yesNo, optionName.containsOnly(optionNameChars));
    });
}

}
