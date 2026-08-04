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

void RulesDefinitionValidator::testKeyDefinitionPlacement(const RulePtr &rule) {
    if (!rule->hasKeyDefinitions()) {
        return;
    }
    if (rule->type() != vr::RuleType::Section) {
        ERBSLAND_CORE_CONF_REQUIRE_DEBUG(!rule->keyDefinitions().empty(), "key definitions must not be empty"_el);
        throwValidationError(
            "ConfKey definitions may only be placed in a section or the document root"_el,
            rule->keyDefinitions().front()->location());
    }
    // test if the path of each key points to a rule with a section list.
    std::unordered_set<Name> seenIndexNames;
    for (const auto &keyDefinition : rule->keyDefinitions()) {
        try {
            NamePath firstListPath;
            RulePtr firstListRule;
            if (!keyDefinition->name().empty()) {
                if (seenIndexNames.contains(keyDefinition->name())) {
                    throwValidationError("All 'vr_key' definition in the same section must have an unique name"_el);
                }
                seenIndexNames.insert(keyDefinition->name());
            }
            for (const auto &key : keyDefinition->keys()) {
                const auto entryIndex = key.find(Name::vrName(Name::VR::ReservedEntry));
                if (entryIndex == NamePath::npos) {
                    throwValidationError(
                        text::StringFormat{"Keys must point to values inside a section list. "
                                           "The 'vr_entry' is missing in the key path '{}'"_el}
                            .build(key.toText()));
                }
                const auto newListPath = key.subPath(0, entryIndex);
                if (newListPath.empty()) {
                    throwValidationError(
                        text::StringFormat{"The key '{}' does not point to a section list. "
                                           "No list named in from of the 'vr_entry'"_el}
                            .build(key.toText()));
                }
                if (firstListPath.empty()) {
                    firstListPath = newListPath;
                    firstListRule = rule->child(firstListPath);
                    if (firstListRule == nullptr || firstListRule->type() != vr::RuleType::SectionList) {
                        throwValidationError(
                            text::StringFormat{"The initial path '{}' in a key does not point to a section list"_el}
                                .build(firstListPath.toText()));
                    }
                } else if (firstListPath != newListPath) {
                    throwValidationError(
                        text::StringFormat{
                            "All keys in a `vr_key` definition must point to the same section list. "
                            "The key '{}' points to a different list as previous keys in the same definition"_el}
                            .build(key.toText()));
                }
                const auto valuePath = key.subPath(entryIndex + 1);
                if (valuePath.empty()) {
                    throwValidationError(
                        text::StringFormat{"The key '{}' has no value path after 'vr_entry'"_el}.build(key.toText()));
                }
                if (valuePath.find(Name::vrName(Name::VR::ReservedEntry)) != NamePath::npos) {
                    throwValidationError(
                        text::StringFormat{"The key '{}' points to a value in a nested section list"_el}.build(
                            key.toText()));
                }
                const auto entryRule = firstListRule->child(Name::vrName(Name::VR::ReservedEntry));
                if (entryRule == nullptr || entryRule->type() != vr::RuleType::Section) {
                    throwValidationError(
                        text::StringFormat{
                            "The 'vr_entry' in the key path '{}' does not point to a section in a section list"_el}
                            .build(key.toText()));
                }
                const auto valueRule = entryRule->child(valuePath);
                if (valueRule == nullptr) {
                    throwValidationError(
                        text::StringFormat{"The value path '{}' in the key '{}' does not point to a validated value"_el}
                            .build(valuePath.toText(), key.toText()));
                }
                if (valueRule->type() == vr::RuleType::Alternatives) {
                    bool hasIntegerOrText = false;
                    for (const auto &child : valueRule->childrenImpl()) {
                        if (child->type() == vr::RuleType::Integer || child->type() == vr::RuleType::Text) {
                            hasIntegerOrText = true;
                            break;
                        }
                    }
                    if (!hasIntegerOrText) {
                        throwValidationError(
                            text::StringFormat{
                                "The value path '{}' in the key '{}' points to a value with alternatives, "
                                "but none of the alternatives contain a text or integer value"_el}
                                .build(valuePath.toText(), key.toText()));
                    }
                } else if (valueRule->type() != vr::RuleType::Text && valueRule->type() != vr::RuleType::Integer) {
                    throwValidationError(
                        text::StringFormat{
                            "The value path '{}' in the key '{}' does not point to a text or integer value"_el}
                            .build(valuePath.toText(), key.toText()));
                }
            }
        } catch (const ConfError &error) {
            throw error.withLocation(keyDefinition->location());
        }
    }
}

