// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionValue.hpp"

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace erbsland::options {

using unit::ArgumentCount;
using unit::ArgumentIndex;

OptionValue::OptionValue(OptionValueStorage storage) : _storage{std::move(storage)} {
}

OptionValue::OptionValue(OptionValueStorage storage, std::vector<ArgumentIndex> argumentIndexes) :
    _storage{std::move(storage)}, _argumentIndexes{std::move(argumentIndexes)} {
}

OptionValue::OptionValue(OptionWeakPtr option, OptionValueStorage storage) :
    _option{std::move(option)}, _storage{std::move(storage)} {
}

OptionValue::OptionValue(OptionWeakPtr option, OptionValueStorage storage, std::vector<ArgumentIndex> argumentIndexes) :
    _option{std::move(option)}, _storage{std::move(storage)}, _argumentIndexes{std::move(argumentIndexes)} {
}

auto OptionValue::create(OptionValueStorage storage) -> OptionValuePtr {
    return std::make_shared<OptionValue>(std::move(storage));
}

auto OptionValue::create(OptionValueStorage storage, std::vector<ArgumentIndex> argumentIndexes) -> OptionValuePtr {
    return std::make_shared<OptionValue>(std::move(storage), std::move(argumentIndexes));
}

auto OptionValue::create(OptionWeakPtr option, OptionValueStorage storage) -> OptionValuePtr {
    return std::make_shared<OptionValue>(std::move(option), std::move(storage));
}

auto OptionValue::create(OptionWeakPtr option, OptionValueStorage storage, std::vector<ArgumentIndex> argumentIndexes)
    -> OptionValuePtr {
    return std::make_shared<OptionValue>(std::move(option), std::move(storage), std::move(argumentIndexes));
}

auto OptionValue::argumentIndex() const noexcept -> ArgumentIndex {
    if (_argumentIndexes.empty()) {
        return ArgumentIndex::noIndex();
    }
    return _argumentIndexes.front();
}

auto OptionValue::flagCount() const noexcept -> ArgumentCount {
    if (std::holds_alternative<bool>(_storage)) {
        return ArgumentCount::fromSizeT(_argumentIndexes.size());
    }
    return ArgumentCount::zero();
}

auto OptionValue::valueCount() const noexcept -> ArgumentCount {
    return std::visit(
        [](const auto &value) -> ArgumentCount {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (
                std::is_same_v<Value, std::vector<OptionInteger>> || std::is_same_v<Value, std::vector<text::String>>) {
                return ArgumentCount::fromSizeT(value.size());
            } else {
                return ArgumentCount::one();
            }
        },
        _storage);
}

auto OptionValue::type() const noexcept -> OptionValueType {
    return std::visit(
        [](const auto &value) -> OptionValueType {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, bool>) {
                return OptionValueType::Flag;
            } else if constexpr (std::is_same_v<Value, OptionInteger>) {
                return OptionValueType::Integer;
            } else if constexpr (std::is_same_v<Value, std::vector<OptionInteger>>) {
                return OptionValueType::IntegerList;
            } else if constexpr (std::is_same_v<Value, text::String>) {
                return OptionValueType::Text;
            } else {
                return OptionValueType::TextList;
            }
        },
        _storage);
}

auto OptionValue::getFlag(const bool defaultFlag) const -> bool {
    if (const auto flag = std::get_if<bool>(&_storage)) {
        return *flag;
    }
    return defaultFlag;
}

auto OptionValue::getInteger(const OptionInteger defaultInteger) const -> OptionInteger {
    if (const auto integer = std::get_if<OptionInteger>(&_storage)) {
        return *integer;
    }
    return defaultInteger;
}

auto OptionValue::getText(const text::String &defaultText) const -> text::String {
    if (const auto text = std::get_if<text::String>(&_storage)) {
        return *text;
    }
    return defaultText;
}

auto OptionValue::getTextList(std::vector<text::String> defaultTextList) const -> std::vector<text::String> {
    if (const auto textList = std::get_if<std::vector<text::String>>(&_storage)) {
        auto result = std::vector<text::String>{};
        result.reserve(textList->size());
        for (const auto &text : *textList) {
            result.emplace_back(text);
        }
        return result;
    }
    if (const auto text = std::get_if<text::String>(&_storage)) {
        return {*text};
    }
    return defaultTextList;
}

auto OptionValue::getIntegerList(std::vector<OptionInteger> defaultIntegerList) const -> std::vector<OptionInteger> {
    if (const auto integerList = std::get_if<std::vector<OptionInteger>>(&_storage)) {
        return *integerList;
    }
    if (const auto integer = std::get_if<OptionInteger>(&_storage)) {
        return {*integer};
    }
    return defaultIntegerList;
}

}
