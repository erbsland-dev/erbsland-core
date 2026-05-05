// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionValues.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

auto OptionValues::create() -> OptionValuesPtr {
    return std::make_shared<OptionValues>();
}

void OptionValues::setValue(const text::StringView &name, OptionValuePtr value) {
    _values[name.copy()] = std::move(value);
}

void OptionValues::setValue(std::initializer_list<text::StringView> names, OptionValuePtr value) {
    for (const auto &name : names) {
        _values[name.copy()] = value;
    }
}

auto OptionValues::valueCount(const text::StringView &name) const -> unit::ArgumentCount {
    const auto found = _values.find(name.copy());
    if (found == _values.end() || found->second == nullptr) {
        return unit::ArgumentCount::zero();
    }
    return found->second->valueCount();
}

auto OptionValues::value(const text::StringView &name) const -> OptionValuePtr {
    const auto found = _values.find(name.copy());
    if (found == _values.end()) {
        return {};
    }
    return found->second;
}

auto OptionValues::getFlag(const text::StringView &name, const bool defaultFlag) const -> bool {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultFlag;
    }
    return optionValue->getFlag(defaultFlag);
}

auto OptionValues::getFlagCount(const text::StringView &name, const unit::ArgumentCount defaultCount) const
    -> unit::ArgumentCount {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultCount;
    }
    return optionValue->flagCount();
}

auto OptionValues::getInteger(const text::StringView &name, const OptionInteger defaultInteger) const -> OptionInteger {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultInteger;
    }
    return optionValue->getInteger(defaultInteger);
}

auto OptionValues::getText(const text::StringView &name, const text::StringView &defaultText) const
    -> text::StringView {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultText;
    }
    return optionValue->getText(defaultText);
}

auto OptionValues::getTextList(const text::StringView &name, std::vector<text::StringView> defaultTextList) const
    -> std::vector<text::StringView> {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultTextList;
    }
    return optionValue->getTextList(std::move(defaultTextList));
}

auto OptionValues::getIntegerList(const text::StringView &name, std::vector<OptionInteger> defaultIntegerList) const
    -> std::vector<OptionInteger> {
    const auto optionValue = value(name);
    if (optionValue == nullptr) {
        return defaultIntegerList;
    }
    return optionValue->getIntegerList(std::move(defaultIntegerList));
}

}
