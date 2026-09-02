// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RulesFromDocument.hpp"

#include "DependencyDefinition.hpp"
#include "RulesConstants.hpp"
#include "ValidationError.hpp"

#include "../utilities/InternalError.hpp"
#include "../value/ValueHelper.hpp"
#include "../value/ValueTreeWalker.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void RulesFromDocument::handleTypeOrTemplate(const conf::ValuePtr &node, const RulePtr &rule) {
    if (const auto value = node->value(Name::vrName(Name::VR::Type)); value != nullptr) {
        if (value->type() != ValueType::Text) {
            throwValidationError("The 'type' value must be a text"_el, value->namePath(), value->location());
        }
        auto ruleType = vr::RuleType::fromText(value->asText());
        if (ruleType == vr::RuleType::Undefined) {
            throwValidationError("Unknown rule type"_el, value->namePath(), value->location());
        }
        const auto useTemplateValue = node->value(Name::vrName(Name::VR::UseTemplate));
        if (useTemplateValue != nullptr && !useTemplateValue->type().isStructural()) {
            throwValidationError("The section cannot have both a 'type' and a 'use_template' value"_el);
        }
        rule->setType(ruleType);
    } else if (
        const auto useTemplateValue = node->value(Name::vrName(Name::VR::UseTemplate)); useTemplateValue != nullptr) {
        // Process the template first.
        processTemplate(node, useTemplateValue, rule);
    } else {
        throwValidationError("The section must have either a 'type' or a 'use_template' value"_el);
    }
}

void RulesFromDocument::handleCaseSensitive(const conf::ValuePtr &node, const RulePtr &rule) {
    if (auto caseSensitive = node->value(Name::vrName(Name::VR::CaseSensitive)); caseSensitive != nullptr) {
        if (caseSensitive->type() != ValueType::Boolean) {
            throwValidationError("The 'case_sensitive' value must be boolean"_el);
        }
        rule->setCaseSensitivity(
            caseSensitive->asBoolean() ? text::CaseSensitivity::CaseSensitive : text::CaseSensitivity::CaseInsensitive);
    }
}

void RulesFromDocument::processTemplate(
    const conf::ValuePtr &node, const conf::ValuePtr &useTemplateValue, const RulePtr &rule) {

    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(useTemplateValue != nullptr, "useTemplateValue must not be null"_el);
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(rule != nullptr, "rule must not be null"_el);
    try {
        if (useTemplateValue->type() != ValueType::Text) {
            throwValidationError("The 'use_template' value must be a text"_el);
        }
        if (!_pathForTemplate.empty()) {
            throwValidationError("You must not use 'use_template' in template definitions"_el);
        }
        NamePath templateNamePath;
        try {
            templateNamePath =
                NamePath{{Name::vrName(Name::VR::ReservedTemplate), Name::createRegular(useTemplateValue->asText())}};
        } catch (const ConfError &error) {
            throwValidationError(
                text::StringFormat{"The name specified in 'use_template' is not a valid template name: {}"_el}.build(
                    error.description()));
        }
        const auto templateNode = _document->value(templateNamePath);
        if (templateNode == nullptr) {
            throwValidationError("The template referenced by 'use_template' does not exist"_el);
        }
        if (templateNode->type() != ValueType::SectionWithNames && templateNode->type() != ValueType::SectionList) {
            throwValidationError("Template definitions must be sections or section lists"_el);
        }
        _pathForTemplate = node->namePath();
        if (templateNode->type() == ValueType::SectionList) {
            processAlternatives(templateNode, rule);
        } else {
            processNodeRules(templateNode, rule);
        }
        // Process all template nodes, like regular ones.
        ValueTreeWalker walker;
        walker.setRoot(templateNode);
        auto filter = [](const conf::ValuePtr &processedNode) -> bool {
            return processedNode->type().isMap() || processedNode->type() == ValueType::SectionList;
        };
        walker.setFilter(filter);
        auto visit = [this, templateNode](const conf::ValuePtr &processedNode) -> void {
            if (processedNode == templateNode) {
                return; // The template root is already merged into the current rule.
            }
            processDocumentNode(processedNode);
        };
        walker.walk(visit);
        _pathForTemplate.clear();
    } catch (const ConfError &error) {
        if (error.location().isUndefined()) {
            throw error.withNamePathAndLocation(useTemplateValue->namePath(), useTemplateValue->location());
        }
        throw;
    }
}

