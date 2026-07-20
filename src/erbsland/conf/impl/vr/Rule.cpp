// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Rule.hpp"

#include "../value/Value.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::conf::impl {

using namespace text::literals;

void Rule::addOrOverwriteConstraint(const ConstraintPtr &constraint) {
    auto it = std::ranges::find_if(_constraints, [&constraint](const auto &existingConstraint) -> bool {
        return existingConstraint->type() == constraint->type();
    });
    if (it != _constraints.end()) {
        // replace the existing constraint in-place.
        it = _constraints.erase(it);
        _constraints.insert(it, constraint);
    } else {
        _constraints.emplace_back(constraint);
    }
}

auto Rule::hasConstraint(vr::ConstraintType type) const -> bool {
    return std::ranges::any_of(
        _constraints, [&](const auto &constraint) -> bool { return constraint->type() == type; });
}

auto Rule::hasConstraint(const text::String &name) const -> bool {
    return std::ranges::any_of(
        _constraints, [&](const auto &constraint) -> bool { return constraint->name() == name; });
}

auto Rule::constraint(const text::String &name) const -> ConstraintPtr {
    const auto it =
        std::ranges::find_if(_constraints, [&](const auto &constraint) -> bool { return constraint->name() == name; });
    if (it != _constraints.end()) {
        return *it;
    }
    return {};
}

auto Rule::constraint(vr::ConstraintType type) const -> ConstraintPtr {
    const auto it =
        std::ranges::find_if(_constraints, [&](const auto &constraint) -> bool { return constraint->type() == type; });
    if (it != _constraints.end()) {
        return *it;
    }
    return {};
}

void Rule::addKeyDefinition(const KeyDefinitionPtr &keyDefinition) {
    _keyDefinitions.emplace_back(keyDefinition);
}

auto Rule::hasKeyDefinitions() const -> bool {
    return !_keyDefinitions.empty();
}

auto Rule::keyDefinitions() const -> const KeyDefinitionList & {
    return _keyDefinitions;
}

void Rule::addDependencyDefinition(const DependencyDefinitionPtr &dependencyDefinition) {
    _dependencyDefinitions.emplace_back(dependencyDefinition);
}

auto Rule::child(const Name &name) const -> RulePtr {
    return _children.rule(name);
}

auto Rule::child(const NamePath &namePath) const -> RulePtr {
    if (namePath.empty()) {
        return {};
    }
    auto result = child(namePath.front());
    if (result == nullptr) {
        return {};
    }
    for (std::size_t i = 1; i < namePath.size(); ++i) {
        result = result->child(namePath.at(i));
        if (result == nullptr) {
            return {};
        }
    }
    return result;
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const Rule &rule) -> InternalViewPtr {
    auto result = InternalView::create();
    result->setValue("ruleNamePath", rule._ruleNamePath.toText());
    result->setValue("targetNamePath", rule._targetNamePath.toText());
    result->setValue("type", rule._type.toText());
    result->setUnsafeText("title", rule._title);
    result->setUnsafeText("description", rule._description);
    result->setUnsafeText("errorMessage", rule._errorMessage);
    result->setValue("isOptional", rule._isOptional);
    result->setValue("caseSensitivity", rule._caseSensitivity.toString());
    result->setValue("isSecret", rule._isSecret);
    if (rule._defaultValue != nullptr) {
        result->setValue("defaultValue", rule._defaultValue->toTestText());
    } else {
        result->setValue("defaultValue", text::String{"<null>"_el});
    }
    result->setValue("versionMask", rule._versionMask.toText());
    if (rule._parent.expired()) {
        result->setValue("parent", text::String{"<null>"_el});
    } else {
        result->setValue("parent", rule._parent.lock()->namePath().toText());
    }
    result->setValue(
        "constraints",
        InternalView::createNamedList(
            rule._constraints.begin(), rule._constraints.end(), [](const ConstraintPtr &constraint) -> text::String {
                return text::StringFormat{"Constraint \"{}\""_el}.build(constraint->name());
            }));
    result->setValue(
        "children",
        InternalView::createNamedList(
            rule._children.begin(), rule._children.end(), [](const RulePtr &child) -> text::String {
                return text::StringFormat{"Rule \"{}\""_el}.build(child->ruleNamePath().toText());
            }));
    return result;
}
auto internalView(const RulePtr &rule) -> InternalViewPtr {
    if (rule == nullptr) {
        return InternalView::create();
    }
    return internalView(*rule);
}
#endif

}