void RulesDefinitionValidator::validateKeyReference(const RulePtr &rule, const NamePath &keyReference) {
    if (keyReference.empty()) {
        throwValidationError("A key reference cannot be empty"_el);
    }
    if (!keyReference.at(0).isRegular()) {
        throwValidationError("A key reference must start with a regular name"_el);
    }
    if (keyReference.size() >= 2) {
        if (!keyReference.at(1).isIndex()) {
            throwValidationError("Only an index is allowed after the name of the key reference"_el);
        }
        if (keyReference.at(1).asIndex() > 9) {
            throwValidationError("The key index must be between 0 and 9"_el);
        }
    }
    if (keyReference.size() > 2) {
        throwValidationError("Unexpected name path elements after the key reference"_el);
    }
    // search for the key definition.
    auto ruleInPath = rule->parent();
    KeyDefinitionPtr foundKeyDefinition = {};
    while (foundKeyDefinition == nullptr && ruleInPath != nullptr) {
        if (ruleInPath->hasKeyDefinitions()) {
            for (const auto &keyDefinition : ruleInPath->keyDefinitions()) {
                if (keyDefinition->name() == keyReference.at(0)) {
                    foundKeyDefinition = keyDefinition;
                    break;
                }
            }
            if (foundKeyDefinition != nullptr) {
                break;
            }
        }
        ruleInPath = ruleInPath->parent();
    }
    if (foundKeyDefinition == nullptr) {
        throwValidationError(
            text::StringFormat{
                "The 'vr_key' definition for the reference '{}' was not found in the scope of the constraint"_el}
                .build(keyReference.toText()));
    }
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(ruleInPath != nullptr, "The root rule must not be null"_el);
    constexpr std::size_t allKeys = std::numeric_limits<std::size_t>::max();
    auto index = allKeys;
    if (keyReference.size() > 1) {
        index = keyReference.at(1).asIndex();
        if (index >= foundKeyDefinition->keys().size()) {
            throwValidationError(
                text::StringFormat{"The key index in the key reference '{}' is out of bounds"_el}.build(
                    keyReference.toText()));
        }
    }
    if (foundKeyDefinition->keys().size() > 1) {
        if (index == allKeys) {
            if (rule->type() == vr::RuleType::Text) {
                return; // success
            }
            throwValidationError("A key referencing a multi-key index as a whole must be of type 'text'"_el);
        }
    } else {
        index = 0;
    }
    const auto keyTypes = resolveKeyDefinitionType(ruleInPath, foundKeyDefinition, index);
    if (std::ranges::find(keyTypes, rule->type()) == keyTypes.end()) {
        throwValidationError(
            text::StringFormat{"A key referencing {} index must be of of the same type"_el}.build(
                expectedRuleTypesText(keyTypes)));
    }
    // success
}

void RulesDefinitionValidator::testKeyReferences(const RulePtr &rule) {
    if (!rule->hasConstraint(vr::ConstraintType::ConfKey)) {
        return;
    }
    if (rule->type() != vr::RuleType::Text && rule->type() != vr::RuleType::Integer) {
        throwValidationError("ConfKey references can only be used on text or integer values"_el);
    }
    auto constraint = std::dynamic_pointer_cast<KeyConstraint>(rule->constraint(vr::ConstraintType::ConfKey));
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(constraint != nullptr, "ConfKey constraint must not be null"_el);
    try {
        std::unordered_set<NamePath> seenKeyPaths;
        for (const auto &keyReference : constraint->getKeyReferences()) {
            if (seenKeyPaths.contains(keyReference)) {
                throwValidationError("Each key reference must be unique"_el);
            }
            seenKeyPaths.insert(keyReference);
            validateKeyReference(rule, keyReference);
        }
    } catch (const ConfError &error) {
        throw error.withLocation(constraint->location());
    }
}

