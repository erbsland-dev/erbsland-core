// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VariableSource.hpp"

#include "ReplacerError.hpp"

#include "impl/Name.hpp"

#include "../Literals.hpp"
#include "../StringFormat.hpp"

namespace erbsland::text::placeholder {

using namespace literals;

VariableSource::VariableSource(StringMap<String> variables, const String &name) :
    _variables{normalizedVariables(std::move(variables))}, _name{name.isEmpty() ? "var"_el : name} {
}

auto VariableSource::sourceNames() const -> StringList {
    return StringList{_name};
}

auto VariableSource::validate(const String &, const String &parameter) -> bool {
    try {
        [[maybe_unused]] const auto normalized = impl::normalizeName(parameter);
        return true;
    } catch (const ReplacerError &) {
        return false;
    }
}

auto VariableSource::resolve(const String &, const String &parameter) -> String {
    const auto name = impl::normalizeName(parameter);
    if (const auto value = _variables.get(name); value.has_value()) {
        return *value;
    }
    throw ReplacerError{
        ReplacerErrorCategory::ValueNotFound, StringFormat{"Placeholder variable is not defined: {}"_el}.build(name)};
}

void VariableSource::setVariables(StringMap<String> variables) {
    _variables = normalizedVariables(std::move(variables));
}

auto VariableSource::normalizedVariables(StringMap<String> variables) -> StringMap<String> {
    auto result = StringMap<String>{};
    for (const auto &[name, value] : variables) {
        const auto normalizedName = impl::normalizeName(name);
        if (result.contains(normalizedName)) {
            throw ReplacerError{
                ReplacerErrorCategory::NameConflict,
                StringFormat{"Placeholder variable name is defined more than once: {}"_el}.build(normalizedName)};
        }
        result.set(normalizedName, value);
    }
    return result;
}

}
