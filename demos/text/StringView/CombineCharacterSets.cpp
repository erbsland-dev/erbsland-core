// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `CharSet` objects can be combined and compared to express larger validation policies.
///
/// Use set operations to build the final policy from named parts, then use subset checks when one policy must remain
/// inside another.
void combineCharacterSets() {
    static const auto letters = el::CharSet::from(el::UnicodeCategoryGroup::Letter);
    static const auto digits = el::CharSet::from(el::UnicodeCategory::DecimalNumber);
    static const auto identifierStart = letters | el::CharSet{U'_'};
    static const auto identifierContinue = identifierStart | digits | el::CharSet{U'-'};

    const auto identifier = el::StringView{"orbite-7"_el};
    const auto firstCharacterOk = identifierStart.contains(identifier.charAt(el::StringSide::Front));
    const auto fullIdentifierOk = identifier.containsOnly(identifierContinue);

    const auto yesNo = el::BooleanFormat::yesNo();
    el::io::printLine("Start policy is subset ....: "_el, yesNo, identifierStart.isSubsetOf(identifierContinue));
    el::io::printLine("First character accepted ..: "_el, yesNo, firstCharacterOk);
    el::io::printLine("Identifier accepted .......: "_el, yesNo, fullIdentifierOk);
}

}
