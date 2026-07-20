// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DocumentValidator.hpp"

#include "ValidationContext.hpp"
#include "ValidationError.hpp"

#include "../utilities/InternalError.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void DocumentValidator::validateNameConstraints(const RulePtr &rule, const ValuePtr &value) {
    if (!rule->hasNameConstraints()) {
        return; // skip if this rule has no name constraints.
    }
    const auto name = value->name();
    if (value->name().type() == NameType::Index || value->name().type() == NameType::TextIndex) {
        throwValidationError(
            "Expected a named value, but got a list entry or text index"_el, value->namePath(), value->location());
    }
    const auto nameRule = rule->nameConstraints();
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(nameRule != nullptr, "Unexpected missing name rule");
    const auto validationContext = ValidationContext{
        .target = ValidationTarget::Name,
        .value = value,
        .rule = nameRule,
    };
    validateConstraints(nameRule, validationContext);
}

void DocumentValidator::validateValueConstraints(const RulePtr &rule, const ValuePtr &value) {
    const auto validationContext = ValidationContext{
        .target = ValidationTarget::Value,
        .value = value,
        .rule = rule,
    };
    validateConstraints(rule, validationContext);
}

void DocumentValidator::validateConstraints(const RulePtr &rule, const ValidationContext &validationContext) {
    for (const auto &constraint : rule->constraintsImpl()) {
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(
            constraint->type() != vr::ConstraintType::Undefined, "Unexpected constraint type");
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(
            constraint->type() != vr::ConstraintType::ConfVersion, "Unexpected constraint type");
        if (constraint->type() == vr::ConstraintType::ConfKey) {
            continue; // ignore key constraints for now.
        }
        try {
            constraint->validate(validationContext);
        } catch (const ConfError &error) {
            if (error.category() == ConfErrorCategory::Validation) {
                if (constraint->hasCustomError()) {
                    throw error.withDescription(constraint->customError());
                }
                if (rule->hasCustomError()) {
                    throw error.withDescription(rule->customError());
                }
            }
            throw;
        }
    }
}

auto DocumentValidator::expectedValueTypeText(const RulePtr &rule) const -> text::String {
    return impl::expectedValueTypeText(rule, _version);
}

auto DocumentValidator::parentLocationText(const conf::ValuePtr &value) -> text::String {
    if (value == nullptr || value->isDocument()) {
        // Accept value == nullptr, when no parent indicates a note at the document root.
        return "the document root"_el;
    }
    if (value->type() == ValueType::SectionWithNames || value->type() == ValueType::IntermediateSection) {
        return text::StringFormat{"the section '{}'"_el}.build(value->namePath().toText());
    }
    if (value->type() == ValueType::SectionWithTexts) {
        return text::StringFormat{"the section with texts '{}'"_el}.build(value->namePath().toText());
    }
    return {};
}

void DocumentValidator::throwExpectedVsActual(const RulePtr &rule, const ValuePtr &value) const {
    impl::throwExpectedVsActual(rule, value, _version);
}

auto DocumentValidator::errorNamePathsOr(const NamePathList &paths, const bool forNegation) -> text::String {

    text::StringEditor result;
    if (paths.size() > 1) {
        if (forNegation) {
            result.append("configure none of "_el);
        } else {
            result.append("at least one of "_el);
        }
    } else {
        if (forNegation) {
            result.append("not configure "_el);
        }
    }
    result.append("'"_el);
    for (std::size_t i = 0; i != paths.size(); ++i) {
        result.append(paths[i].toText());
        if (i < paths.size() - 1) {
            if (i == paths.size() - 2) {
                result.append("', or '"_el);
            } else {
                result.append("', '"_el);
            }
        }
    }
    result.append("'"_el);
    return result;
}

}
