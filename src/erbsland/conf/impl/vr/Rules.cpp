// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Rules.hpp"

#include "DocumentValidator.hpp"
#include "RulesDefinitionValidator.hpp"
#include "ValidationError.hpp"

#include "../utilities/InternalError.hpp"

#include "../../../text/StringFormat.hpp"

#include <algorithm>
#include <ranges>
#include <stdexcept>

namespace erbsland::conf::impl {

using namespace text::literals;

Rules::Rules() : _root{std::make_shared<Rule>()} {
    _root->setType(vr::RuleType::Section);
}

void Rules::validate(const conf::ValuePtr &value, const Integer version) {
    if (value == nullptr) {
        throwValidationError("Cannot validate a null value"_el);
    }
    if (!(value->isDocument() || value->isSectionWithNames())) {
        throwValidationError("The value to validate must be a document or a section with names"_el);
    }
    auto validator = DocumentValidator{_root, value, version};
    validator.validate();
}

auto Rules::empty() const -> bool {
    return _root->empty();
}

auto Rules::isDefinitionValidated() const -> bool {
    return _isDefinitionValidated;
}

auto Rules::addRule(const RulePtr &rule) -> void {
    if (rule == nullptr) {
        throw err::ParameterError{"Cannot add a null rule", "rule"};
    }
    auto parentRule = _root;                 // the parent rule where the given one should be added.
    const auto &path = rule->ruleNamePath(); // The original name path of the rule.
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(!path.empty(), "The name-path of a rule must not be empty");
    if (path.size() > 1) {
        parentRule = ruleForNamePath(path, path.size() - 1);
        if (parentRule == nullptr) {
            throwValidationError(
                text::StringFormat{"Adding a rule failed, because the parent rule for rule '{}' does not exist"_el}
                    .build(path.back().asText()));
        }
    }
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(parentRule != nullptr, "At this point, parentRule must not be null");
    rule->setParent(parentRule);
    parentRule->addChild(rule);
    _isDefinitionValidated = false;
}

auto Rules::addAlternativeRule(const RulePtr &rule) -> void {
    if (rule == nullptr) {
        throw err::ParameterError{"Cannot add a null rule", "rule"};
    }
    auto parentRule = _root;                 // the parent rule where the given one should be added.
    const auto &path = rule->ruleNamePath(); // The original name path of the rule.
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(!path.empty(), "The name-path of a rule must not be empty");
    if (path.size() > 1) {
        parentRule = ruleForNamePath(path, path.size() - 1);
        if (parentRule == nullptr) {
            throwValidationError(
                text::StringFormat{"Adding a rule failed, because the parent rule for rule '{}' does not exist"_el}
                    .build(path.back().asText()));
        }
    }
    ERBSLAND_CORE_CONF_REQUIRE_DEBUG(parentRule != nullptr, "At this point, parentRule must not be null");
    auto alternativeRule = parentRule->child(path.back());
    if (alternativeRule == nullptr) {
        alternativeRule = std::make_shared<Rule>();
        alternativeRule->setRuleNamePath(path);
        alternativeRule->setTargetNamePath(path);
        alternativeRule->setType(vr::RuleType::Alternatives);
        alternativeRule->setParent(parentRule);
        parentRule->addChild(alternativeRule);
    } else {
        if (alternativeRule->type() != vr::RuleType::Alternatives) {
            throwValidationError(
                text::StringFormat{
                    "Adding a rule failed, because the rule '{}' already exists and is no alternative rule"_el}
                    .build(path.subPath(0, path.size() - 1).toText()));
        }
    }
    // Make sure the rule, as part of the alternative, gets a valid index.
    const auto newIndex = alternativeRule->children().size();
    auto newPath = rule->ruleNamePath();
    newPath.append(Name::createIndex(newIndex));
    rule->setRuleNamePath(newPath);
    rule->setParent(alternativeRule);
    alternativeRule->addChild(rule);
    _isDefinitionValidated = false;
}

auto Rules::root() const -> RulePtr {
    return _root;
}

void Rules::validateDefinition() {
    if (_isDefinitionValidated) {
        return; // Skip this if the definition was already validated.
    }
    RulesDefinitionValidator validator{_root};
    validator.validate();
    _isDefinitionValidated = true;
}

auto Rules::ruleForNamePath(const NamePath &path, std::size_t maxDepth) const -> RulePtr {
    if (path.empty()) {
        return {};
    }
    if (maxDepth == 0) {
        maxDepth = path.size();
    } else {
        maxDepth = std::min(maxDepth, path.size());
    }
    auto result = _root;
    for (std::size_t i = 0; i < maxDepth; ++i) {
        result = result->child(path.at(i));
        if (result == nullptr) {
            return {};
        }
    }
    return result;
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const Rules &rule) -> InternalViewPtr {
    return internalView(rule._root);
}
auto internalView(const RulesPtr &rule) -> InternalViewPtr {
    if (rule == nullptr) {
        return InternalView::create();
    }
    return internalView(*rule);
}
#endif

}
