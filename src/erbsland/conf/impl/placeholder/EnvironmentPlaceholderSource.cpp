// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EnvironmentPlaceholderSource.hpp"

#include "../../../system/EnvironmentVariables.hpp"
#include "../../../text/Char.hpp"
#include "../../../text/CharSet.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../unit/ItemCount.hpp"
#include "../../ConfError.hpp"

namespace erbsland::conf::impl::placeholder {

using namespace text::literals;

auto EnvironmentPlaceholderSource::sourceNames() const -> text::StringList {
    return text::StringList{"env"_el};
}

auto EnvironmentPlaceholderSource::resolve(const text::String &, const text::String &parameter) -> text::String {
    const auto parameters = parseParameters(parameter);
    const auto value = system::EnvironmentVariables{}.get(parameters.variableName);
    if (!value.has_value()) {
        if (parameters.isRequired) {
            throw ConfError{
                ConfErrorCategory::ValueNotFound,
                text::StringFormat{"Required environment variable is not set: {}"_el}.build(parameters.variableName)};
        }
        return "undefined"_el;
    }
    if (parameters.isUnsafeRaw) {
        return *value;
    }
    return value->transformed([](const text::Char character) noexcept -> text::Char {
        if (character == U'\t' || character == U'\n') {
            return character;
        }
        return character.isControlOrFormat() ? text::Char::noCodePoint() : character;
    });
}

auto EnvironmentPlaceholderSource::parseParameters(const text::String &parameter) -> Parameters {
    static const auto separators = text::CharSet{","_el};
    auto result = Parameters{};
    auto isFirst = true;
    const auto entries = text::StringList::fromSplit(parameter, separators, unit::ItemCount::infinite(), true);
    for (const auto &entry : entries) {
        if (isFirst) {
            if (entry.isEmpty()) {
                throw ConfError{ConfErrorCategory::Syntax, "The 'env' source requires a variable name."_el};
            }
            result.variableName = entry;
            isFirst = false;
        } else {
            applyFlag(result, entry);
        }
    }
    return result;
}

void EnvironmentPlaceholderSource::applyFlag(Parameters &parameters, const text::String &flag) {
    const auto normalizedFlag = flag.transformed(text::Char::toIdentifierNormalized);
    if (normalizedFlag == "required"_el && !parameters.isRequired) {
        parameters.isRequired = true;
        return;
    }
    if (normalizedFlag == "unsafe_raw"_el && !parameters.isUnsafeRaw) {
        parameters.isUnsafeRaw = true;
        return;
    }
    throw ConfError{
        ConfErrorCategory::Syntax, text::StringFormat{"Unknown or duplicate 'env' source flag: {}"_el}.build(flag)};
}

}
