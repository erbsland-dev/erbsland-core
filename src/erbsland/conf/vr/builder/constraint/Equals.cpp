// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Equals.hpp"

#include "../../../impl/vr/EqualsConstraint.hpp"

#include <type_traits>

namespace erbsland::conf::vr::builder {

void Equals::operator()(impl::Rule &rule) {
    auto constraint = std::visit(
        [&rule](const auto &value) -> impl::ConstraintPtr {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, Integer>) {
                requireRuleTypeForConstraint(
                    rule,
                    "equals"_el,
                    {vr::RuleType::Integer,
                        vr::RuleType::Text,
                        vr::RuleType::Bytes,
                        vr::RuleType::ValueList,
                        vr::RuleType::Section,
                        vr::RuleType::SectionList,
                        vr::RuleType::SectionWithTexts});
                return std::make_shared<impl::EqualsIntegerConstraint>(value);
            } else if constexpr (std::is_same_v<T, bool>) {
                requireRuleTypeForConstraint(rule, "equals"_el, {vr::RuleType::Boolean});
                return std::make_shared<impl::EqualsBooleanConstraint>(value);
            } else if constexpr (std::is_same_v<T, Float>) {
                requireRuleTypeForConstraint(rule, "equals"_el, {vr::RuleType::Float});
                return std::make_shared<impl::EqualsFloatConstraint>(value);
            } else if constexpr (std::is_same_v<T, text::String>) {
                requireRuleTypeForConstraint(rule, "equals"_el, {vr::RuleType::Text});
                return std::make_shared<impl::EqualsTextConstraint>(value);
            } else if constexpr (std::is_same_v<T, mem::ByteBlock>) {
                requireRuleTypeForConstraint(rule, "equals"_el, {vr::RuleType::Bytes});
                return std::make_shared<impl::EqualsBytesConstraint>(value);
            } else {
                requireRuleTypeForConstraint(rule, "equals"_el, {vr::RuleType::ValueMatrix});
                return std::make_shared<impl::EqualsMatrixConstraint>(value.first, value.second);
            }
        },
        _value);
    _options.addToRule(rule, constraint, "equals"_el);
}

}
