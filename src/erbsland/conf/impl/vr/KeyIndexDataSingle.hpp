// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyIndexData.hpp"

#include <unordered_set>

namespace erbsland::conf::impl {

/// Stores keys with one element.
template <typename tKeyHash, typename tKeyEqual>
class KeyIndexDataSingle final : public KeyIndexData {
public:
    [[nodiscard]] auto hasKey(const ConfKey &key) const noexcept -> bool override { return _keys.contains(key); }

    [[nodiscard]] auto hasKeyElement(const text::String &element, const std::size_t index) const noexcept
        -> bool override {
        return index == 0 && hasKey(ConfKey{element});
    }

    [[nodiscard]] auto tryAddKey(const ConfKey &key) -> bool override { return _keys.insert(key).second; }

private:
    std::unordered_set<ConfKey, tKeyHash, tKeyEqual> _keys;
};

}
