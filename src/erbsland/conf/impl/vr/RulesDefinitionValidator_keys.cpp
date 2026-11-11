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

using namespace text;
using namespace text::literals;

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
            NamePathList entryValuePaths;
            if (!keyDefinition->name().empty()) {
                if (seenIndexNames.contains(keyDefinition->name())) {
                    throwValidationError("All 'vr_key' definition in the same section must have an unique name"_el);
                }
                seenIndexNames.insert(keyDefinition->name());
            }
            for (const auto &key : keyDefinition->keys()) {
                const auto valueRule = rule->descendantByValuePath(key);
                if (valueRule == nullptr) {
                    throwValidationError(
                        StringFormat{"The key path '{}' does not point to a validated value"_el}.build(key.toText()));
                }
                RulePtr entryRule;
                for (auto ancestor = valueRule; ancestor != rule; ancestor = ancestor->parent()) {
                    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(ancestor != nullptr, "Key must remain within its scope"_el);
                    if (ancestor->role() != RuleRole::ListEntry) {
                        continue;
                    }
                    if (entryRule != nullptr) {
                        throwValidationError(
                            StringFormat{"The key '{}' points to a value in a nested section list"_el}.build(
                                key.toText()));
                    }
                    entryRule = ancestor;
                }
                if (entryRule == nullptr) {
                    throwValidationError(
                        StringFormat{"Keys must point to values inside a section list. "
                                     "The 'vr_entry' is missing in the key path '{}'"_el}
                            .build(key.toText()));
                }
                const auto listRule = entryRule->parent();
                if (listRule->type() != vr::RuleType::SectionList || entryRule->type() != vr::RuleType::Section) {
                    throwValidationError(
                        StringFormat{
                            "The 'vr_entry' in the key path '{}' does not point to a section in a section list"_el}
                            .build(key.toText()));
                }
                const auto newListPath = listRule->valueNamePath().subPath(rule->valueNamePath().size());
                if (firstListPath.empty()) {
                    firstListPath = newListPath;
                } else if (firstListPath != newListPath) {
                    throwValidationError(
                        StringFormat{
                            "All keys in a 'vr_key' definition must point to the same section list. "
                            "The key '{}' points to a different list as previous keys in the same definition"_el}
                            .build(key.toText()));
                }
                const auto valuePath = valueRule->valueNamePath().subPath(entryRule->valueNamePath().size());
                if (valuePath.empty()) {
                    throwValidationError(
                        StringFormat{"The key '{}' has no value path after 'vr_entry'"_el}.build(key.toText()));
                }
                entryValuePaths.push_back(valuePath);
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
                            StringFormat{"The value path '{}' in the key '{}' points to a value with alternatives, "
                                         "but none of the alternatives contain a text or integer value"_el}
                                .build(valuePath.toText(), key.toText()));
                    }
                } else if (valueRule->type() != vr::RuleType::Text && valueRule->type() != vr::RuleType::Integer) {
                    throwValidationError(
                        StringFormat{"The value path '{}' in the key '{}' does not point to a text or integer value"_el}
                            .build(valuePath.toText(), key.toText()));
                }
            }
            keyDefinition->setResolvedPaths(std::move(firstListPath), std::move(entryValuePaths));
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
            StringFormat{
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
                StringFormat{"The key index in the key reference '{}' is out of bounds"_el}.build(
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
            StringFormat{"A key referencing {} index must be of of the same type"_el}.build(
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
    const auto targetRule = rule->descendantByValuePath(keyDefinition->keys().at(index));
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(targetRule != nullptr, "A given key definition does not exist"_el);
    std::vector<vr::RuleType> result;
    if (targetRule->type() == vr::RuleType::Alternatives) {
        for (const auto &alternative : targetRule->childrenImpl()) {
            if (alternative->type() == vr::RuleType::Text || alternative->type() == vr::RuleType::Integer) {
                if (std::ranges::find(result, alternative->type()) == result.end()) {
                    result.push_back(alternative->type());
                }
            }
        }
        ERBSLAND_CORE_CONF_REQUIRE_DEBUG(!result.empty(), "Unexpected alternative without matching types."_el);
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
    const auto targetRule = rule->descendantByValuePath(dependencyPath);
    if (targetRule == nullptr) {
        throwValidationError(
            StringFormat{"The dependency path '{}' does not point to a validated value"_el}.build(
                dependencyPath.toText()));
    }
    bool hasOptionalAncestor = false;
    for (auto ancestor = targetRule; ancestor != rule; ancestor = ancestor->parent()) {
        ERBSLAND_CORE_CONF_REQUIRE_DEBUG(ancestor != nullptr, "Dependency must remain within its scope"_el);
        if (ancestor->role() == RuleRole::ListEntry) {
            throwValidationError(
                StringFormat{"The dependency path '{}' points to a value in a section list"_el}.build(
                    dependencyPath.toText()));
        }
        hasOptionalAncestor = hasOptionalAncestor || isRuleOptional(ancestor);
    }
    if (hasOptionalAncestor) {
        return;
    }
    throwValidationError(
        StringFormat{"The dependency path '{}' points to a value that is neither optional nor has a default value"_el}
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