void RulesFromDocument::processImplicitRules(const conf::ValuePtr &node, const RulePtr &rule) {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(
        node->type() == ValueType::IntermediateSection, "Expected intermediate section node"_el);
    // For intermediate sections, create a rule that expects a section but do not add any constraints.
    rule->setLocation(node->location());
    rule->setType(vr::RuleType::Section);
}

void RulesFromDocument::processAlternatives(const conf::ValuePtr &node, const RulePtr &rule) {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(node->type() == ValueType::SectionList, "Expected section list node"_el);
    // For section lists, create a rule with alternatives. As we traverse the whole node tree,
    // the alternatives will be automatically created later.
    rule->setLocation(node->location());
    rule->setType(vr::RuleType::Alternatives);
}

void RulesFromDocument::processNameNode(const conf::ValuePtr &node) {
    const auto rule = std::make_shared<Rule>();
    rule->setLocation(node->location());
    rule->setRuleNamePath(createRuleNamePath(node->namePath()));
    _rules->addRule(rule); // add the rule early to ensure we have a valid parent node.
    rule->setType(vr::RuleType::Text);
    if (node->type() == ValueType::SectionList) {
        throwValidationError("Name node-rules definitions cannot be alternatives"_el);
    }
    if (node->type() == ValueType::IntermediateSection) {
        throwValidationError("Name node-rules definition must not have subsections"_el);
    }
    processNameNodeRules(node, rule);
}

