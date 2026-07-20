// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RulesFromDocument.hpp"

#include "ValidationError.hpp"
#include "VersionMask.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

auto RulesFromDocument::handleDefault(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    if (!rule->type().acceptsDefaults()) {
        throwValidationError(
            text::StringFormat{"A default value cannot be used for '{}' node rules"_el}.build(rule->type().toText()));
    }
    if (!rule->type().matchesValueType(node->type())) {
        throwValidationError(
            text::StringFormat{"The 'default' value must be {}"_el}.build(rule->type().expectedValueTypeText()));
    }
    rule->setDefaultValue(std::dynamic_pointer_cast<const Value>(node)->deepCopy());
    return {};
}

auto RulesFromDocument::handleDescription(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    if (node->type() != ValueType::Text) {
        throwValidationError("The 'description' value must be text"_el);
    }
    rule->setDescription(node->asText());
    return {};
}

auto RulesFromDocument::handleError(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    if (node->type() != ValueType::Text) {
        throwValidationError("The 'error' value must be text"_el);
    }
    rule->setErrorMessage(node->asText());
    return {};
}

auto RulesFromDocument::handleIsOptional(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    if (node->type() != ValueType::Boolean) {
        throwValidationError("The 'is_optional' value must be boolean"_el);
    }
    rule->setOptional(node->asBoolean());
    return {};
}

auto RulesFromDocument::handleIsSecret(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    if (node->type() != ValueType::Boolean) {
        throwValidationError("The 'is_secret' value must be boolean"_el);
    }
    rule->setSecret(node->asBoolean());
    return {};
}

auto RulesFromDocument::handleTitle(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    if (node->type() != ValueType::Text) {
        throwValidationError("The 'title' value must be a text"_el);
    }
    rule->setTitle(node->asText());
    return {};
}

auto RulesFromDocument::handleVersion(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    const auto versionList = node->asList<Integer>();
    if (versionList.empty()) {
        throwValidationError("The 'version' value must be one or more integers"_el);
    }
    // test for uniqueness.
    for (std::size_t i = 0; i < versionList.size(); ++i) {
        if (versionList[i] < 0) {
            throwValidationError("The values in 'version' must be non-negative integers"_el);
        }
        for (auto j = i + 1; j < versionList.size(); ++j) {
            if (versionList[i] == versionList[j]) {
                throwValidationError("The values in 'version' must be unique"_el);
            }
        }
    }
    auto mask = VersionMask::fromIntegers(versionList);
    if (context.isNegated) {
        mask = !mask;
    }
    rule->limitVersionMask(mask);
    return {};
}

auto RulesFromDocument::handleMinimumVersion(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    if (node->type() != ValueType::Integer) {
        throwValidationError("The 'minimum_version' value must be an integer"_el);
    }
    auto version = node->asInteger();
    if (version < 0) {
        throwValidationError("The 'minimum_version' value must be non-negative"_el);
    }
    auto mask = VersionMask::fromRanges({ConfVersionRange{version, std::numeric_limits<Integer>::max()}});
    if (context.isNegated) {
        mask = !mask;
    }
    rule->limitVersionMask(mask);
    return {};
}

auto RulesFromDocument::handleMaximumVersion(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    if (node->type() != ValueType::Integer) {
        throwValidationError("The 'maximum_version' value must be an integer"_el);
    }
    auto version = node->asInteger();
    if (version < 0) {
        throwValidationError("The 'minimum_version' value must be non-negative"_el);
    }
    auto mask = VersionMask::fromRanges({ConfVersionRange{0, version}});
    if (context.isNegated) {
        mask = !mask;
    }
    rule->limitVersionMask(mask);
    return {};
}

}
