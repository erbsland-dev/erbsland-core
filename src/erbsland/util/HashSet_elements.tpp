// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::count() const noexcept -> Count {
    return Count::fromSizeT(raw().size());
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto HashSet<tKey, tHash, tEqual, tSelf>::countIf(Function function) const -> Count {
    auto result = std::size_t{0};
    for (const auto &key : raw()) {
        if (std::invoke(function, key)) {
            ++result;
        }
    }
    return Count::fromSizeT(result);
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::first() const -> Key {
    const auto &data = raw();
    if (data.empty()) {
        return {};
    }
    return *data.begin();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::last() const -> Key {
    const auto &data = raw();
    if (data.empty()) {
        return {};
    }
    auto iterator = data.begin();
    auto lastIterator = iterator;
    while (iterator != data.end()) {
        lastIterator = iterator;
        ++iterator;
    }
    return *lastIterator;
}

}
