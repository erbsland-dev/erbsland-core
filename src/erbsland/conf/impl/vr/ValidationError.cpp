// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValidationError.hpp"

#include "Rule.hpp"

#include "../value/Value.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

auto expectedRuleTypesText(const std::vector<vr::RuleType> &ruleTypes) -> text::String {
    text::StringEditor result;
    for (std::size_t i = 0; i != ruleTypes.size(); ++i) {
        result.append(ruleTypes[i].expectedValueTypeText());
        if (i < ruleTypes.size() - 1) {
            if (i == ruleTypes.size() - 2) {
                result.append(", or "_el);
            } else {
                result.append(", "_el);
            }
        }
    }
    return result;
}

auto expectedValueTypeText(const RulePtr &rule, const Integer version) -> text::String {
    text::String result;
    if (rule->type() == vr::RuleType::Alternatives) {
        std::vector<vr::RuleType> expectedRuleTypes;
        for (const auto &alternativeRule : rule->childrenImpl()) {
            if (alternativeRule->versionMask().matches(version)) {
                if (std::ranges::find(expectedRuleTypes, alternativeRule->type()) == expectedRuleTypes.end()) {
                    expectedRuleTypes.push_back(alternativeRule->type());
                }
            }
        }
        result = expectedRuleTypesText(expectedRuleTypes);
    } else {
        result = rule->type().expectedValueTypeText();
    }
    return result;
}

void throwExpectedVsActual(const RulePtr &rule, const ValuePtr &value, const Integer version) {
    throwValidationError(
        text::StringFormat{"Expected {} but got {}"_el}.build(
            expectedValueTypeText(rule, version), value->type().toValueDescription(true)),
        value->namePath(),
        value->location());
}

}
