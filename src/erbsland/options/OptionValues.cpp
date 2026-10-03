// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionValues.hpp"

#include "Option.hpp"
#include "OptionModule.hpp"

#include "impl/AsciiOptionText.hpp"

#include "../err/ParameterFailure.hpp"
#include "../i18n/DisplayText.hpp"
#include "../text/Literals.hpp"
#include "../text/StringList.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

using namespace unit;
using namespace text;
using namespace literals;

auto OptionValues::create() -> OptionValuesPtr {
    return std::make_shared<OptionValues>();
}

void OptionValues::setValue(const String &name, OptionValuePtr value) {
    if (!Option::isValidOptionName(name) && !Option::isValidPositionalName(name)) {
        throw err::ParameterFailure{i18n::displayText("options.invalid_definition_name"_el), "name"_el};
    }
    _values[impl::normalizeOptionName(name).copy()] = std::move(value);
}

void OptionValues::setValue(std::initializer_list<String> names, OptionValuePtr value) {
    auto canonicalNames = StringList{};
    canonicalNames.reserve(ItemCount{names.size(), ItemCount::Unchecked{}});
    for (const auto &name : names) {
        if (!Option::isValidOptionName(name) && !Option::isValidPositionalName(name)) {
            throw err::ParameterFailure{i18n::displayText("options.invalid_definition_name"_el), "names"_el};
        }
        canonicalNames.append(impl::normalizeOptionName(name));
    }
    for (const auto &name : canonicalNames) {
        _values[name.copy()] = value;
    }
}

void OptionValues::setModuleName(const String &moduleName) {
    if (!moduleName.isEmpty() && !OptionModule::isValidName(moduleName)) {
        throw err::ParameterFailure{i18n::displayText("options.invalid_module_definition_name"_el), "moduleName"_el};
    }
    _moduleName = impl::normalizeOptionName(moduleName);
}

auto OptionValues::valueCount(const String &name) const -> ArgumentCount {
    const auto found = _values.find(impl::normalizeOptionName(name));
    if (found == _values.end() || found->second == nullptr) {
        return ArgumentCount::zero();
    }
    return found->second->valueCount();
}

auto OptionValues::value(const String &name) const -> OptionValuePtr {
    const auto found = _values.find(impl::normalizeOptionName(name));
    if (found == _values.end()) {
        return {};
    }
    return found->second;
}

auto OptionValues::getFlag(const String &name, const bool defaultFlag) const -> bool {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultFlag;
    }
    return optionValue->getFlag(defaultFlag);
}

auto OptionValues::getFlagCount(const String &name, const ArgumentCount defaultCount) const -> ArgumentCount {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultCount;
    }
    return optionValue->flagCount();
}

auto OptionValues::getBoolean(const String &name, const bool defaultBoolean) const -> bool {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultBoolean;
    }
    return optionValue->getBoolean(defaultBoolean);
}

auto OptionValues::getBooleanList(const String &name, std::vector<bool> defaultBooleanList) const -> std::vector<bool> {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultBooleanList;
    }
    return optionValue->getBooleanList(std::move(defaultBooleanList));
}

auto OptionValues::getInteger(const String &name, const OptionInteger defaultInteger) const -> OptionInteger {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultInteger;
    }
    return optionValue->getInteger(defaultInteger);
}

auto OptionValues::getText(const String &name, const String &defaultText) const -> String {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultText;
    }
    return optionValue->getText(defaultText);
}

auto OptionValues::getTextList(const String &name, StringList defaultTextList) const -> StringList {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultTextList;
    }
    return optionValue->getTextList(std::move(defaultTextList));
}

auto OptionValues::getIntegerList(const String &name, std::vector<OptionInteger> defaultIntegerList) const
    -> std::vector<OptionInteger> {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultIntegerList;
    }
    return optionValue->getIntegerList(std::move(defaultIntegerList));
}

}
