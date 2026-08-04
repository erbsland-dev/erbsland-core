// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DocumentValidator.hpp"

#include "KeyConstraint.hpp"
#include "Pass2Frame.hpp"
#include "ValidationError.hpp"

#include "../utilities/InternalError.hpp"
#include "../value/ValueHelper.hpp"

#include "../../../text/StringFormat.hpp"

#include <ranges>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

namespace erbsland::conf::impl {

using namespace text::literals;

DocumentValidator::DocumentValidator(RulePtr root, conf::ValuePtr value, const Integer version) :
    _root{std::move(root)}, _value{std::move(value)}, _version{version} {

    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(_root != nullptr, "The root rule must not be null"_el);
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(_value != nullptr, "The value must not be null"_el);
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(_root->type() == vr::RuleType::Section, "The root rule must be a section"_el);
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(
        _value->isDocument() || _value->isSectionWithNames(),
        "The value must be a document or a section with names"_el);
}

void DocumentValidator::validate() {
    if (_root->empty()) {
        return;
    }

    validatePass1();
    validatePass2();
}

void DocumentValidator::validatePass1() {

    // initialize the use-indexes flag with root key definitions
    _useIndexes = _root->hasKeyDefinitions();

    std::vector<Frame> stack;
    stack.reserve(32);
    stack.emplace_back(Frame{.valueNode = _value, .ruleNode = _root});

    while (!stack.empty()) {
        auto [value, rule] = stack.back();
        stack.pop_back();
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(value != nullptr, "The value node must not be null"_el);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(rule != nullptr, "The rule node must not be null"_el);
        if (value != _value) { // do not validate the root value.
            const auto valueImpl = getImplValue(value);
            // Drop defaults from previous validations for this node before evaluating constraints and descendants.
            valueImpl->removeDefaultValues();
            rule = validate(rule, valueImpl);
            if (rule == nullptr) { // = skip this branch (not-validated or no matching alternative)
                continue;          // skip this branch
            }
            valueImpl->setValidationRule(rule);
            if (rule->type() == vr::RuleType::ValueList || rule->type() == vr::RuleType::ValueMatrix) {
                // Value list and matrix entries are already validated at this point.
                // Skipping the rest of this branch.
                continue;
            }
        } else { // for the root value, only remove defaults and assign the root rule to mark it as validated.
            callImplValueFn(value, [&rule](auto &&valueImpl) -> void {
                valueImpl->removeDefaultValues();
                valueImpl->setValidationRule(rule);
            });
        }
        // Descend into the child values:
        // Add in reverse order to preserve the original order of validation.
        std::unordered_set<RulePtr> rulesWithMatchingValues;
        for (const auto &child : std::ranges::reverse_view(*value)) {
            auto nextValue = getImplValue(child);
            auto nextRule = nextRuleForValue(rule, nextValue); // may throw
            rulesWithMatchingValues.insert(nextRule);
            stack.emplace_back(Frame{.valueNode = std::move(nextValue), .ruleNode = std::move(nextRule)});
        }
        // Now handle the rules that had no matching values.
        for (const auto &childRule : rule->childrenImpl()) {
            if (rulesWithMatchingValues.contains(childRule)) {
                continue; // ignore all rules we already matched with values.
            }
            handleMissingValues(childRule, value);
        }
    }
}

void DocumentValidator::validatePass2() {
    if (!_useIndexes && !_useDependencies) {
        return; // Skip pass 2 if we have no indexes and no dependency checks.
    }

    // For the second pass, scan the value tree and the assigned rules.
    std::vector<Pass2Frame> stack;
    stack.reserve(32);
    stack.emplace_back(Pass2Frame::createEnter(_value, _root));
    KeyIndexList keyIndexStack;

    while (!stack.empty()) {
        auto frame = stack.back();
        stack.pop_back();
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(frame.value != nullptr, "The value node must not be null"_el);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(frame.rule != nullptr, "The rule node must not be null"_el);

        if (frame.isExit) {
            if (frame.addedIndexes > 0) {
                ERBSLAND_CORE_CONF_REQUIRE_SAFETY(
                    keyIndexStack.size() >= frame.addedIndexes, "Index stack mismatch"_el);
                keyIndexStack.resize(keyIndexStack.size() - frame.addedIndexes);
            }
            continue;
        }

        if (frame.rule->hasKeyDefinitions()) {
            // Validate the definitions and add all names indexes to the index stack.
            auto keyIndexes = buildKeyIndexes(frame.value, frame.rule);
            frame.addedIndexes = keyIndexes.size();
            keyIndexStack.reserve(keyIndexStack.size() + keyIndexes.size());
            keyIndexStack.insert(
                keyIndexStack.end(),
                std::make_move_iterator(keyIndexes.begin()),
                std::make_move_iterator(keyIndexes.end()));
        }
        if (frame.rule->hasConstraint(vr::ConstraintType::ConfKey)) {
            validateKeyConstraint(keyIndexStack, frame.value, frame.rule);
        }
        if (frame.rule->hasDependencyDefinitions()) {
            validateDependencies(frame.value, frame.rule);
        }
        stack.emplace_back(frame.createExit());
        for (const auto &child : std::ranges::reverse_view(*frame.value)) {
            if (child->isDefaultValue()) {
                continue; // ignore default values applied to the value tree.
            }
            auto childRule = std::dynamic_pointer_cast<Rule>(child->validationRule());
            if (childRule != nullptr) {
                if (childRule->type() == vr::RuleType::NotValidated) {
                    continue; // ignore not validated value trees.
                }
                stack.emplace_back(Pass2Frame::createEnter(child, childRule));
            }
        }
    }
}

auto DocumentValidator::buildKeyIndexes(const conf::ValuePtr &value, const RulePtr &rule) -> KeyIndexList {
    KeyIndexList result;
    for (const auto &keyDefinition : rule->keyDefinitions()) {
        auto keyIndex = buildKeyIndexAndValidateUniqueness(value, keyDefinition);
        if (!keyIndex->name().empty()) {
            // only store named key indexes.
            result.emplace_back(std::move(keyIndex));
        }
    }
    return result;
}

auto DocumentValidator::buildKeyIndexAndValidateUniqueness(
    [[maybe_unused]] const conf::ValuePtr &value, const KeyDefinitionPtr &keyDefinition) -> KeyIndexPtr {

    NamePath listPath;
    std::vector<NamePath> valuePaths;
    for (const auto &key : keyDefinition->keys()) {
        ERBSLAND_CORE_CONF_REQUIRE_DEBUG(!key.containsIndex(), "The key must not contain an index"_el);
        ERBSLAND_CORE_CONF_REQUIRE_DEBUG(!key.containsText(), "The key must not contain text"_el);
        auto entryIndex = key.find(Name::vrName(Name::VR::ReservedEntry));
        auto newListPath = key.subPath(0, entryIndex);
        if (listPath.empty()) {
            listPath = newListPath;
        } else {
            ERBSLAND_CORE_CONF_REQUIRE_SAFETY(
                listPath == newListPath, "The list portion of key paths must be equal"_el);
        }
        auto valuePath = key.subPath(entryIndex + 1);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(!valuePath.empty(), "The value path must not be empty"_el);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(
            valuePath.find(Name::vrName(Name::VR::ReservedEntry)) == NamePath::npos,
            "A key must not point into nested lists."_el);
        valuePaths.emplace_back(std::move(valuePath));
    }

    auto keyIndex =
        std::make_shared<KeyIndex>(keyDefinition->name(), keyDefinition->caseSensitivity(), valuePaths.size());
    // first, try to get a section list or return an empty index.
    if (!value->hasValue(listPath)) {
        return keyIndex;
    }
    auto listValue = value->value(listPath);
    if (listValue->type() != ValueType::SectionList) {
        return keyIndex;
    }
    // add all values for each entry to the index and check for uniqueness.
    for (const auto &entry : *listValue) {
        text::StringList keyElements;
        bool atLeastOneValueExists = false;
        for (const auto &valuePath : valuePaths) {
            const auto entryValue = entry->value(valuePath);
            if (entryValue != nullptr &&
                (entryValue->type() == ValueType::Text || entryValue->type() == ValueType::Integer)) {
                keyElements.append(entryValue->toTextRepresentation());
                atLeastOneValueExists = true;
            } else {
                keyElements.append(text::String{});
            }
        }
        if (!atLeastOneValueExists) {
            continue; // ignore entries with non-existing values
        }
        auto success = keyIndex->tryAddKey(ConfKey{keyElements});
        if (!success) {
            if (valuePaths.size() == 1) {
                throwValidationError(
                    text::StringFormat{"The key '{}' is not unique in the list '{}'. Found a duplicate"_el}.build(
                        valuePaths.front().toText(), listValue->namePath().toText()),
                    entry->namePath(),
                    entry->location());
            }
            text::StringList keyNamePathsForError;
            for (const auto &valuePath : valuePaths) {
                keyNamePathsForError.append(valuePath.toText());
            }
            throwValidationError(
                text::StringFormat{"The combines keys '{}' are not unique in the list '{}'. Found a duplicate"_el}
                    .build(keyNamePathsForError.join("', '"_el), listValue->namePath().toText()),
                entry->namePath(),
                entry->location());
        }
    }
    return keyIndex; // At this point, no duplicates were found and the index was successfully created.
}

void DocumentValidator::validateKeyConstraint(
    const KeyIndexList &indexStack, const conf::ValuePtr &value, const RulePtr &rule) {

    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(
        value->type() == ValueType::Text || value->type() == ValueType::Integer,
        "The key constraint can only be applied to text or integer values"_el);
    // Prepare the actual key text that must be found in the index.
    const auto testedKey = value->toTextRepresentation();
    auto keyConstraint = std::dynamic_pointer_cast<KeyConstraint>(rule->constraint(vr::ConstraintType::ConfKey));
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(keyConstraint != nullptr, "Missing key constraint"_el);
    auto keyReferences = keyConstraint->getKeyReferences();
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(!keyReferences.empty(), "ConfKey references cannot be empty"_el);
    bool foundKey = false;
    std::vector<KeyIndexPtr> matchingIndexes;
    matchingIndexes.reserve(keyReferences.size());
    for (const auto &keyReference : keyReferences) {
        ERBSLAND_CORE_CONF_REQUIRE_DEBUG(!keyReference.empty(), "ConfKey reference cannot be empty"_el);
        const auto &keyName = keyReference.at(0);
        ERBSLAND_CORE_CONF_REQUIRE_DEBUG(
            keyName.type() == NameType::Regular, "First element must be a regular name"_el);
        KeyIndexPtr keyIndex;
        for (const auto &index : std::ranges::reverse_view(indexStack)) {
            if (index->name() == keyName) {
                keyIndex = index;
                break;
            }
        }
        ERBSLAND_CORE_CONF_REQUIRE_DEBUG(keyIndex != nullptr, "Missing key index"_el);
        matchingIndexes.emplace_back(keyIndex);
        if (keyReference.size() > 1) {
            ERBSLAND_CORE_CONF_REQUIRE_DEBUG(
                keyReference.at(1).type() == NameType::Index, "Second element must be an index"_el);
            auto index = keyReference.at(1).asIndex();
            // test for a partial key.
            if (keyIndex->hasKey(testedKey, index)) {
                foundKey = true;
                break;
            }
        } else {
            // test the full key.
            if (keyIndex->hasKey(testedKey)) {
                foundKey = true;
                break;
            }
        }
    }
    if (!foundKey) {
        if (keyConstraint->hasCustomError()) {
            throwValidationError(keyConstraint->customError(), value->namePath(), value->location());
        }
        throwValidationError(
            "This value must refer to an existing key, but no matching entry was found"_el,
            value->namePath(),
            value->location());
    }
}

void DocumentValidator::validateDependencies(const conf::ValuePtr &value, const RulePtr &rule) {
    for (const auto &dependency : rule->dependencyDefinitions()) {
        bool hasSource = false;
        bool hasTarget = false;
        for (const auto &path : dependency->sources()) {
            if (const auto depValue = value->value(path); depValue != nullptr) {
                if (!depValue->isDefaultValue()) {
                    hasSource = true;
                    break;
                }
            }
        }
        for (const auto &path : dependency->targets()) {
            if (const auto depValue = value->value(path); depValue != nullptr) {
                if (!depValue->isDefaultValue()) {
                    hasTarget = true;
                    break;
                }
            }
        }
        if (!dependency->mode().isValid(hasSource, hasTarget)) {
            if (dependency->hasErrorMessage()) {
                throwValidationError(dependency->errorMessage(), value->namePath(), value->location());
            }

            text::String message;
            switch (dependency->mode().raw()) {
            case DependencyMode::If:
                message = text::StringFormat{"If {} is configured, you must also configure {}"_el}.build(
                    errorNamePathsOr(dependency->sources(), false), errorNamePathsOr(dependency->targets(), false));
                break;
            case DependencyMode::IfNot:
                message = text::StringFormat{"If {} is configured, you must {}"_el}.build(
                    errorNamePathsOr(dependency->sources(), false), errorNamePathsOr(dependency->targets(), true));
                break;
            case DependencyMode::OR: {
                auto allNamePaths = dependency->sources();
                allNamePaths.insert(allNamePaths.end(), dependency->targets().begin(), dependency->targets().end());
                message = text::StringFormat{"You must configure {}"_el}.build(errorNamePathsOr(allNamePaths, false));
                break;
            }
            case DependencyMode::XOR:
                message = text::StringFormat{"You must either configure {} or configure {}"_el}.build(
                    errorNamePathsOr(dependency->sources(), false), errorNamePathsOr(dependency->targets(), false));
                break;
            case DependencyMode::XNOR:
                message = text::StringFormat{"You must configure {} and configure {}, or none of them"_el}.build(
                    errorNamePathsOr(dependency->sources(), false), errorNamePathsOr(dependency->targets(), false));
                break;
            default:
                message = "Unknown dependency mode"_el;
                break;
            }
            throwValidationError(message, value->namePath(), value->location());
        }
    }
}

}
