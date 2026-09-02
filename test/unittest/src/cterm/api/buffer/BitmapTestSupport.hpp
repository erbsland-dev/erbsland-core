// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BitmapAccessor.hpp"

#include "../../support/TestHelper.hpp"

#include <erbsland/text/StringConverter.hpp>

#include <vector>

/// Test helpers for asserting terminal bitmap contents and geometry.
/// @notest{Used only by bitmap unit tests.}
class BitmapTestSupport : public TestHelper {
public:
    /// Render bitmap pixels as rows of hash and dot characters.
    [[nodiscard]] auto renderRows(const Bitmap &bitmap) -> std::vector<std::string> {
        auto rows = std::vector<std::string>{};
        rows.reserve(bitmap.size().height().toSizeT());
        for (auto y = block::Coordinate{0}; y < bitmap.size().height(); ++y) {
            auto row = std::string{};
            row.reserve(bitmap.size().width().toSizeT());
            for (auto x = block::Coordinate{0}; x < bitmap.size().width(); ++x) {
                row += bitmap.pixel(block::Position{x, y}) ? '#' : '.';
            }
            rows.push_back(std::move(row));
        }
        return rows;
    }

    /// Require bitmap rows to match expected native-string rows.
    void requireRowsEqual(const Bitmap &bitmap, const std::vector<std::string> &expectedRows) {
        REQUIRE_EQUAL_LINES(renderRows(bitmap), expectedRows);
    }

    /// Require bitmap rows to match expected Erbsland-string rows.
    void requireRowsEqual(const Bitmap &bitmap, const std::initializer_list<erbsland::text::String> expectedRows) {
        auto convertedRows = std::vector<std::string>{};
        convertedRows.reserve(expectedRows.size());
        for (const auto row : expectedRows) {
            convertedRows.push_back(erbsland::text::StringConverter{row}.toStdString());
        }
        REQUIRE_EQUAL_LINES(renderRows(bitmap), convertedRows);
    }

    /// Require two block rectangles to have identical origin and size.
    void requireRectangleEqual(const block::Rectangle &actual, const block::Rectangle &expected) {
        REQUIRE_EQUAL(actual.topLeft(), expected.topLeft());
        REQUIRE_EQUAL(actual.size(), expected.size());
    }
};
