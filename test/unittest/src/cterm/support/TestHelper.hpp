// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColorTermIncludes.hpp"

#include <erbsland/cterm/all.hpp>
#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

using namespace erbsland::text::literals;

/// Convert one terminal block to standard text.
[[nodiscard]] inline auto blockToStdString(const Block &block) -> std::string {
    return erbsland::text::StringConverter{block.toString()}.toStdString();
}

/// Create a terminal block coordinate from an integer.
[[nodiscard]] constexpr auto blockCoordinate(const int value) noexcept -> bgeo::BlockCoordinate {
    return bgeo::BlockCoordinate{value};
}

/// Create terminal block coordinates from integer values.
[[nodiscard]] inline auto blockCoordinates(const std::initializer_list<int> values)
    -> std::vector<bgeo::BlockCoordinate> {
    auto result = std::vector<bgeo::BlockCoordinate>{};
    result.reserve(values.size());
    for (const auto value : values) {
        result.emplace_back(value);
    }
    return result;
}

/// Create a grid layout from column widths and row heights.
[[nodiscard]] inline auto gridLayout(
    const std::initializer_list<int> columnWidths, const std::initializer_list<int> rowHeights) -> GridLayout {
    return GridLayout{blockCoordinates(columnWidths), blockCoordinates(rowHeights)};
}

/// Provides common terminal test helpers.
/// @notest{This helper is exercised by terminal test suites that inherit from it.}
class TestHelper : public el::UnitTest {
public:
    /// Create a byte string from byte values.
    [[nodiscard]] static auto bytes(const std::initializer_list<uint8_t> values) -> std::string {
        auto result = std::string{};
        result.reserve(values.size());
        for (const auto value : values) {
            result.push_back(static_cast<char>(value));
        }
        return result;
    }
};
