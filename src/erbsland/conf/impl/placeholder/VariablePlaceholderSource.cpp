// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VariablePlaceholderSource.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../ConfError.hpp"
#include "../../Name.hpp"

namespace erbsland::conf::impl::placeholder {

using namespace text::literals;

VariablePlaceholderSource::VariablePlaceholderSource(text::StringMap<text::String> variables) :
    _variables{normalizedVariables(std::move(variables))} {
}

auto VariablePlaceholderSource::sourceNames() const -> text::StringList {
    return text::StringList{"var"_el};
}

auto VariablePlaceholderSource::resolve(const text::String &, const text::String &parameter) -> text::String {
    const auto name = Name::normalize(parameter);
    if (const auto value = _variables.get(name); value.has_value()) {
        return *value;
    }
    throw ConfError{
        ConfErrorCategory::ValueNotFound, text::StringFormat{"Placeholder variable is not defined: {}"_el}.build(name)};
}

void VariablePlaceholderSource::setVariables(text::StringMap<text::String> variables) {
    _variables = normalizedVariables(std::move(variables));
}

auto VariablePlaceholderSource::normalizedVariables(text::StringMap<text::String> variables)
    -> text::StringMap<text::String> {
    auto result = text::StringMap<text::String>{};
    for (const auto &[name, value] : variables) {
        const auto normalizedName = Name::normalize(name);
        if (result.contains(normalizedName)) {
            throw ConfError{
                ConfErrorCategory::NameConflict,
                text::StringFormat{"Placeholder variable name is defined more than once: {}"_el}.build(normalizedName)};
        }
        result.set(normalizedName, value);
    }
    return result;
}

}
