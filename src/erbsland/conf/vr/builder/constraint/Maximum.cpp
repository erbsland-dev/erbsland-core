// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Maximum.hpp"

#include "../../../impl/vr/MinMaxConstraint.hpp"
#include "../../../impl/vr/MinMaxDateConstraint.hpp"
#include "../../../impl/vr/MinMaxDateTimeConstraint.hpp"
#include "../../../impl/vr/MinMaxFloatConstraint.hpp"
#include "../../../impl/vr/MinMaxIntegerConstraint.hpp"
#include "../../../impl/vr/MinMaxMatrixConstraint.hpp"

#include <type_traits>

namespace erbsland::conf::vr::builder {

void Maximum::operator()(Rule &rule) {
    auto constraint = std::visit(
        [&rule](const auto &value) -> impl::ConstraintPtr {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, Integer>) {
                requireRuleTypeForConstraint(
                    rule,
                    "maximum"_el,
                    {vr::RuleType::Integer,
                        vr::RuleType::Text,
                        vr::RuleType::Bytes,
                        vr::RuleType::ValueList,
                        vr::RuleType::Section,
                        vr::RuleType::SectionList,
                        vr::RuleType::SectionWithTexts});
                return std::make_shared<impl::MinMaxIntegerConstraint>(impl::MinMaxConstraint::Max, value);
            } else if constexpr (std::is_same_v<T, Float>) {
                requireRuleTypeForConstraint(rule, "maximum"_el, {vr::RuleType::Float});
                return std::make_shared<impl::MinMaxFloatConstraint>(impl::MinMaxConstraint::Max, value);
            } else if constexpr (std::is_same_v<T, time::Date>) {
                requireRuleTypeForConstraint(rule, "maximum"_el, {vr::RuleType::Date});
                return std::make_shared<impl::MinMaxDateConstraint>(impl::MinMaxConstraint::Max, value);
            } else if constexpr (std::is_same_v<T, time::DateTime>) {
                requireRuleTypeForConstraint(rule, "maximum"_el, {vr::RuleType::DateTime});
                return std::make_shared<impl::MinMaxDateTimeConstraint>(impl::MinMaxConstraint::Max, value);
            } else {
                requireRuleTypeForConstraint(rule, "maximum"_el, {vr::RuleType::ValueMatrix});
                return std::make_shared<impl::MinMaxMatrixConstraint>(
                    impl::MinMaxConstraint::Max, value.first, value.second);
            }
        },
        _value);
    _options.addToRule(rule, constraint, "maximum"_el);
}

}