void RulesFromDocument::processDependencies(const conf::ValuePtr &node) {
    if (node->type() != ValueType::SectionList) {
        throwValidationError("Dependency 'vr_dependency' node-rules definitions must be section lists"_el);
    }
    const auto parentRule = getParentRuleForNode(node);
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(parentRule != nullptr, "Expected parent rule for key node"_el);
    // Process the individual dependency definitions.
    for (const auto &dependencyNode : getImplValue(node)->childrenImpl()) {
        if (dependencyNode->type() != ValueType::SectionWithNames) {
            throwValidationError("Dependency definitions must be sections with regular names"_el);
        }
        auto sourceSpecified = false;
        auto targetSpecified = false;
        auto mode = vr::DependencyMode{vr::DependencyMode::Undefined};
        NamePathList sourcePaths;
        NamePathList targetPaths;
        text::String errorMessage;
        for (const auto &child : dependencyNode->childrenImpl()) {
            try {
                if (child->name() == Name::vrName(Name::VR::DepMode)) {
                    if (child->type() != ValueType::Text) {
                        throwValidationError("The 'mode' value in 'vr_dependency' must be a text value"_el);
                    }
                    mode = vr::DependencyMode::fromText(child->asText());
                    if (mode == vr::DependencyMode::Undefined) {
                        throwValidationError(
                            "The 'mode' value in 'vr_dependency' must be one of: 'if', 'if_not', 'or', 'xnor', "_el
                            "'xor'"_el);
                    }
                } else if (
                    child->name() == Name::vrName(Name::VR::DepSource) ||
                    child->name() == Name::vrName(Name::VR::DepTarget)) {
                    auto namePathTexts = child->asList<text::String>();
                    if (namePathTexts.empty()) {
                        throwValidationError(
                            text::StringFormat{"The '{}' value in 'vr_dependency' must one or more text values"_el}
                                .build(child->name().asText()));
                    }
                    if (namePathTexts.size() > 10) {
                        throwValidationError(
                            text::StringFormat{
                                "This implementation does not support more than 10 '{}' paths in one 'vr_dependency'"_el}
                                .build(child->name().asText()));
                    }
                    NamePathList paths;
                    for (const auto &namePathText : namePathTexts) {
                        NamePath path;
                        try {
                            path = NamePath::fromText(namePathText);
                        } catch (const ConfError &) {
                            throwValidationError(
                                text::StringFormat{"A name path in '{}' in 'vr_dependency' is not valid"_el}.build(
                                    child->name().asText()));
                        }
                        paths.emplace_back(std::move(path));
                    }
                    if (child->name() == Name::vrName(Name::VR::DepSource)) {
                        sourcePaths = std::move(paths);
                        sourceSpecified = true;
                    } else {
                        targetPaths = std::move(paths);
                        targetSpecified = true;
                    }
                } else if (child->name() == Name::vrName(Name::VR::DepError)) {
                    if (child->type() != ValueType::Text) {
                        throwValidationError("The 'error' value in 'vr_dependency' must be a text value"_el);
                    }
                    errorMessage = child->asText();
                } else {
                    throwValidationError("Unexpected element in 'vr_dependency'"_el);
                }
            } catch (const ConfError &error) {
                if (error.location().isUndefined()) {
                    throw error.withNamePathAndLocation(child->namePath(), child->location());
                }
                throw;
            }
        }
        if (mode == vr::DependencyMode::Undefined) {
            throwValidationError("A 'vr_dependency' definition must have a 'mode' value"_el);
        }
        if (!sourceSpecified) {
            throwValidationError("A 'vr_dependency' definition must have a 'source' value"_el);
        }
        if (!targetSpecified) {
            throwValidationError("A 'vr_dependency' definition must have a 'target' value"_el);
        }
        auto dependencyDefinition = DependencyDefinition::create(mode, sourcePaths, targetPaths, errorMessage);
        dependencyDefinition->setLocation(dependencyNode->location());
        parentRule->addDependencyDefinition(dependencyDefinition);
    }
}

void RulesFromDocument::processKey(const conf::ValuePtr &node) {
    if (node->type() != ValueType::SectionList) {
        throwValidationError("ConfKey 'vr_key' node-rules definitions must be section lists"_el);
    }
    const auto parentRule = getParentRuleForNode(node);
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(parentRule != nullptr, "Expected parent rule for key node"_el);
    // Assign the individual keys
    for (const auto &child : getImplValue(node)->childrenImpl()) {
        Name name;
        const auto nameValue = child->value(Name::vrName(Name::VR::KeyName));
        if (nameValue != nullptr) {
            if (nameValue->type() != ValueType::Text) {
                throwValidationError("The 'name' in 'vr_key' must be a text value with a regular name"_el);
            }
            try {
                name = Name::createRegular(nameValue->asText());
            } catch (const ConfError &error) {
                throwValidationError(
                    text::StringFormat{"The 'name' in 'vr_key' is not a valid regular name: {}"_el}.build(
                        error.description()),
                    nameValue->namePath(),
                    nameValue->location());
            }
        }
        const auto keyPathValue = child->value(Name::vrName(Name::VR::KeyKey));
        if (keyPathValue == nullptr) {
            throwValidationError("A 'vr_key' definition must have a 'key' value"_el);
        }
        const auto keyPaths = keyPathValue->toValueList();
        if (keyPaths.empty()) {
            throwValidationError("The 'key' in 'vr_key' must be either a text value or a list of text values"_el);
        }
        if (keyPaths.size() > 10) {
            throwValidationError("This implementation does not support more than 10 'key' paths in one 'vr_key'"_el);
        }
        for (const auto &keyPath : keyPaths) {
            if (keyPath->type() != ValueType::Text) {
                throwValidationError(
                    text::StringFormat{"Expected one or more text values in 'key' of 'vr_key', but got {}"_el}.build(
                        keyPath->type().toValueDescription(true)),
                    keyPath->namePath(),
                    keyPath->location());
            }
        }
        auto caseSensitivity = text::CaseSensitivity::CaseInsensitive;
        const auto caseSensitiveValue = child->value(Name::vrName(Name::VR::CaseSensitive));
        if (caseSensitiveValue != nullptr) {
            if (caseSensitiveValue->type() != ValueType::Boolean) {
                throwValidationError("The 'case_sensitive' value must be boolean"_el);
            }
            caseSensitivity = caseSensitiveValue->asBoolean() ? text::CaseSensitivity::CaseSensitive
                                                              : text::CaseSensitivity::CaseInsensitive;
        }
        KeyDefinition::Keys keys;
        for (const auto &keyPath : keyPaths) {
            try {
                keys.emplace_back(NamePath::fromText(keyPath->asText()));
            } catch (const ConfError &error) {
                throwValidationError(
                    text::StringFormat{"A name path in 'key' in 'vr_key' is not valid: {}"_el}.build(
                        error.description()),
                    keyPath->namePath(),
                    keyPath->location());
            }
        }
        parentRule->addKeyDefinition(KeyDefinition::create(name, keys, caseSensitivity, child->location()));

        // scan for additional unwanted elements.
        for (const auto &subChild : *child) {
            if (subChild->name() != Name::vrName(Name::VR::KeyKey) &&
                subChild->name() != Name::vrName(Name::VR::KeyName) &&
                subChild->name() != Name::vrName(Name::VR::CaseSensitive)) {
                throwValidationError("Unexpected element in 'vr_key'"_el, subChild->namePath(), subChild->location());
            }
        }
    }
}

