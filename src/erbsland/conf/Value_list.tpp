// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::conf {

using namespace text::literals;

template <typename T>
auto Value::asListOrThrow() const -> std::vector<T> {
    constexpr auto expectedType = ValueType::from<T>();
    static_assert(
        expectedType.raw() != ValueType::Undefined, "no type support available for the specified template argument.");
    if (type() == expectedType) { // convert a single value into a list.
        return std::vector<T>{{asType<T>()}};
    }
    if (type() != ValueType::ValueList) {
        throw ConfError(
            ConfErrorCategory::TypeMismatch,
            text::StringFormat{"Expected a list of '{}' values, but got a single value of type '{}'."_el}.build(
                expectedType.toText(), type().toText()),
            namePath(),
            location());
    }
    const auto valueList = asValueList();
    std::vector<T> result;
    result.reserve(valueList.size());
    for (const auto &value : valueList) {
        if (value->type() != expectedType) {
            throw ConfError(
                ConfErrorCategory::TypeMismatch,
                text::StringFormat{
                    "Expected all values in the list to be of type '{}', but found an element of type '{}'."_el}
                    .build(expectedType.toText(), value->type().toText()),
                value->namePath(),
                value->location());
        }
        result.push_back(value->asType<T>());
    }
    return result;
}

template <typename T>
auto Value::getListOrThrow(const NamePathLike &namePath) const -> std::vector<T> {
    auto valueAtPath = valueOrThrow(namePath);
    constexpr auto expectedType = ValueType::from<T>();
    static_assert(
        expectedType.raw() != ValueType::Undefined, "no type support available for the specified template argument.");
    return valueAtPath->asListOrThrow<T>();
}

template <typename T>
auto Value::getList(const NamePathLike &namePath) const noexcept -> std::vector<T> {
    try {
        return getListOrThrow<T>(namePath);
    } catch (const ConfError &) {
        return {};
    }
}

template <typename T>
auto Value::asList() const noexcept -> std::vector<T> {
    try {
        return asListOrThrow<T>();
    } catch (const ConfError &) {
        return {};
    }
}

}
