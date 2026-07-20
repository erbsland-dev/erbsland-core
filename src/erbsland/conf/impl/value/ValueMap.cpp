// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueMap.hpp"

#include "Value.hpp"

#include <numeric>

namespace erbsland::conf::impl {

ValueMap::ValueMap(List &&valueList) : _valueList(std::move(valueList)) {
    std::size_t index = 0;
    for (const auto &value : _valueList) {
        value->setName(Name::createIndex(index));
        _valueMap.insert_or_assign(value->name(), value);
        index += 1;
    }
}

auto ValueMap::empty() const noexcept -> bool {
    return _valueList.empty();
}

auto ValueMap::size() const noexcept -> std::size_t {
    return _valueList.size();
}

auto ValueMap::hasValue(const NamePathLike &namePathLike) const noexcept -> bool {
    try {
        if (const auto *text = std::get_if<text::String>(&namePathLike)) {
            return hasValueImpl(NamePath::fromText(*text));
        }
        if (const auto *namePath = std::get_if<NamePath>(&namePathLike)) {
            return hasValueImpl(*namePath);
        }
        if (const auto *name = std::get_if<Name>(&namePathLike)) {
            return hasValueImpl(*name);
        }
        return hasValueImpl(std::get<std::size_t>(namePathLike));
    } catch (const ConfError &) {
        return false;
    }
}

void ValueMap::setParent(const conf::ValuePtr &parent) {
    for (const auto &value : _valueList) {
        value->setParent(parent);
    }
}

auto ValueMap::hasValueImpl(const NamePath &namePath) const noexcept -> bool {
    if (namePath.empty()) {
        return false;
    }
    const auto result = std::accumulate(
        std::next(namePath.begin()),
        namePath.end(),
        valueImpl(namePath.front()),
        [&](const ValuePtr &currentValue, auto const &nextName) {
            return currentValue ? currentValue->valueImpl(nextName) : nullptr;
        });
    return result != nullptr;
}

auto ValueMap::hasValueImpl(const Name &name) const noexcept -> bool {
    return _valueMap.contains(name);
}

auto ValueMap::hasValueImpl(std::size_t index) const noexcept -> bool {
    return index < _valueList.size();
}

auto ValueMap::value(const NamePathLike &namePathLike) const -> ValuePtr {
    try {
        if (const auto *text = std::get_if<text::String>(&namePathLike)) {
            return valueImpl(NamePath::fromText(*text));
        }
        if (const auto *namePath = std::get_if<NamePath>(&namePathLike)) {
            return valueImpl(*namePath);
        }
        if (const auto *name = std::get_if<Name>(&namePathLike)) {
            return valueImpl(*name);
        }
        return valueImpl(std::get<std::size_t>(namePathLike));
    } catch (const ConfError &) {
        return {};
    }
}

auto ValueMap::valueOrThrow(const NamePathLike &namePathLike, const conf::Value &value) const -> ValuePtr {
    ValuePtr result;
    if (const auto *text = std::get_if<text::String>(&namePathLike)) {
        result = valueImpl(NamePath::fromText(*text));
    } else if (const auto *namePath = std::get_if<NamePath>(&namePathLike)) {
        result = valueImpl(*namePath);
    } else if (const auto *name = std::get_if<Name>(&namePathLike)) {
        result = valueImpl(*name);
    } else {
        return valueImpl(std::get<std::size_t>(namePathLike));
    }
    if (result == nullptr) {
        Value::throwValueNotFound(value, namePathLike);
    }
    return result;
}

auto ValueMap::valueImpl(const NamePath &namePath) const -> ValuePtr {
    if (namePath.empty()) {
        return {};
    }
    const auto result = std::accumulate(
        std::next(namePath.begin()),
        namePath.end(),
        valueImpl(namePath.front()),
        [&](const ValuePtr &currentValue, auto const &nextName) {
            return currentValue ? currentValue->valueImpl(nextName) : nullptr;
        });
    return result;
}

auto ValueMap::valueImpl(const Name &name) const -> ValuePtr {
    if (name.isTextIndex()) {
        const auto index = name.asIndex();
        if (!_textIndexesAllowed || index >= _valueList.size()) {
            return {};
        }
        return _valueList[index];
    }
    auto it = _valueMap.find(name);
    if (it == _valueMap.end()) {
        return {};
    }
    return it->second;
}

auto ValueMap::valueImpl(std::size_t index) const -> ValuePtr {
    if (index >= _valueList.size()) {
        return {};
    }
    return _valueList[index];
}

auto ValueMap::begin() const noexcept -> ValueIterator {
    if (_valueMap.empty()) {
        return {};
    }
    return ValueIterator{_valueList.begin()};
}

auto ValueMap::end() const noexcept -> ValueIterator {
    if (_valueMap.empty()) {
        return {};
    }
    return ValueIterator{_valueList.end()};
}

void ValueMap::addValue(const ValuePtr &value) {
    if (value->name().empty()) {
        value->setName(Name::createIndex(_valueList.size()));
    }
    _valueList.push_back(value);
    _valueMap.insert_or_assign(value->name(), value);
}

void ValueMap::removeDefaultValues() {
    std::erase_if(_valueList, [](const auto &childValue) -> bool { return childValue->isDefaultValue(); });

    _valueMap.clear();
    _valueMap.reserve(_valueList.size());
    for (const auto &childValue : _valueList) {
        _valueMap.insert_or_assign(childValue->name(), childValue);
    }
}

}
