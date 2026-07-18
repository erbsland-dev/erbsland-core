// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionValues.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

using text::String;

auto OptionValues::create() -> OptionValuesPtr {
    return std::make_shared<OptionValues>();
}

void OptionValues::setValue(const String &name, OptionValuePtr value) {
    _values[name.copy()] = std::move(value);
}

void OptionValues::setValue(std::initializer_list<String> names, OptionValuePtr value) {
    for (const auto &name : names) {
        _values[name.copy()] = value;
    }
}

auto OptionValues::valueCount(const String &name) const -> unit::ArgumentCount {
    const auto found = _values.find(name);
    if (found == _values.end() || found->second == nullptr) {
        return unit::ArgumentCount::zero();
    }
    return found->second->valueCount();
}

auto OptionValues::value(const String &name) const -> OptionValuePtr {
    const auto found = _values.find(name);
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

auto OptionValues::getFlagCount(const String &name, const unit::ArgumentCount defaultCount) const
    -> unit::ArgumentCount {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultCount;
    }
    return optionValue->flagCount();
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

auto OptionValues::getTextList(const String &name, std::vector<String> defaultTextList) const -> std::vector<String> {
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
