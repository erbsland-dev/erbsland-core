// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyIndexData.hpp"

#include <unordered_set>
#include <vector>

namespace erbsland::conf::impl {

/// Stores keys with multiple elements and tracks the values per element index.
template <typename tKeyHash, typename tKeyEqual, typename tKeyElementHash, typename tKeyElementEqual>
class KeyIndexDataMultiple final : public KeyIndexData {
public:
    /// Create storage for keys with a fixed element count.
    /// @param elementCount The number of elements in every key.
    explicit KeyIndexDataMultiple(const std::size_t elementCount) : _keysByElement(elementCount) {}

    [[nodiscard]] auto hasKey(const ConfKey &key) const noexcept -> bool override { return _keys.contains(key); }

    [[nodiscard]] auto hasKeyElement(const text::String &element, const std::size_t index) const noexcept
        -> bool override {
        return index < _keysByElement.size() && _keysByElement[index].contains(element);
    }

    [[nodiscard]] auto tryAddKey(const ConfKey &key) -> bool override {
        if (!_keys.insert(key).second) {
            return false;
        }
        for (std::size_t index = 0; index < _keysByElement.size(); ++index) {
            _keysByElement[index].insert(key.element(index));
        }
        return true;
    }

private:
    std::unordered_set<ConfKey, tKeyHash, tKeyEqual> _keys;
    std::vector<std::unordered_set<text::String, tKeyElementHash, tKeyElementEqual>> _keysByElement;
};

}
