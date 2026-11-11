// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Rule.hpp"

#include "NamePathHelper.hpp"
#include "ValidationError.hpp"

#include "../value/Value.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../vr/builder/attribute/Attribute.hpp"

#include <algorithm>
#include <limits>
#include <ranges>

namespace erbsland::conf::impl {

using namespace text;
using namespace text::literals;

auto Rule::roleForDefinitionName(const Name &name) -> RuleRole {
    if (name.isIndex()) {
        return RuleRole::Alternative;
    }
    if (name == Name::vrName(Name::VR::ReservedEntry)) {
        return RuleRole::ListEntry;
    }
    if (name == Name::vrName(Name::VR::ReservedAny)) {
        return RuleRole::AnyValue;
    }
    if (name == Name::vrName(Name::VR::ReservedName)) {
        return RuleRole::NameConstraint;
    }
    return RuleRole::NamedValue;
}

auto Rule::childByRole(const RuleRole role) const -> RulePtr {
    for (const auto &child : _children) {
        if (child->role() == role) {
            return child;
        }
    }
    return {};
}

auto Rule::nameConstraintRule() const -> RulePtr {
    return childByRole(RuleRole::NameConstraint);
}

auto Rule::listEntryRule() const -> RulePtr {
    return childByRole(RuleRole::ListEntry);
}

auto Rule::anyValueRule() const -> RulePtr {
    return childByRole(RuleRole::AnyValue);
}

auto Rule::childByValueName(const Name &name) const -> RulePtr {
    for (const auto &child : _children) {
        if (child->role() == RuleRole::NamedValue && child->valueName() == name) {
            return child;
        }
    }
    return {};
}

auto Rule::childForValuePathComponent(const Name &name) const -> RulePtr {
    if (!name.isRegular() || _type == vr::RuleType::Alternatives) {
        return {};
    }
    if ((_type == vr::RuleType::SectionList || _type == vr::RuleType::ValueList ||
            _type == vr::RuleType::ValueMatrix) &&
        name == Name::vrName(Name::VR::ReservedEntry)) {
        return listEntryRule();
    }
    return childByValueName(name);
}

auto Rule::descendantByValuePath(const NamePath &path) const -> RulePtr {
    if (path.empty()) {
        return {};
    }
    auto result = childForValuePathComponent(path.front());
    for (std::size_t index = 1; result != nullptr && index < path.size(); ++index) {
        result = result->childForValuePathComponent(path.at(index));
    }
    return result;
}

void Rule::setEntryType(const vr::RuleType type) {
    if (_type != vr::RuleType::ValueList || !type.isScalar() || !_entryType.isUndefined()) {
        throwValidationError("EntryType requires a value list, a scalar entry type, and no previous EntryType."_el);
    }
    _entryType = type;
}

void Rule::addEntryAttributes(std::vector<std::shared_ptr<const vr::builder::Attribute>> attributes) {
    if (_type != vr::RuleType::ValueList) {
        throwValidationError("Entry attributes require a value list."_el);
    }
    for (const auto &attribute : attributes) {
        if (attribute == nullptr) {
            throw err::ParameterFailure{"An entry attribute cannot be null."_el, "attributes"_el};
        }
    }
    _hasEntryAttributes = true;
    _entryAttributes.insert(_entryAttributes.end(), attributes.begin(), attributes.end());
}

void Rule::setDefaultValue(const conf::ValuePtr &value) {
    _defaultValue = std::dynamic_pointer_cast<impl::Value>(value);
    if (value != nullptr && _defaultValue == nullptr) {
        throwValidationError("The default value is not an Erbsland configuration value"_el);
    }
}

void Rule::addConstraint(const vr::ConstraintPtr &constraint, String name, const bool isNegated, String errorMessage) {
    auto constraintImpl = std::dynamic_pointer_cast<impl::Constraint>(constraint);
    if (constraintImpl == nullptr) {
        throwValidationError("The constraint was not created by the validation-rule builder"_el);
    }
    if (isNegated) {
        StringEditor prefixedName{"not_"_el};
        prefixedName.append(name);
        name = prefixedName;
    }
    constraintImpl->setName(std::move(name));
    constraintImpl->setNegated(isNegated);
    if (!errorMessage.isEmpty()) {
        constraintImpl->setErrorMessage(std::move(errorMessage));
    }
    addOrOverwriteConstraint(constraintImpl);
}

void Rule::addDependency(
    const vr::DependencyMode mode,
    const std::vector<NamePathLike> &sources,
    const std::vector<NamePathLike> &targets,
    String errorMessage) {
    auto sourcePaths = parseNamePathList(sources);
    auto targetPaths = parseNamePathList(targets);
    addDependencyDefinition(
        DependencyDefinition::create(mode, std::move(sourcePaths), std::move(targetPaths), std::move(errorMessage)));
}

void Rule::addKeyIndex(
    const Name &name, const std::vector<NamePathLike> &keyPaths, const CaseSensitivity caseSensitivity) {
    addKeyDefinition(KeyDefinition::create(name, parseNamePathList(keyPaths), caseSensitivity, {}));
}

void Rule::limitVersions(const std::vector<Integer> &versions, const bool isNegated) {
    if (versions.empty()) {
        throwValidationError("The version list must not be empty"_el);
    }
    auto uniqueVersions = util::List<Integer>{};
    uniqueVersions.reserve(unit::ItemCount{versions.size(), unit::ItemCount::Unchecked{}});
    for (const auto version : versions) {
        if (version < 0) {
            throwValidationError("Versions must be non-negative integers"_el);
        }
        if (std::ranges::find(uniqueVersions, version) == uniqueVersions.end()) {
            uniqueVersions.append(version);
        }
    }
    auto mask = VersionMask::fromIntegers(uniqueVersions);
    limitVersionMask(isNegated ? !mask : mask);
}

void Rule::limitMinimumVersion(const Integer version, const bool isNegated) {
    if (version < 0) {
        throwValidationError("The minimum version must be non-negative"_el);
    }
    auto mask = VersionMask::fromRanges({ConfVersionRange{version, std::numeric_limits<Integer>::max()}});
    limitVersionMask(isNegated ? !mask : mask);
}

void Rule::limitMaximumVersion(const Integer version, const bool isNegated) {
    if (version < 0) {
        throwValidationError("The maximum version must be non-negative"_el);
    }
    auto mask = VersionMask::fromRanges({ConfVersionRange{0, version}});
    limitVersionMask(isNegated ? !mask : mask);
}

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

auto Rule::hasConstraint(const String &name) const -> bool {
    return std::ranges::any_of(
        _constraints, [&](const auto &constraint) -> bool { return constraint->name() == name; });
}

auto Rule::constraint(const String &name) const -> ConstraintPtr {
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

auto Rule::childByDefinitionName(const Name &name) const -> RulePtr {
    return _children.ruleByDefinitionName(name);
}

auto Rule::descendantByDefinitionPath(const NamePath &namePath) const -> RulePtr {
    if (namePath.empty()) {
        return {};
    }
    auto result = childByDefinitionName(namePath.front());
    if (result == nullptr) {
        return {};
    }
    for (std::size_t i = 1; i < namePath.size(); ++i) {
        result = result->childByDefinitionName(namePath.at(i));
        if (result == nullptr) {
            return {};
        }
    }
    return result;
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const Rule &rule) -> InternalViewPtr {
    auto result = InternalView::create();
    result->setValue("definitionNamePath", rule._definitionNamePath.toText());
    result->setValue("valueNamePath", rule._valueNamePath.toText());
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
        result->setValue("defaultValue", "<null>"_el);
    }
    result->setValue("versionMask", rule._versionMask.toText());
    if (rule._parent.expired()) {
        result->setValue("parent", "<null>"_el);
    } else {
        result->setValue("parent", rule._parent.lock()->namePath().toText());
    }
    result->setValue(
        "constraints",
        InternalView::createNamedList(
            rule._constraints.begin(), rule._constraints.end(), [](const ConstraintPtr &constraint) -> String {
                return StringFormat{"Constraint \"{}\""_el}.build(constraint->name());
            }));
    result->setValue(
        "children",
        InternalView::createNamedList(rule._children.begin(), rule._children.end(), [](const RulePtr &child) -> String {
            return StringFormat{"Rule \"{}\""_el}.build(child->definitionNamePath().toText());
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
