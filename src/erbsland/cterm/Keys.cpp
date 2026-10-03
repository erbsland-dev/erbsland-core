// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Keys.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::cterm {

using namespace unit;
using namespace text;
using namespace text::literals;

Keys::Keys(Key key) {
    add(std::move(key));
}

Keys::Keys(const Key::Type keyType) {
    add(keyType);
}

Keys::Keys(const Char character) {
    add(character);
}

Keys::Keys(const std::initializer_list<Key> keys) {
    setKeys(keys);
}

Keys::Keys(std::vector<Key> keys) {
    setKeys(std::move(keys));
}

auto Keys::mainKeyCount() const noexcept -> MainCount {
    return std::min(_mainKeyCount, _keys.size());
}

auto Keys::mainKeys() const -> std::vector<Key> {
    const auto count = mainKeyCount();
    return {_keys.begin(), _keys.begin() + static_cast<Container::difference_type>(count)};
}

auto Keys::alternativeKeys() const -> std::vector<Key> {
    const auto count = mainKeyCount();
    return {_keys.begin() + static_cast<Container::difference_type>(count), _keys.end()};
}

auto Keys::mainKeyLabels() const -> StringList {
    auto keyLabels = StringList{};
    const auto keys = mainKeys();
    keyLabels.reserve(ItemCount::fromSizeT(keys.size()));
    for (const auto &key : keys) {
        keyLabels.append(key.toDisplayText(false));
    }
    return keyLabels;
}

auto Keys::setKeys(std::vector<Key> keys) -> Keys & {
    clear();
    for (auto &key : keys) {
        add(std::move(key));
    }
    return *this;
}

auto Keys::setKeys(const std::initializer_list<Key> keys) -> Keys & {
    clear();
    for (const auto &key : keys) {
        add(key);
    }
    return *this;
}

auto Keys::add(Key key) -> Keys & {
    validateKey(key);
    if (!contains(key)) {
        _keys.emplace_back(std::move(key));
    }
    return *this;
}

auto Keys::add(const Key::Type keyType) -> Keys & {
    return add(keyFromType(keyType));
}

auto Keys::add(const Char character) -> Keys & {
    return add(Key{character});
}

auto Keys::clear() noexcept -> Keys & {
    _keys.clear();
    _mainKeyCount = allKeysAreMain();
    return *this;
}

auto Keys::setMainKeyCount(const MainCount mainKeyCount) noexcept -> Keys & {
    _mainKeyCount = mainKeyCount;
    return *this;
}

auto Keys::contains(const Key &key) const noexcept -> bool {
    return std::ranges::find(_keys, key) != _keys.end();
}

void Keys::validateKey(const Key &key) {
    if (!key.valid()) {
        throw err::ParameterFailure{"Key binding must be a displayable key."_el, "key"_el};
    }
    const auto displayText = key.toDisplayText(false);
    if (displayText.isEmpty()) {
        throw err::ParameterFailure{"Key binding must be a displayable key."_el, "key"_el};
    }
}

auto Keys::keyFromType(const Key::Type keyType) -> Key {
    switch (keyType) {
    case Key::None:
    case Key::Character:
    case Key::Combined:
        throw err::ParameterFailure{"Key binding type must be one special key type."_el, "keyType"_el};
    default:
        return Key{keyType};
    }
}

}
