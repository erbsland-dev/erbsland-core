// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "In.hpp"

#include "../../../../text/StringFormat.hpp"
#include "../../../impl/vr/InConstraint.hpp"

#include <ranges>
#include <type_traits>

namespace erbsland::conf::vr::builder {

void In::operator()(impl::Rule &rule) {
    auto constraint = std::visit(
        [&rule](const auto &values) -> impl::ConstraintPtr {
            using T = std::decay_t<decltype(values)>;
            if (std::ranges::empty(values)) {
                throwValidationError(
                    text::StringFormat{"The 'in' constraint must specify a single {} value or a list of {} values"_el}
                        .build(rule.type().toText(), rule.type().toText()));
            }
            if constexpr (std::is_same_v<T, std::vector<Integer>>) {
                requireRuleTypeForConstraint(rule, "in"_el, {vr::RuleType::Integer});
                if (impl::InIntegerConstraint::hasDuplicate(values, rule.caseSensitivity())) {
                    throwValidationError("The 'in' list must not contain duplicate values"_el);
                }
                return std::make_shared<impl::InIntegerConstraint>(values);
            } else if constexpr (std::is_same_v<T, std::vector<Float>>) {
                requireRuleTypeForConstraint(rule, "in"_el, {vr::RuleType::Float});
                if (impl::InFloatConstraint::hasDuplicate(values, rule.caseSensitivity())) {
                    throwValidationError("The 'in' list must not contain duplicate values"_el);
                }
                return std::make_shared<impl::InFloatConstraint>(values);
            } else if constexpr (std::is_same_v<T, text::StringList>) {
                requireRuleTypeForConstraint(rule, "in"_el, {vr::RuleType::Text});
                if (impl::InTextConstraint::hasDuplicate(values, rule.caseSensitivity())) {
                    throwValidationError("The 'in' list must not contain duplicate values"_el);
                }
                return std::make_shared<impl::InTextConstraint>(values);
            } else {
                requireRuleTypeForConstraint(rule, "in"_el, {vr::RuleType::Bytes});
                if (impl::InBytesConstraint::hasDuplicate(values, rule.caseSensitivity())) {
                    throwValidationError("The 'in' list must not contain duplicate values"_el);
                }
                return std::make_shared<impl::InBytesConstraint>(values);
            }
        },
        _values);
    _options.addToRule(rule, constraint, "in"_el);
}

}
