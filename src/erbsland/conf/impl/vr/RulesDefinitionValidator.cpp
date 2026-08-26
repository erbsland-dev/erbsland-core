// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RulesDefinitionValidator.hpp"

#include "KeyConstraint.hpp"
#include "MinMaxConstraint.hpp"
#include "MinMaxDateConstraint.hpp"
#include "MinMaxDateTimeConstraint.hpp"
#include "MinMaxFloatConstraint.hpp"
#include "MinMaxIntegerConstraint.hpp"
#include "MinMaxMatrixConstraint.hpp"
#include "ValidationError.hpp"

#include "../utilities/InternalError.hpp"
#include "../value/Value.hpp"

#include "../../../text/StringFormat.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <unordered_set>

namespace erbsland::conf::impl {

using namespace text::literals;

RulesDefinitionValidator::RulesDefinitionValidator(RulePtr root) : _root{std::move(root)} {
}

void RulesDefinitionValidator::validate() {
    std::vector<RulePtr> stack{_root};
    while (!stack.empty()) {
        auto rule = stack.back();
        stack.pop_back();
        validateRule(rule);
        stack.insert(stack.end(), rule->childrenImpl().begin(), rule->childrenImpl().end());
    }
}

void RulesDefinitionValidator::validateRule(const RulePtr &rule) {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(rule != nullptr, "rule must not be null"_el);
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(rule->type() != vr::RuleType::Undefined, "Unexpected undefined rule type"_el);

    try {
        constexpr std::array testFunctions{
            &RulesDefinitionValidator::testVrNameMustBeText,
            &RulesDefinitionValidator::testSectionList,
            &RulesDefinitionValidator::testAlternatives,
            &RulesDefinitionValidator::testVrAny,
            &RulesDefinitionValidator::testValueList,
            &RulesDefinitionValidator::testDefaultsAndOptionality,
            &RulesDefinitionValidator::testSecretMarkerType,
            &RulesDefinitionValidator::testMinimumMaximumRelation,
            &RulesDefinitionValidator::testKeyDefinitionPlacement,
            &RulesDefinitionValidator::testKeyReferences,
            &RulesDefinitionValidator::testDependencyDefinition,
        };

        for (const auto &testFunction : testFunctions) {
            testFunction(rule);
        }
    } catch (const ConfError &error) {
        if (error.location().isUndefined()) {
            throw error.withNamePathAndLocation(rule->namePath(), rule->location());
        }
        throw;
    }
}

void RulesDefinitionValidator::testAlternatives(const RulePtr &rule) {
    if (rule->type() != vr::RuleType::Alternatives) {
        return;
    }
    // 1. alternatives must not contain other alternatives.
    // 2. only one default value.
    // 3. Optional must only be specified once.
    std::size_t index = 0;
    bool hasDefault = false;
    for (const auto &child : rule->childrenImpl()) {
        if (child->type() == vr::RuleType::Alternatives) {
            throwValidationError("Alternatives may not contain other alternatives"_el);
        }
        if (child->hasDefault()) {
            if (hasDefault) {
                throwValidationError("Only one alternative may have a default value"_el);
            }
            hasDefault = true;
        }
        if (child->isOptional()) {
            if (index > 0) {
                throwValidationError("Only the first alternative may be marked as optional"_el);
            }
        }
        index += 1;
    }
}

void RulesDefinitionValidator::testVrAny(const RulePtr &rule) {
    if (rule->ruleName() != Name::vrName(Name::VR::ReservedAny)) {
        return;
    }
    if (rule->isOptional()) {
        throwValidationError("The 'vr_any' rule cannot be set optional, it is optional by definition"_el);
    }
    if (rule->hasDefault()) {
        throwValidationError("The 'vr_any' rule cannot have a default value"_el);
    }
}

void RulesDefinitionValidator::testVrNameMustBeText(const RulePtr &rule) {
    if (rule->ruleName() == Name::vrName(Name::VR::ReservedName)) {
        if (rule->type() != vr::RuleType::Text) {
            throwValidationError("The name rule must have a type of 'text'"_el);
        }
    }
}

void RulesDefinitionValidator::testSectionList(const RulePtr &rule) {
    if (rule->type() != vr::RuleType::SectionList) {
        return;
    }
    if (!rule->hasChild(Name::vrName(Name::VR::ReservedEntry))) {
        throwValidationError("A section list rule must have a 'vr_entry' node-rules definition"_el);
    }
    // also validate its type.
    const auto entryRule = rule->child(Name::vrName(Name::VR::ReservedEntry));
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(entryRule != nullptr, "vr_entry must not be null"_el);
    try {
        if (entryRule->type() == vr::RuleType::Alternatives) {
            // if vr_entry is an alternatives rule, validate its children.
            for (const auto &child : entryRule->childrenImpl()) {
                try {
                    if (child->type() != vr::RuleType::Section && child->type() != vr::RuleType::SectionWithTexts) {
                        throwValidationError(
                            "All alternatives in a 'vr_entry' node-rules definition for a section list "_el
                            "must be of type 'section' or 'section_with_texts'"_el);
                    }
                    testVrEntryCommonConstraints(child);
                } catch (const ConfError &error) {
                    throw error.withNamePathAndLocation(child->namePath(), child->location());
                }
            }
        } else if (entryRule->type() != vr::RuleType::Section && entryRule->type() != vr::RuleType::SectionWithTexts) {
            throwValidationError(
                "The 'vr_entry' node-rules definition for a section list "_el
                "must be of type 'section' or 'section_with_texts'"_el);
        } else {
            testVrEntryCommonConstraints(entryRule);
        }
    } catch (const ConfError &error) {
        if (error.location().isUndefined()) {
            throw error.withNamePathAndLocation(entryRule->namePath(), entryRule->location());
        }
        throw;
    }
    testNoOtherSubsectionInListDefinitions(rule);
}

void RulesDefinitionValidator::testValueList(const RulePtr &rule) {
    if (rule->type() != vr::RuleType::ValueList && rule->type() != vr::RuleType::ValueMatrix) {
        return;
    }
    if (!rule->hasChild(Name::vrName(Name::VR::ReservedEntry))) {
        throwValidationError("A value list or matrix rule must have a 'vr_entry' node-rules definition"_el);
    }
    const auto entryRule = rule->child(Name::vrName(Name::VR::ReservedEntry));
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(entryRule != nullptr, "vr_entry must not be null"_el);
    try {
        if (entryRule->type() == vr::RuleType::Alternatives) {
            // if vr_entry is an alternatives rule, validate its children.
            for (const auto &child : entryRule->childrenImpl()) {
                try {
                    if (!child->type().isScalar()) {
                        throwValidationError(
                            "All alternatives in a 'vr_entry' node-rules definition for a value list "_el
                            "must be scalar types"_el);
                    }
                    testVrEntryCommonConstraints(child);
                } catch (const ConfError &error) {
                    throw error.withNamePathAndLocation(child->namePath(), child->location());
                }
            }
        } else if (!entryRule->type().isScalar()) {
            throwValidationError(
                text::StringFormat{"Unexpected 'vr_entry' node-rules definition type for a value list. "
                                   "Expected a scalar value type, but got {} type"_el}
                    .build(entryRule->type().expectedValueTypeText()));
        } else {
            testVrEntryCommonConstraints(entryRule);
        }
    } catch (const ConfError &error) {
        if (error.location().isUndefined()) {
            throw error.withNamePathAndLocation(entryRule->namePath(), entryRule->location());
        }
        throw;
    }
    testNoOtherSubsectionInListDefinitions(rule);
}

void RulesDefinitionValidator::testVrEntryCommonConstraints(const RulePtr &rule) {
    if (rule->hasDefault()) {
        throwValidationError("The `vr_entry` node-rules definition may not have a default value"_el);
    }
    if (rule->isOptional()) {
        throwValidationError("The `vr_entry` node-rules definition cannot be optional"_el);
    }
}

void RulesDefinitionValidator::testNoOtherSubsectionInListDefinitions(const RulePtr &rule) {
    for (const auto &child : rule->childrenImpl()) {
        if (child->ruleName() != Name::vrName(Name::VR::ReservedEntry)) {
            throwValidationError(
                text::StringFormat{"Unexpected sub-node-rules definition in '{}' rule: only 'vr_entry' is permitted"_el}
                    .build(rule->type().toText()),
                child->namePath(),
                child->location());
        }
    }
}

void RulesDefinitionValidator::testDefaultsAndOptionality(const RulePtr &rule) {
    if (rule->hasDefault() && rule->isOptional()) {
        throwValidationError("A node-rules definition may not be both optional and have a default value"_el);
    }
    if (rule->hasDefault()) {
        if (!rule->type().matchesValueType(rule->defaultValue()->type())) {
            throwValidationError(
                text::StringFormat{
                    "The default value of a node-rules definition must match its type. Expected {}, but got {}"_el}
                    .build(
                        rule->type().expectedValueTypeText(), rule->defaultValue()->type().toValueDescription(true)));
        }
    }
}

void RulesDefinitionValidator::testSecretMarkerType(const RulePtr &rule) {
    if (!rule->isSecret()) {
        return;
    }
    if (!rule->type().isScalar()) {
        throwValidationError(
            text::StringFormat{"The 'is_secret' marker can only be used for scalar value types. Found {} type"_el}
                .build(rule->type().toText()));
    }
}

void RulesDefinitionValidator::testMinimumMaximumRelation(const RulePtr &rule) {
    if (!rule->hasConstraint(vr::ConstraintType::Minimum) || !rule->hasConstraint(vr::ConstraintType::Maximum)) {
        return;
    }
    const auto minimum = std::dynamic_pointer_cast<MinMaxConstraint>(rule->constraint(vr::ConstraintType::Minimum));
    const auto maximum = std::dynamic_pointer_cast<MinMaxConstraint>(rule->constraint(vr::ConstraintType::Maximum));
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(minimum != nullptr, "minimum constraint must be min/max constraint"_el);
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(maximum != nullptr, "maximum constraint must be min/max constraint"_el);
    if (minimum->isNegated() || maximum->isNegated()) {
        return;
    }
    const auto throwInvalidRange = []() {
        throwValidationError("The 'minimum' constraint value must be less than or equal to the 'maximum' value"_el);
    };
    switch (rule->type().raw()) {
    case vr::RuleType::Integer:
    case vr::RuleType::Text:
    case vr::RuleType::Bytes:
    case vr::RuleType::ValueList:
    case vr::RuleType::Section:
    case vr::RuleType::SectionList:
    case vr::RuleType::SectionWithTexts: {
        const auto minInt = std::dynamic_pointer_cast<MinMaxIntegerConstraint>(minimum);
        const auto maxInt = std::dynamic_pointer_cast<MinMaxIntegerConstraint>(maximum);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(minInt != nullptr, "minimum integer constraint type mismatch"_el);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(maxInt != nullptr, "maximum integer constraint type mismatch"_el);
        if (minInt->value() > maxInt->value()) {
            throwInvalidRange();
        }
        return;
    }
    case vr::RuleType::Float: {
        const auto minFloat = std::dynamic_pointer_cast<MinMaxFloatConstraint>(minimum);
        const auto maxFloat = std::dynamic_pointer_cast<MinMaxFloatConstraint>(maximum);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(minFloat != nullptr, "minimum float constraint type mismatch"_el);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(maxFloat != nullptr, "maximum float constraint type mismatch"_el);
        if (minFloat->value() > maxFloat->value()) {
            throwInvalidRange();
        }
        return;
    }
    case vr::RuleType::Date: {
        const auto minDate = std::dynamic_pointer_cast<MinMaxDateConstraint>(minimum);
        const auto maxDate = std::dynamic_pointer_cast<MinMaxDateConstraint>(maximum);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(minDate != nullptr, "minimum date constraint type mismatch"_el);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(maxDate != nullptr, "maximum date constraint type mismatch"_el);
        if (minDate->value() > maxDate->value()) {
            throwInvalidRange();
        }
        return;
    }
    case vr::RuleType::DateTime: {
        const auto minDateTime = std::dynamic_pointer_cast<MinMaxDateTimeConstraint>(minimum);
        const auto maxDateTime = std::dynamic_pointer_cast<MinMaxDateTimeConstraint>(maximum);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(minDateTime != nullptr, "minimum date-time constraint type mismatch"_el);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(maxDateTime != nullptr, "maximum date-time constraint type mismatch"_el);
        if (minDateTime->value() > maxDateTime->value()) {
            throwInvalidRange();
        }
        return;
    }
    case vr::RuleType::ValueMatrix: {
        const auto minMatrix = std::dynamic_pointer_cast<MinMaxMatrixConstraint>(minimum);
        const auto maxMatrix = std::dynamic_pointer_cast<MinMaxMatrixConstraint>(maximum);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(minMatrix != nullptr, "minimum matrix constraint type mismatch"_el);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(maxMatrix != nullptr, "maximum matrix constraint type mismatch"_el);
        if (minMatrix->value() > maxMatrix->value() || minMatrix->secondValue() > maxMatrix->secondValue()) {
            throwInvalidRange();
        }
        return;
    }
    default:
        return;
    }
}

}
