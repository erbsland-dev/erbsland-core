// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EnvironmentChanges.hpp"

#include "impl/environment/EnvironmentChangesProcessor.hpp"
#include "impl/environment/EnvironmentPaths.hpp"
#include "impl/environment/EnvironmentVariableBackend.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/CharSet.hpp"
#include "../text/Literals.hpp"
#include "../unit/ItemIndex.hpp"

namespace erbsland::system {

using namespace path;
using namespace text;
using namespace text::literals;

void EnvironmentChanges::validateName(const String &name) {
    if (name.isEmpty() || !name.isEncodingValid() || name.containsOneOf(CharSet{Char{U'='}, Char{U'\0'}})) {
        throw err::ParameterFailure{
            "Environment names must be non-empty valid UTF-8 without '=' or null characters."_el, "name"_el};
    }
}

void EnvironmentChanges::validateValue(const String &value) {
    if (!value.isEncodingValid() || value.containsOneOf(CharSet{U'\0'})) {
        throw err::ParameterFailure{"Environment values must be valid UTF-8 without null characters."_el, "value"_el};
    }
}

void EnvironmentChanges::validatePath(const Path &path) {
    impl::EnvironmentPaths::validate(path, impl::EnvironmentVariableBackend::native());
}

auto EnvironmentChanges::setInherit(const EnvironmentInherit inherit) -> EnvironmentChanges & {
    if (inherit != EnvironmentInherit::All && inherit != EnvironmentInherit::None) {
        throw err::ParameterFailure{"Invalid environment inheritance mode."_el, "inherit"_el};
    }
    _inherit = inherit;
    _inheritedNames.reset();
    return *this;
}

auto EnvironmentChanges::setInherit(StringList names) -> EnvironmentChanges & {
    for (const auto &name : names) {
        validateName(name);
    }
    _inheritedNames = std::move(names);
    return *this;
}

auto EnvironmentChanges::hasPathChanges() const noexcept -> bool {
    return !_frontPaths.isEmpty() || !_backPaths.isEmpty() || !_removedPaths.isEmpty() || !_removedBases.isEmpty();
}

auto EnvironmentChanges::isUnchangedInheritance() const noexcept -> bool {
    return _inherit == EnvironmentInherit::All && !_inheritedNames.has_value() && _variables.isEmpty() &&
        !hasPathChanges();
}

auto EnvironmentChanges::setVariable(const String &name, String value) -> EnvironmentChanges & {
    validateName(name);
    validateValue(value);
    eraseVariableChange(name);
    _variables.append({name, std::move(value)});
    return *this;
}

auto EnvironmentChanges::removeVariable(const String &name) -> EnvironmentChanges & {
    validateName(name);
    eraseVariableChange(name);
    _variables.append({name, std::nullopt});
    return *this;
}

auto EnvironmentChanges::removeVariable(const StringList &names) -> EnvironmentChanges & {
    for (const auto &name : names) {
        validateName(name);
    }
    for (const auto &name : names) {
        eraseVariableChange(name);
        _variables.append({name, std::nullopt});
    }
    return *this;
}

auto EnvironmentChanges::clearVariableChange(const String &name) -> EnvironmentChanges & {
    validateName(name);
    eraseVariableChange(name);
    return *this;
}

void EnvironmentChanges::eraseVariableChange(const String &name) {
    const auto &backend = impl::EnvironmentVariableBackend::native();
    for (auto index = unit::ItemIndex::zero(); index.isWithin(_variables.count()); ++index) {
        if (backend.compare(_variables.getRef(index).first, name) == 0) {
            _variables.remove(index);
            break;
        }
    }
}

auto EnvironmentChanges::clearVariableChanges() noexcept -> EnvironmentChanges & {
    _variables.clear();
    return *this;
}

auto EnvironmentChanges::prependPath(Path path) -> EnvironmentChanges & {
    validatePath(path);
    _frontPaths.insert(unit::ItemIndex::zero(), std::move(path));
    return *this;
}

auto EnvironmentChanges::appendPath(Path path) -> EnvironmentChanges & {
    validatePath(path);
    _backPaths.append(std::move(path));
    return *this;
}

auto EnvironmentChanges::removePath(Path path) -> EnvironmentChanges & {
    validatePath(path);
    _removedPaths.append(std::move(path));
    return *this;
}

auto EnvironmentChanges::removePathsIn(Path base) -> EnvironmentChanges & {
    validatePath(base);
    _removedBases.append(std::move(base));
    return *this;
}

auto EnvironmentChanges::clearPathChanges() noexcept -> EnvironmentChanges & {
    _frontPaths.clear();
    _backPaths.clear();
    _removedPaths.clear();
    _removedBases.clear();
    return *this;
}

auto EnvironmentChanges::apply(const StringMap<String> &base) const -> StringMap<String> {
    using Processor = impl::EnvironmentChangesProcessor;
    auto nativeBase =
        Processor::Environment<String>{Processor::NameLess<String>{impl::EnvironmentVariableBackend::native()}};
    for (const auto &[name, value] : base) {
        nativeBase.insert_or_assign(name, value);
    }
    auto result = StringMap<String>{};
    for (const auto &[name, value] : Processor::apply(*this, nativeBase)) {
        result.set(name, value);
    }
    return result;
}

}