auto RulesDefinitionValidator::resolveKeyDefinitionType(
    const RulePtr &rule, const KeyDefinitionPtr &keyDefinition, const std::size_t index) -> std::vector<vr::RuleType> {

    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(index < keyDefinition->keys().size(), "Partial key index out of bounds"_el);
    const auto targetRule = rule->child(keyDefinition->keys().at(index));
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(targetRule != nullptr, "A given key definition does not exist"_el);
    std::vector<vr::RuleType> result;
    if (targetRule->type() == vr::RuleType::Alternatives) {
        for (const auto &alternative : targetRule->childrenImpl()) {
            if (alternative->type() == vr::RuleType::Text || alternative->type() == vr::RuleType::Integer) {
                if (std::ranges::find(result, alternative->type()) == result.end()) {
                    result.push_back(alternative->type());
                }
            }
            ERBSLAND_CORE_CONF_REQUIRE_DEBUG(!result.empty(), "Unexpected alternative without matching types."_el);
        }
        return result;
    }
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(
        targetRule->type() == vr::RuleType::Text || targetRule->type() == vr::RuleType::Integer,
        "Unexpected rule type"_el);
    result.emplace_back(targetRule->type());
    return result;
}

void RulesDefinitionValidator::testDependencyDefinition(const RulePtr &rule) {
    if (!rule->hasDependencyDefinitions()) {
        return;
    }
    if (rule->type() != vr::RuleType::Section) {
        throwValidationError("Dependency definitions can only be placed in node-rules definition of a section"_el);
    }
    std::unordered_set<NamePath> seenDependencyPaths;
    for (const auto &dependencyDefinition : rule->dependencyDefinitions()) {
        try {
            for (const auto &sourcePath : dependencyDefinition->sources()) {
                if (seenDependencyPaths.contains(sourcePath)) {
                    throwValidationError("Each dependency in 'source' and 'target' path must be unique"_el);
                }
                seenDependencyPaths.insert(sourcePath);
                validateDependencyPath(rule, sourcePath);
            }
            for (const auto &targetPath : dependencyDefinition->targets()) {
                if (seenDependencyPaths.contains(targetPath)) {
                    throwValidationError("Each dependency in 'source' and 'target' path must be unique"_el);
                }
                seenDependencyPaths.insert(targetPath);
                validateDependencyPath(rule, targetPath);
            }
        } catch (const ConfError &error) {
            if (error.location().isUndefined()) {
                throw error.withLocation(dependencyDefinition->location());
            }
            throw;
        }
    }
}

void RulesDefinitionValidator::validateDependencyPath(const RulePtr &rule, const NamePath &dependencyPath) {
    if (dependencyPath.containsIndex() || dependencyPath.containsText()) {
        throwValidationError("The dependency path cannot contain an index or text"_el);
    }
    if (dependencyPath.find(Name::vrName(Name::VR::ReservedEntry)) != NamePath::npos) {
        throwValidationError(
            text::StringFormat{"The dependency path '{}' points to a value in a section list"_el}.build(
                dependencyPath.toText()));
    }
    auto targetRule = rule->child(dependencyPath);
    if (targetRule == nullptr) {
        throwValidationError(
            text::StringFormat{"The dependency path '{}' does not point to a validated value"_el}.build(
                dependencyPath.toText()));
    }
    // Test all rules in the branch if they are optional
    auto testedPath = dependencyPath;
    auto testedRule = targetRule;
    while (true) {
        if (isRuleOptional(testedRule)) {
            return; // ok
        }
        if (testedPath.size() == 1) {
            break;
        }
        testedPath = testedPath.parent();
        testedRule = rule->child(testedPath);
    }
    throwValidationError(
        text::StringFormat{
            "The dependency path '{}' points to a value that is neither optional nor has a default value"_el}
            .build(dependencyPath.toText()));
}

auto RulesDefinitionValidator::isRuleOptional(const RulePtr &rule) -> bool {
    if (rule->type() == vr::RuleType::Alternatives) {
        bool isOptional = false;
        for (const auto &alternativeRule : rule->childrenImpl()) {
            if (alternativeRule->isOptional() || alternativeRule->hasDefault()) {
                isOptional = true;
                break;
            }
        }
        return isOptional;
    }
    return rule->isOptional() || rule->hasDefault();
}

}
