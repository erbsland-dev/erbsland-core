// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::findFirst(const Element &value) const -> Index {
    return findFirst(value, Index::zero());
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::findFirst(const Element &value, const Index start) const -> Index {
    return findFirstIf([&](const Element &entry) -> bool { return entry == value; }, start);
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::findFirstIf(Function function) const -> Index {
    return findFirstIf(function, Index::zero());
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::findFirstIf(Function function, const Index start) const -> Index {
    const auto &data = raw();
    if (start.isNoIndex() || start.toSizeT() >= data.size()) {
        return Index::noIndex();
    }
    for (auto position = start.toSizeT(); position < data.size(); ++position) {
        if (std::invoke(function, data[position])) {
            return Index::fromSizeT(position);
        }
    }
    return Index::noIndex();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::findLast(const Element &value) const -> Index {
    const auto &data = raw();
    if (data.empty()) {
        return Index::noIndex();
    }
    return findLast(value, Index::fromSizeT(data.size() - 1U));
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::findLast(const Element &value, const Index start) const -> Index {
    return findLastIf([&](const Element &entry) -> bool { return entry == value; }, start);
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::findLastIf(Function function) const -> Index {
    const auto &data = raw();
    if (data.empty()) {
        return Index::noIndex();
    }
    return findLastIf(function, Index::fromSizeT(data.size() - 1U));
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::findLastIf(Function function, const Index start) const -> Index {
    const auto &data = raw();
    if (start.isNoIndex() || start.toSizeT() >= data.size()) {
        return Index::noIndex();
    }
    for (auto position = start.toSizeT() + 1U; position > 0U; --position) {
        const auto index = position - 1U;
        if (std::invoke(function, data[index])) {
            return Index::fromSizeT(index);
        }
    }
    return Index::noIndex();
}

}
