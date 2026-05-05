// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// The default `CharSet` constructor creates an empty set.
///
/// Empty sets are useful for disabled filters and for policies where no character is allowed.
void characterSetEmpty() {
    auto disabledFilter = el::CharSet{};
    const auto sample = el::StringView{"orbite"_el};

    const auto yesNo = el::BooleanFormat::yesNo();
    el::io::printLine("Set is empty ................: "_el, yesNo, disabledFilter.isEmpty());
    el::io::printLine("Sample contains one of set ..: "_el, yesNo, sample.containsOneOf(disabledFilter));
    el::io::printLine("Sample contains only set ....: "_el, yesNo, sample.containsOnly(disabledFilter));
    el::io::printLine("Empty text contains only set : "_el, yesNo, el::StringView{}.containsOnly(disabledFilter));
}
