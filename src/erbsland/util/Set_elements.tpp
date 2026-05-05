// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::count() const noexcept -> Count {
    return Count::fromSizeT(raw().size());
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto Set<tKey, tCompare, tSelf>::countIf(Function function) const -> Count {
    auto result = std::size_t{0};
    for (const auto &key : raw()) {
        if (std::invoke(function, key)) {
            ++result;
        }
    }
    return Count::fromSizeT(result);
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::first() const -> Key {
    const auto &data = raw();
    if (data.empty()) {
        return {};
    }
    return *data.begin();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::last() const -> Key {
    const auto &data = raw();
    if (data.empty()) {
        return {};
    }
    return *data.rbegin();
}

}
