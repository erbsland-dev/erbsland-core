// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RulesFromDocument.hpp"

#include "CharsConstraint.hpp"
#include "EqualsConstraint.hpp"
#include "InConstraint.hpp"
#include "KeyConstraint.hpp"
#include "MatchesConstraint.hpp"
#include "MinMaxConstraint.hpp"
#include "MultipleConstraint.hpp"
#include "RulesConstants.hpp"
#include "StringPartConstraint.hpp"
#include "ValidationError.hpp"

#include "../utilities/InternalError.hpp"
#include "../value/ValueTreeWalker.hpp"

#include "../../../text/StringFormat.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::conf::impl {

using namespace text::literals;

auto RulesFromDocument::constraintHandlerTable() -> const ConstraintHandlers & {
    static const ConstraintHandlers constraintHandlers = {
        {vrc::ctChars, &handleCharsConstraint},
        {vrc::ctContains, &handleContainsConstraint},
        {vrc::ctDefault, &RulesFromDocument::handleDefault, false, false},
        {vrc::ctDescription, &RulesFromDocument::handleDescription, false, false},
        {vrc::ctEnds, &handleEndsConstraint},
        {vrc::ctEquals, &handleEqualsConstraint},
        {vrc::ctError, &RulesFromDocument::handleError, false, false},
        {vrc::ctIn, &handleInConstraint},
        {vrc::ctIsOptional, &RulesFromDocument::handleIsOptional, false, false},
        {vrc::ctIsSecret, &RulesFromDocument::handleIsSecret, false, false},
        {vrc::ctKey, handleKeyConstraint, true, true},
        {vrc::ctMatches, handleMatchesConstraint},
        {vrc::ctMaximum, &handleMaximumConstraint},
        {vrc::ctMaximumVersion, &RulesFromDocument::handleMaximumVersion, true, false},
        {vrc::ctMinimum, &handleMinimumConstraint},
        {vrc::ctMinimumVersion, &RulesFromDocument::handleMinimumVersion, true, false},
        {vrc::ctMultiple, &handleMultipleConstraint},
        {vrc::ctStarts, &handleStartsConstraint},
        {vrc::ctTitle, &RulesFromDocument::handleTitle, false, false},
        {vrc::ctVersion, &RulesFromDocument::handleVersion, true, false},
    };
    return constraintHandlers;
}

void RulesFromDocument::read() {
    if (!_rules->empty()) {
        throwValidationError("Rules from a document can only be read into an empty rule-set"_el);
    }
    auto filter = [](const conf::ValuePtr &node) -> bool {
        if (!node->isDocument() && node->namePath().front() == Name::vrName(Name::VR::ReservedTemplate)) {
            return false; // Skip the template nodes, as they are just referenced from regular nodes.
        }
        if (node->name().isIndex()) {
            if (!node->hasParent()) {
                return false; // coverage: this should never happen
            }
            const auto parentName = node->parent()->name();
            if (parentName == Name::vrName(Name::VR::ReservedKey) ||
                parentName == Name::vrName(Name::VR::ReservedDependency)) {
                return false; // skip the individual entries in `vr_key` and `vr_dependency`
            }
        }
        return node->type().isMap() || node->type() == ValueType::SectionList;
    };
    auto visit = [this](const conf::ValuePtr &node) -> void {
        if (!node->isDocument()) {
            processDocumentNode(node);
        }
    };
    // Document derives from Value; pass it as root via the instance API.
    ValueTreeWalker walker;
    walker.setRoot(_document);
    walker.setFilter(filter);
    walker.walk(visit);
}

