// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DocumentValidator.hpp"

#include "ValidationError.hpp"

#include "../utilities/InternalError.hpp"
#include "../value/ValueHelper.hpp"
#include "../value/ValueTreeWalker.hpp"

#include "../../../text/StringFormat.hpp"

#include <optional>
#include <ranges>

namespace erbsland::conf::impl {

using namespace text::literals;

auto DocumentValidator::validate(const RulePtr &rule, const ValuePtr &value) -> RulePtr {
    validateNameConstraints(rule, value);
    if (rule->hasKeyDefinitions() || rule->hasConstraint(vr::ConstraintType::ConfKey)) {
        _useIndexes = true;
    }
    if (rule->hasDependencyDefinitions()) {
        _useDependencies = true;
    }
    switch (rule->type()) {
    case vr::RuleType::NotValidated:
        return handleNotValidatedValues(rule, value);
    case vr::RuleType::Alternatives:
        return handleAlternatives(rule, value);
    case vr::RuleType::SectionList:
        return handleSectionLists(rule, value);
    case vr::RuleType::ValueList:
        return handleValueLists(rule, value);
    case vr::RuleType::ValueMatrix:
        return handleValueMatrix(rule, value);
    default:
        break;
    }
    return handleCommonValues(rule, value);
}

void DocumentValidator::handleMissingValues(const RulePtr &rule, const conf::ValuePtr &parentValue) {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(rule != nullptr, "The rule must not be null");
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(parentValue != nullptr, "The parent value must not be null");
    if (rule->type() == vr::RuleType::NotValidated || !rule->versionMask().matches(_version) ||
        rule->ruleName().isReservedValidationRule() || rule->isOptional()) {
        return; // ignore values that do not need to exit.
    }
    if (rule->hasDefault()) {
        copyDefaultValue(rule, parentValue); // if we have a default value, apply it, done.
        return;
    }
    if (rule->type() == vr::RuleType::Alternatives) {
        for (const auto &alternativeRule : rule->childrenImpl()) {
            if (!alternativeRule->versionMask().matches(_version)) {
                continue; // only consider rules that are active for the current version.
            }
            if (alternativeRule->isOptional()) {
                return; // ignore optional values.
            }
            if (alternativeRule->hasDefault()) {
                copyDefaultValue(alternativeRule, parentValue);
                return; // apply the default, done.
            }
        }
    }
    // at this point, the missing value for the rule couldn't be resolved.
    throwValidationError(
        text::StringFormat{"In {}, expected {} with the name '{}'"_el}.build(
            parentLocationText(parentValue), expectedValueTypeText(rule), rule->targetName().toPathText()),
        parentValue->namePath(),
        parentValue->location());
}

void DocumentValidator::copyDefaultValue(const RulePtr &rule, const conf::ValuePtr &parentValue) {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(rule != nullptr, "The rule must not be null");
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(parentValue != nullptr, "The parent value must not be null");
    const auto defaultValue = rule->defaultValue()->deepCopy();
    defaultValue->setName(rule->targetName());
    defaultValue->setParent(parentValue);
    ValueTreeWalker treeWalker;
    treeWalker.setRoot(defaultValue);
    treeWalker.walk([&rule](const conf::ValuePtr &value) -> void {
        const auto valueImpl = getImplValue(value);
        valueImpl->setValidationRule(rule);
        valueImpl->markAsDefaultValue();
    });
    callImplValueFn(parentValue, [&defaultValue](auto &&valueImpl) -> void { valueImpl->addValue(defaultValue); });
}

auto DocumentValidator::handleNotValidatedValues(const RulePtr &rule, const ValuePtr &value) -> RulePtr {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(rule != nullptr, "The rule must not be null");
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(value != nullptr, "The value must not be null");
    // Mark the whole branch in the value-tree as not-validated.
    ValueTreeWalker treeWalker;
    treeWalker.setRoot(value);
    treeWalker.walk([&rule](const conf::ValuePtr &value) -> void {
        const auto valueImpl = getImplValue(value);
        valueImpl->setValidationRule(rule);
    });
    return {};
}

auto DocumentValidator::handleAlternatives(const RulePtr &rule, const ValuePtr &value) -> RulePtr {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(rule != nullptr, "The rule must not be null");
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(value != nullptr, "The value must not be null");
    // validate all alternatives and return the first matching rule.
    std::vector<RulePtr> matchingRules;
    matchingRules.reserve(rule->childrenImpl().size());
    for (const auto &alternativeRule : rule->childrenImpl()) {
        if (alternativeRule->versionMask().matches(_version) &&
            alternativeRule->type().matchesValueType(value->type())) {

            matchingRules.emplace_back(alternativeRule);
        }
    }
    // if we have no rules that match by version and type, report an error showing all possible types.
    if (matchingRules.empty()) {
        throwExpectedVsActual(rule, value);
    }
    // if we have one or more matching rules, search one that matches.
    std::optional<ConfError> firstError;
    RulePtr matchingRule = {};
    for (const auto &alternativeRule : matchingRules) {
        try {
            validateValueConstraints(alternativeRule, value);
            matchingRule = alternativeRule;
            break;
        } catch (const ConfError &error) {
            if (!firstError.has_value()) {
                firstError = error;
            }
        }
    }
    // if no matching rule was found, throw an error
    if (matchingRule == nullptr) {
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(firstError.has_value(), "Expected having an error to throw");
        throw ConfError{firstError.value()};
    }
    return matchingRule;
}

auto DocumentValidator::handleSectionLists(const RulePtr &rule, const ValuePtr &value) -> RulePtr {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(rule != nullptr, "The rule must not be null");
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(value != nullptr, "The value must not be null");
    if (value->type() != ValueType::SectionList) {
        throwExpectedVsActual(rule, value);
    }
    validateValueConstraints(rule, value);
    return rule;
}

auto DocumentValidator::handleValueListOrMatrixPreCheck(const RulePtr &rule, const ValuePtr &value) -> RulePtr {

    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(rule != nullptr, "The rule must not be null");
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(value != nullptr, "The value must not be null");
    // Check the constraints for the list's size.
    validateValueConstraints(rule, value);
    // Make sure we actually got a list of values or scalar.
    if (value->type() != ValueType::ValueList && !value->type().isScalar()) {
        throwValidationError(
            text::StringFormat{"Expected a list of values, but found {}"_el}.build(
                value->type().toValueDescription(true)));
    }
    // If this is true, we are sure that the value can be converted into a value list.
    const auto valueRule = rule->child(vrc::cReservedEntry);
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(valueRule != nullptr, "Missing 'vr_entry' rule for list rule");
    return valueRule;
}

void DocumentValidator::validateListOrMatrixValue(const RulePtr &valueRule, const ValuePtr &value) {

    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(valueRule != nullptr, "The value rule must not be null");
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(value != nullptr, "The value must not be null");
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(
        valueRule->type().isScalar() || valueRule->type() == vr::RuleType::Alternatives,
        "Unexpected rule type for 'vr_entry'");

    // Use the regular handlers to validate the list/matrix values.
    RulePtr validatedRule;
    if (valueRule->type() == vr::RuleType::Alternatives) {
        validatedRule = handleAlternatives(valueRule, value);
    } else {
        validatedRule = handleCommonValues(valueRule, value);
    }
    // Assign the rule that was used to validate the value.
    value->setValidationRule(validatedRule);
}

auto DocumentValidator::handleValueLists(const RulePtr &rule, const ValuePtr &value) -> RulePtr {
    const auto valueRule = handleValueListOrMatrixPreCheck(rule, value);
    for (const auto &valueListEntry : value->toValueList()) {
        validateListOrMatrixValue(valueRule, getImplValue(valueListEntry));
    }
    return rule;
}

auto DocumentValidator::handleValueMatrix(const RulePtr &rule, const ValuePtr &value) -> RulePtr {
    const auto valueRule = handleValueListOrMatrixPreCheck(rule, value);
    const auto valueMatrix = value->toValueMatrix();
    for (unit::ElementIndex row; row.isWithin(valueMatrix.rowCount()); ++row) {
        for (unit::ElementIndex column; column.isWithin(valueMatrix.columnCount()); ++column) {
            if (valueMatrix.isDefined(row, column)) {
                validateListOrMatrixValue(valueRule, getImplValue(valueMatrix.value(row, column)));
            }
        }
    }
    return rule;
}

auto DocumentValidator::handleCommonValues(const RulePtr &rule, const ValuePtr &value) -> RulePtr {
    // Validate the expected value type.
    if (!rule->type().matchesValueType(value->type())) {
        throwExpectedVsActual(rule, value);
    }
    validateValueConstraints(rule, value);
    return rule;
}

auto DocumentValidator::nextRuleForValue(const RulePtr &parentRule, const ValuePtr &value) const -> RulePtr {
    const auto name = value->name();
    if (name.isIndex()) {
        // An index as the name means that this is an entry of a list.
        // therefore, the vr_entry rule is used to validate the list entry.
        auto entryRule = parentRule->child(vrc::cReservedEntry);
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(entryRule != nullptr, "Missing entry rule for list rule");
        return entryRule;
    }
    RulePtr anyRule = {};
    for (const auto &childRule : parentRule->childrenImpl()) {
        if (!childRule->versionMask().matches(_version)) {
            continue; // ignore all rules that do not match the current version
        }
        if (childRule->ruleName() == vrc::cReservedAny) {
            anyRule = childRule; // store the "any" rule for later. It is evaluated if no other rule matches.
            continue;
        }
        if (childRule->targetName() == name) {
            return childRule;
        }
    }
    if (anyRule != nullptr) {
        return anyRule;
    }
    throwValidationError(
        text::StringFormat{"Found an unexpected {} in this document"_el}.build(value->type().toValueDescription(false)),
        value->namePath(),
        value->location());
}

}
