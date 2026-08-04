// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/Throw.hpp"

#include <functional>

namespace erbsland::util {

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::count() const noexcept -> Count {
    return Count::fromSizeT(raw().size());
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::countIf(Function function) const -> Count {
    auto result = std::size_t{0};
    for (const auto &value : raw()) {
        if (std::invoke(function, value)) {
            ++result;
        }
    }
    return Count::fromSizeT(result);
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::get(const Index index) const -> Element {
    return get(index, Element{});
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::get(const Index index, const Element &defaultValue) const -> Element {
    const auto &data = raw();
    if (!validIndex(index, data.size())) {
        return defaultValue;
    }
    return data[index.toSizeT()];
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::getRef(const Index index) const -> const Element & {
    const auto &data = raw();
    if (!validIndex(index, data.size())) {
        static const auto cDefaultElement = Element{};
        return cDefaultElement;
    }
    return data[index.toSizeT()];
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::getRefOrThrow(const Index index) const -> const Element & {
    const auto &data = raw();
    if (!validIndex(index, data.size())) {
        impl::throwOutOfRange("List index out of range.");
    }
    return data[index.toSizeT()];
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::first() const -> Element {
    const auto &data = raw();
    return data.empty() ? Element{} : data.front();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::last() const -> Element {
    const auto &data = raw();
    return data.empty() ? Element{} : data.back();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::set(const Index index, const Element &value) -> Self & {
    auto &data = mutableRaw();
    if (validIndex(index, data.size())) {
        data[index.toSizeT()] = value;
    }
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::set(const Index index, Element &&value) -> Self & {
    auto &data = mutableRaw();
    if (validIndex(index, data.size())) {
        data[index.toSizeT()] = std::move(value);
    }
    return self();
}

}