void RulesFromDocument::processDocumentNode(const conf::ValuePtr &node) {
    try {
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(!node->isDocument(), "Document nodes are not allowed in validation rules"_el);
        if (node->type() == ValueType::SectionWithTexts) {
            throwValidationError("Section with texts is not allowed in a validation rules document"_el);
        }
        const auto namePath = node->namePath();
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(!namePath.empty(), "Expected non-empty name path for a node"_el);
        if (namePath.containsText()) {
            throwValidationError("Text names are not allowed in a validation rules document"_el);
        }
        const auto &name = namePath.back();
        if (name == Name::vrName(Name::VR::ReservedTemplate)) {
            // As the template node is filtered, any occurrence of `vr_template` means that this definition
            // is a subsection and therefore at the wrong place.
            throwValidationError("Templates must be defined in the document root"_el);
        }
        if (name == Name::vrName(Name::VR::ReservedName)) {
            processNameNode(node);
        } else if (name == Name::vrName(Name::VR::ReservedDependency)) {
            processDependencies(node);
        } else if (name == Name::vrName(Name::VR::ReservedKey)) {
            processKey(node);
        } else if (name == Name::vrName(Name::VR::ReservedEntry) || name == Name::vrName(Name::VR::ReservedAny)) {
            processRegularNode(node);
        } else if (!name.isReservedValidationRule() || name.isEscapedReservedValidationRule()) {
            processRegularNode(node);
        } else {
            throwValidationError("Unknown reserved name"_el);
        }
    } catch (const ConfError &error) {
        if (error.location().isUndefined()) {
            throw error.withNamePathAndLocation(node->namePath(), node->location());
        }
        throw;
    }
}

void RulesFromDocument::processRegularNode(const conf::ValuePtr &node) {
    const auto rule = std::make_shared<Rule>();
    rule->setLocation(node->location());
    rule->setRuleNamePath(createRuleNamePath(node->namePath()));
    rule->setTargetNamePath(createTargetNamePath(node->namePath()));
    // Add the unfinished rule early to the structure ensuring we have a valid parent.
    _rules->addRule(rule);
    if (node->type() == ValueType::SectionList) {
        processAlternatives(node, rule);
    } else if (node->type() == ValueType::IntermediateSection) {
        processImplicitRules(node, rule);
    } else {
        processNodeRules(node, rule);
    }
}

void RulesFromDocument::processNodeRules(const conf::ValuePtr &node, const RulePtr &rule) {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(
        node->type() == ValueType::SectionWithNames, "Expected section with names node"_el);
    handleTypeOrTemplate(node, rule);
    if (rule->type() == vr::RuleType::Alternatives && node->hasValue(Name::vrName(Name::VR::UseTemplate))) {
        // if we used a template with alternatives, do not allow further constraint definitions.
        for (const auto &value : *node) {
            if (value->name() != Name::vrName(Name::VR::UseTemplate)) {
                throwValidationError(
                    "Templates that define alternatives cannot be customized at the usage location"_el);
            }
        }
    } else {
        handleCaseSensitive(node, rule);
        processCommonNodeRules(node, rule);
    }
}

void RulesFromDocument::processNameNodeRules(const conf::ValuePtr &node, const RulePtr &rule) {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(
        node->type() == ValueType::SectionWithNames, "Expected section with names node"_el);
    if (const auto value = node->value(Name::vrName(Name::VR::Type)); value != nullptr) {
        if (value->type() != ValueType::Text) {
            throwValidationError("The 'type' value must be a text"_el, value->namePath(), value->location());
        }
        if (vr::RuleType::fromText(value->asText()) != vr::RuleType::Text) {
            throwValidationError(
                "Name node-rules must have a 'type' value of 'text'"_el, value->namePath(), value->location());
        }
    }
    if (node->hasValue(Name::vrName(Name::VR::UseTemplate))) {
        throwValidationError("Name node-rules cannot have a 'use_template' value"_el);
    }
    handleCaseSensitive(node, rule);
    processCommonNodeRules(node, rule);
}

void RulesFromDocument::processCommonNodeRules(const conf::ValuePtr &node, const RulePtr &rule) {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(
        node->type() == ValueType::SectionWithNames, "Expected section with names node"_el);
    std::unordered_map<text::String, text::String> customErrorMessages;
    for (const auto &value : *node) {
        try {
            if (value->type().isStructural()) {
                if (rule->ruleName() == Name::vrName(Name::VR::ReservedName)) {
                    throwValidationError("A 'vr_name' section cannot have subsections"_el);
                }
                continue; // Ignore subsections, section lists, etc.
            }
            if (value->name() == Name::vrName(Name::VR::Type) || value->name() == Name::vrName(Name::VR::UseTemplate) ||
                value->name() == Name::vrName(Name::VR::CaseSensitive)) {
                continue; // "type", "use_template" and "case_sensitive" are already handled.
            }
            handleConstraintAndAttributes(customErrorMessages, rule, value);
        } catch (const ConfError &error) {
            if (error.location().isUndefined()) {
                throw error.withNamePathAndLocation(value->namePath(), value->location());
            }
            throw;
        }
    }
    for (const auto &[name, errorMessage] : customErrorMessages) {
        if (!rule->hasConstraint(name)) {
            throwValidationError(
                text::StringFormat{"There is no constraint '{0}' for the custom error message '{0}_error'"_el}.build(
                    name));
        }
        const auto constraint = rule->constraint(name);
        constraint->setErrorMessage(errorMessage);
    }
}

