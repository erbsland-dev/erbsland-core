// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/OutOfRangeError.hpp"
#include "../unit/ElementCount.hpp"
#include "../unit/ElementIndex.hpp"

#include <concepts>
#include <cstddef>
#include <vector>

namespace erbsland::conf {

/// A 2D matrix with per-row column counts.
/// @tested{MatrixTest}
template <typename T>
    requires std::default_initializable<T> && std::copyable<T>
class Matrix {
public:
    static constexpr std::size_t cMaximumValueCount = 100'000'000U;

public:
    /// Create an empty matrix.
    Matrix() = default;
    /// Create a matrix with a given size.
    /// @param rowCount The number of rows.
    /// @param columnCount The number of columns.
    /// @throws err::OutOfRangeError if the matrix exceeds `cMaximumValueCount`.
    Matrix(const unit::ElementCount rowCount, const unit::ElementCount columnCount) :
        _rowCount{rowCount},
        _columnCount{columnCount},
        _actualColumnCounts(rowVectorSize(rowCount, columnCount), unit::ElementCount::zero()),
        _values(valueVectorSize(rowCount, columnCount)) {}

public: // access
    /// Get the number of rows in this matrix.
    [[nodiscard]] auto rowCount() const noexcept -> unit::ElementCount { return _rowCount; }
    /// Get the number of columns in this matrix.
    [[nodiscard]] auto columnCount() const noexcept -> unit::ElementCount { return _columnCount; }
    /// Get the actual column count for the given row.
    /// @param row The row index.
    /// @return The number of columns defined in the row.
    [[nodiscard]] auto actualColumnCount(const unit::ElementIndex row) const noexcept -> unit::ElementCount {
        if (!row.isWithin(_rowCount)) {
            return {};
        }
        return _actualColumnCounts[row.toSizeT()];
    }
    /// Test if a value was defined in the original nested list.
    /// @param row The row index.
    /// @param column The column index.
    /// @return `true` if the value was defined.
    [[nodiscard]] auto isDefined(const unit::ElementIndex row, const unit::ElementIndex column) const noexcept -> bool {
        if (!row.isWithin(_rowCount) || !column.isWithin(_columnCount)) {
            return false;
        }
        return column.isWithin(_actualColumnCounts[row.toSizeT()]);
    }
    /// Access a value by row and column.
    /// @param row The row index.
    /// @param column The column index.
    /// @param defaultValue The default value for missing cells.
    /// @return The value or the default value if it was not defined.
    [[nodiscard]] auto value(
        const unit::ElementIndex row, const unit::ElementIndex column, const T &defaultValue = {}) const noexcept -> T {

        if (!isDefined(row, column)) {
            return defaultValue;
        }
        return _values[toIndex(row, column)];
    }
    /// Access a value by row and column and throw on bounds errors.
    /// @param row The row index.
    /// @param column The column index.
    /// @return The value or a default value if it was not defined.
    /// @throws err::OutOfRangeError if the row or column is outside the matrix.
    [[nodiscard]] auto valueOrThrow(const unit::ElementIndex row, const unit::ElementIndex column) const -> const T & {
        requireIndices(row, column);
        return _values[toIndex(row, column)];
    }
    /// Set a value.
    /// @param row The row index.
    /// @param column The column index.
    /// @param value The value to set.
    /// @throws err::OutOfRangeError if the row or column is outside the matrix.
    void setValue(const unit::ElementIndex row, const unit::ElementIndex column, const T &value) {
        requireIndices(row, column);
        _values[toIndex(row, column)] = value;
        updateActualColumnCount(row, column.distanceFromZero() + unit::ElementCount::one());
    }
    /// Set values for a complete row.
    /// @param row The row index.
    /// @param values The values to set.
    /// @throws err::OutOfRangeError if the row is outside the matrix.
    void setRow(const unit::ElementIndex row, const std::vector<T> &values) {
        requireRowIndex(row);
        if (values.size() > _columnCount.toSizeT()) {
            throw err::OutOfRangeError("Matrix column index out of range");
        }
        for (unit::ElementIndex column = {}; column.toSizeT() < values.size(); ++column) {
            _values[toIndex(row, column)] = values[column.toSizeT()];
        }
        updateActualColumnCount(row, unit::ElementCount::fromSizeT(values.size()));
    }

private:
    [[nodiscard]] auto toIndex(const unit::ElementIndex row, const unit::ElementIndex column) const noexcept
        -> std::size_t {
        return (row.toSizeT() * _columnCount.toSizeT()) + column.toSizeT();
    }
    void requireRowIndex(const unit::ElementIndex row) const {
        if (!row.isWithin(_rowCount)) {
            throw err::OutOfRangeError("Matrix row index out of range");
        }
    }
    void requireIndices(const unit::ElementIndex row, const unit::ElementIndex column) const {
        requireRowIndex(row);
        if (!column.isWithin(_columnCount)) {
            throw err::OutOfRangeError("Matrix column index out of range");
        }
    }
    void updateActualColumnCount(const unit::ElementIndex row, const unit::ElementCount columnCount) {
        if (columnCount > _actualColumnCounts[row.toSizeT()]) {
            _actualColumnCounts[row.toSizeT()] = columnCount;
        }
    }
    static void requireValidSize(const unit::ElementCount rowCount, const unit::ElementCount columnCount) {
        const auto maximumValueCount = static_cast<unit::ElementCount::Value>(cMaximumValueCount);
        if (!rowCount.isFinite() || !columnCount.isFinite() || rowCount.toRawValue() > maximumValueCount ||
            (rowCount.toRawValue() != 0U && columnCount.toRawValue() > maximumValueCount / rowCount.toRawValue())) {
            throw err::OutOfRangeError("Matrix size exceeds maximum allowed");
        }
    }
    [[nodiscard]] static auto rowVectorSize(const unit::ElementCount rowCount, const unit::ElementCount columnCount)
        -> std::size_t {
        requireValidSize(rowCount, columnCount);
        return rowCount.toSizeTOrThrow();
    }
    [[nodiscard]] static auto valueVectorSize(const unit::ElementCount rowCount, const unit::ElementCount columnCount)
        -> std::size_t {
        requireValidSize(rowCount, columnCount);
        return static_cast<std::size_t>(rowCount.toRawValue() * columnCount.toRawValue());
    }

private:
    unit::ElementCount _rowCount{0};
    unit::ElementCount _columnCount{0};
    std::vector<unit::ElementCount> _actualColumnCounts;
    std::vector<T> _values;
};

}
