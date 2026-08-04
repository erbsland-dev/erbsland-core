// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::conf {

using namespace text::literals;

template <typename T>
auto Value::asMatrixOrThrow() const -> Matrix<T> {
    constexpr auto expectedType = ValueType::from<T>();
    static_assert(
        expectedType.raw() != ValueType::Undefined, "no type support available for the specified template argument.");
    if (type() != ValueType::ValueList && !type().isScalar()) {
        throw ConfError(
            ConfErrorCategory::TypeMismatch,
            text::StringFormat{"Expected a matrix of '{}' values, but got a value of type '{}'."_el}.build(
                expectedType.toText(), type().toText()),
            namePath(),
            location());
    }
    const auto valueList = toValueMatrix();
    const auto rowCount = valueList.rowCount();
    const auto columnCount = valueList.columnCount();
    Matrix<T> result{rowCount, columnCount};
    for (unit::ItemIndex row = {}; row.isWithin(rowCount); ++row) {
        for (unit::ItemIndex column = {}; column.isWithin(valueList.actualColumnCount(row)); ++column) {
            const auto &value = valueList.valueOrThrow(row, column);
            if (!value) {
                throw ConfError(
                    ConfErrorCategory::TypeMismatch,
                    text::StringFormat{
                        "Expected all values in the matrix to be of type '{}', but found an empty value."_el}
                        .build(expectedType.toText()),
                    namePath(),
                    location());
            }
            if (value->type() != expectedType) {
                throw ConfError(
                    ConfErrorCategory::TypeMismatch,
                    text::StringFormat{
                        "Expected all values in the matrix to be of type '{}', but found an element of type '{}'."_el}
                        .build(expectedType.toText(), value->type().toText()),
                    value->namePath(),
                    value->location());
            }
            result.setValue(row, column, value->asType<T>());
        }
    }
    return result;
}

template <typename T>
auto Value::asMatrix() const noexcept -> Matrix<T> {
    try {
        return asMatrixOrThrow<T>();
    } catch (const ConfError &) {
        return {};
    }
}

template <typename T>
auto Value::getMatrixOrThrow(const NamePathLike &namePath) const -> Matrix<T> {
    auto valueAtPath = valueOrThrow(namePath);
    constexpr auto expectedType = ValueType::from<T>();
    static_assert(
        expectedType.raw() != ValueType::Undefined, "no type support available for the specified template argument.");
    return valueAtPath->asMatrixOrThrow<T>();
}

template <typename T>
auto Value::getMatrix(const NamePathLike &namePath) const noexcept -> Matrix<T> {
    try {
        return getMatrixOrThrow<T>(namePath);
    } catch (const ConfError &) {
        return {};
    }
}

}