void RulesFromDocument::handleConstraintAndAttributes(
    std::unordered_map<text::String, text::String> &customErrorMessages,
    const RulePtr &rule,
    const conf::ValuePtr &value) {

    auto name = value->name().asText();
    if (name.endsWith(vrc::ctSuffixError)) {
        name = name.slice(unit::ByteRange{unit::ByteIndex::zero(), name.length() - vrc::ctSuffixError.length()});
        auto lookupName = name;
        if (name.startsWith(vrc::ctPrefixNot)) {
            lookupName = name.slice(
                unit::ByteRange{
                    unit::ByteIndex::fromSizeT(vrc::ctPrefixNot.length().toSizeT()),
                    name.length() - vrc::ctPrefixNot.length()});
        }
        auto constraintHandler = resolveConstraintHandler(lookupName);
        if (!constraintHandler.acceptError) {
            throwValidationError(text::StringFormat{"'{}' does not accept '_error' suffixes"_el}.build(name));
        }
        customErrorMessages[name] = value->asText();
        return;
    }
    bool isNegated = false;
    auto lookupName = name;
    if (name.startsWith(vrc::ctPrefixNot)) {
        isNegated = true;
        lookupName = name.slice(
            unit::ByteRange{
                unit::ByteIndex::fromSizeT(vrc::ctPrefixNot.length().toSizeT()),
                name.length() - vrc::ctPrefixNot.length()});
    }
    ConstraintPtr constraint;
    auto constraintHandler = resolveConstraintHandler(lookupName);
    if (isNegated && !constraintHandler.acceptNot) {
        throwValidationError(text::StringFormat{"'{}' does not accept 'not_' prefixes"_el}.build(name));
    }
    auto context = ConstraintHandlerContext{.rule = rule, .node = value, .isNegated = isNegated};
    constraint = constraintHandler.handler(context);
    if (constraint != nullptr) {
        constraint->setName(name);
        constraint->setLocation(value->location());
        constraint->setNegated(isNegated);
        const auto isFromTemplate = value->namePath().front() == Name::vrName(Name::VR::ReservedTemplate);
        constraint->setFromTemplate(isFromTemplate);
        // Test if there is a conflict and if overwriting is allowed.
        if (rule->hasConstraint(constraint->type())) {
            const auto existingConstraint = rule->constraint(constraint->type());
            if (isFromTemplate == existingConstraint->isFromTemplate()) {
                // Prevent duplicates in templates and non-templates.
                // Improve the error message if
                if (existingConstraint->name() != constraint->name()) {
                    throwValidationError(
                        text::StringFormat{"Constraint '{}' for type '{}' is already defined. "
                                           "You must not mix positive and negative constraints for the same type"_el}
                            .build(constraint->name(), constraint->type().toText()));
                }
                throwValidationError(
                    text::StringFormat{"Constraint '{}' is already defined"_el}.build(constraint->name()));
            }
        }
        // Add or overwrite the constraint.
        rule->addOrOverwriteConstraint(constraint);
    }
}

auto RulesFromDocument::resolveConstraintHandler(const text::String &name) -> const ConstraintHandler & {
    const auto &constraintHandlers = constraintHandlerTable();
    const auto it = std::ranges::find_if(
        constraintHandlers, [name](const ConstraintHandler &handler) -> bool { return handler.name == name; });
    if (it == constraintHandlers.end()) {
        text::StringEditor message{"Unknown constraint: "_el};
        message.append(name);
        throwValidationError(text::String{message});
    }
    return *it;
}

}
