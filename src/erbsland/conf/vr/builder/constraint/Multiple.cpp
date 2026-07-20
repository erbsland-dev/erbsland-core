// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Multiple.hpp"

#include "../../../impl/vr/MultipleConstraint.hpp"

#include <cmath>
#include <limits>
#include <type_traits>

namespace erbsland::conf::vr::builder {

void Multiple::operator()(impl::Rule &rule) {
    auto constraint = std::visit(
        [&rule](const auto &value) -> impl::ConstraintPtr {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, Integer>) {
                requireRuleTypeForConstraint(
                    rule,
                    "multiple"_el,
                    {vr::RuleType::Integer,
                        vr::RuleType::Text,
                        vr::RuleType::Bytes,
                        vr::RuleType::ValueList,
                        vr::RuleType::Section,
                        vr::RuleType::SectionList,
                        vr::RuleType::SectionWithTexts});
                if (value == 0) {
                    throwValidationError("The 'multiple' divisor must not be zero"_el);
                }
                return std::make_shared<impl::MultipleIntegerConstraint>(value);
            } else if constexpr (std::is_same_v<T, Float>) {
                requireRuleTypeForConstraint(rule, "multiple"_el, {vr::RuleType::Float});
                if (std::abs(value) <= std::numeric_limits<Float>::epsilon()) {
                    throwValidationError("The 'multiple' divisor must not be zero"_el);
                }
                return std::make_shared<impl::MultipleFloatConstraint>(value);
            } else {
                requireRuleTypeForConstraint(rule, "multiple"_el, {vr::RuleType::ValueMatrix});
                if (value.first == 0 || value.second == 0) {
                    throwValidationError("The 'multiple' divisors must not be zero"_el);
                }
                return std::make_shared<impl::MultipleMatrixConstraint>(value.first, value.second);
            }
        },
        _value);
    _options.addToRule(rule, constraint, "multiple"_el);
}

}
