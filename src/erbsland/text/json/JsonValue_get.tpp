// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text::json {

template <typename T>
    requires impl::JsonNativeType<T>
auto JsonValue::get() const -> std::optional<T> {
    if constexpr (std::same_as<T, bool>) {
        return getBool();
    } else if constexpr (std::same_as<T, int64_t>) {
        return getInteger();
    } else if constexpr (std::same_as<T, double>) {
        return getNumber();
    } else if constexpr (std::same_as<T, String>) {
        return getText();
    } else if constexpr (std::same_as<T, JsonArray>) {
        return getArray();
    } else if constexpr (std::same_as<T, JsonObject>) {
        return getObject();
    }
}

template <typename T>
    requires impl::JsonNativeType<T>
auto JsonValue::get(T fallback) const -> T {
    const auto value = get<T>();
    return value.has_value() ? *value : std::move(fallback);
}

template <typename T>
    requires impl::JsonNativeType<T>
auto JsonValue::getOrThrow() const -> T {
    const auto value = get<T>();
    if (!value.has_value()) {
        throwTypeError();
    }
    return *value;
}

}