auto RulesFromDocument::getParentRuleForNode(const conf::ValuePtr &node) const -> RulePtr {
    auto ruleNamePath = createRuleNamePath(node->namePath());
    if (ruleNamePath.empty()) {
        return nullptr;
    }
    if (ruleNamePath.size() == 1) {
        return _rules->root();
    }
    return _rules->ruleForNamePath(ruleNamePath, ruleNamePath.size() - 1);
}

auto RulesFromDocument::createRuleNamePath(const NamePath &namePath) const -> NamePath {
    if (namePath.empty() || _pathForTemplate.empty()) {
        return namePath;
    }
    if (namePath.front() == Name::vrName(Name::VR::ReservedTemplate)) {
        auto result = _pathForTemplate;
        auto it = namePath.begin();
        ++it;     // skip "vr_template"
        if (it != namePath.end()) {
            ++it; // skip template name
        }
        for (; it != namePath.end(); ++it) {
            result.append(*it);
        }
        return result;
    }
    return namePath;
}

auto RulesFromDocument::createTargetNamePath(const NamePath &namePath) const -> NamePath {
    NamePath result;
    std::size_t startIndex = 0;
    if (isTemplatePath(namePath)) {
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(!_pathForTemplate.empty(), "Expected non-empty _pathForTemplate"_el);
        result = _pathForTemplate;
        startIndex = 2; // skip "vr_template.<template-name>"
    }
    appendRegularNames(result, namePath, startIndex);
    return result;
}

auto RulesFromDocument::isTemplatePath(const NamePath &namePath) -> bool {
    return !namePath.empty() && namePath.front() == Name::vrName(Name::VR::ReservedTemplate);
}

void RulesFromDocument::appendRegularNames(NamePath &result, const NamePath &namePath, const std::size_t startIndex) {
    for (std::size_t i = startIndex; i < namePath.size(); ++i) {
        const auto &name = namePath.at(i);
        if (name.type() != NameType::Regular) {
            continue;
        }
        if (name.isEscapedReservedValidationRule()) {
            result.append(name.withReservedVRPrefixRemoved());
        } else {
            result.append(name);
        }
    }
}

}
