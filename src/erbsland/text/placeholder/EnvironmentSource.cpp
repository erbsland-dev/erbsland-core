// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EnvironmentSource.hpp"

#include "ReplacerError.hpp"

#include "../Char.hpp"
#include "../CharSet.hpp"
#include "../Literals.hpp"
#include "../StringFormat.hpp"

#include "../../system/EnvironmentVariables.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::text::placeholder {

using namespace literals;

EnvironmentSource::EnvironmentSource(const String &name) : _name{name.isEmpty() ? "env"_el : name} {
}

auto EnvironmentSource::sourceNames() const -> StringList {
    return StringList{_name};
}

auto EnvironmentSource::validate(const String &, const String &parameter) -> bool {
    try {
        [[maybe_unused]] const auto parsed = parseParameters(parameter);
        return true;
    } catch (const ReplacerError &) {
        return false;
    }
}

auto EnvironmentSource::resolve(const String &, const String &parameter) -> String {
    const auto parameters = parseParameters(parameter);
    const auto value = system::EnvironmentVariables{}.get(parameters.variableName);
    if (!value.has_value()) {
        if (parameters.isRequired) {
            throw ReplacerError{
                ReplacerErrorCategory::ValueNotFound,
                StringFormat{"Required environment variable is not set: {}"_el}.build(parameters.variableName)};
        }
        return "undefined"_el;
    }
    if (parameters.isUnsafeRaw) {
        return *value;
    }
    return value->transformed([](const Char character) noexcept -> Char {
        if (character == U'\t' || character == U'\n') {
            return character;
        }
        return character.isControlOrFormat() ? Char::noCodePoint() : character;
    });
}

auto EnvironmentSource::parseParameters(const String &parameter) -> Parameters {
    static const auto separators = CharSet{","_el};
    auto result = Parameters{};
    auto isFirst = true;
    const auto entries = StringList::fromSplit(parameter, separators, unit::ItemCount::infinite(), true);
    for (const auto &entry : entries) {
        if (isFirst) {
            if (entry.isEmpty()) {
                throw ReplacerError{ReplacerErrorCategory::Syntax, "The 'env' source requires a variable name."_el};
            }
            result.variableName = entry;
            isFirst = false;
        } else {
            applyFlag(result, entry);
        }
    }
    return result;
}

void EnvironmentSource::applyFlag(Parameters &parameters, const String &flag) {
    const auto normalizedFlag = flag.transformed(Char::toIdentifierNormalized);
    if (normalizedFlag == "required"_el && !parameters.isRequired) {
        parameters.isRequired = true;
        return;
    }
    if (normalizedFlag == "unsafe_raw"_el && !parameters.isUnsafeRaw) {
        parameters.isUnsafeRaw = true;
        return;
    }
    throw ReplacerError{
        ReplacerErrorCategory::Syntax, StringFormat{"Unknown or duplicate 'env' source flag: {}"_el}.build(flag)};
}

}
