// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColumnIndex.hpp"
#include "CpIndex.hpp"
#include "LineIndex.hpp"

#include "../text/String.hpp"

namespace erbsland::unit {

/// A source code location with optional line, column and code-point position.
/// @tested{CodeLocationTest}
class CodeLocation final {
public:
    /// Create an undefined code location.
    constexpr CodeLocation() noexcept = default;
    /// Create a code location from its optional components.
    constexpr CodeLocation(
        LineIndex line, ColumnIndex column = ColumnIndex::noIndex(), CpIndex position = CpIndex::noIndex()) noexcept :
        _line{line}, _column{column}, _position{position} {}

    // defaults
    ~CodeLocation() = default;
    CodeLocation(const CodeLocation &) = default;
    CodeLocation(CodeLocation &&) noexcept = default;
    auto operator=(const CodeLocation &) -> CodeLocation & = default;
    auto operator=(CodeLocation &&) noexcept -> CodeLocation & = default;

public: // accessors
    /// Get the zero-based line index.
    [[nodiscard]] constexpr auto line() const noexcept -> LineIndex { return _line; }
    /// Set the zero-based line index.
    constexpr auto setLine(LineIndex line) noexcept -> CodeLocation & {
        _line = line;
        return *this;
    }
    /// Get the zero-based column index.
    [[nodiscard]] constexpr auto column() const noexcept -> ColumnIndex { return _column; }
    /// Set the zero-based column index.
    constexpr auto setColumn(ColumnIndex column) noexcept -> CodeLocation & {
        _column = column;
        return *this;
    }
    /// Get the zero-based absolute code-point position.
    [[nodiscard]] constexpr auto position() const noexcept -> CpIndex { return _position; }
    /// Set the zero-based absolute code-point position.
    constexpr auto setPosition(CpIndex position) noexcept -> CodeLocation & {
        _position = position;
        return *this;
    }

public: // operators
    /// Compare two locations.
    auto operator==(const CodeLocation &) const noexcept -> bool = default;

    /// Test if no location component is defined.
    [[nodiscard]] auto isUndefined() const noexcept -> bool;
    /// Advance to the first character of the next line.
    void nextLine() noexcept;
    /// Advance to the next character in the current line.
    void nextColumn() noexcept;
    /// Format the known location in one-based, human-readable form.
    [[nodiscard]] auto toString() const noexcept -> text::String;

private:
    LineIndex _line{LineIndex::noIndex()};       ///< The zero-based line index, if known.
    ColumnIndex _column{ColumnIndex::noIndex()}; ///< The zero-based column index, if known.
    CpIndex _position{CpIndex::noIndex()};       ///< The zero-based code-point position, if known.
};

}
