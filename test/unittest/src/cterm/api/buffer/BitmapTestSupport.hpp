// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BitmapAccessor.hpp"

#include "../../support/TestHelper.hpp"

#include <erbsland/text/StringConverter.hpp>

#include <vector>

class BitmapTestSupport : public TestHelper {
public:
    [[nodiscard]] auto renderRows(const Bitmap &bitmap) -> std::vector<std::string> {
        auto rows = std::vector<std::string>{};
        rows.reserve(bitmap.size().height().toSizeT());
        for (auto y = bgeo::BlockCoordinate{0}; y < bitmap.size().height(); ++y) {
            auto row = std::string{};
            row.reserve(bitmap.size().width().toSizeT());
            for (auto x = bgeo::BlockCoordinate{0}; x < bitmap.size().width(); ++x) {
                row += bitmap.pixel(bgeo::BlockPosition{x, y}) ? '#' : '.';
            }
            rows.push_back(std::move(row));
        }
        return rows;
    }

    void requireRowsEqual(const Bitmap &bitmap, const std::vector<std::string> &expectedRows) {
        REQUIRE_EQUAL_LINES(renderRows(bitmap), expectedRows);
    }

    void requireRowsEqual(const Bitmap &bitmap, const std::initializer_list<erbsland::text::String> expectedRows) {
        auto convertedRows = std::vector<std::string>{};
        convertedRows.reserve(expectedRows.size());
        for (const auto row : expectedRows) {
            convertedRows.push_back(erbsland::text::StringConverter{row}.toStdString());
        }
        REQUIRE_EQUAL_LINES(renderRows(bitmap), convertedRows);
    }

    void requireRectangleEqual(const bgeo::BlockRectangle &actual, const bgeo::BlockRectangle &expected) {
        REQUIRE_EQUAL(actual.topLeft(), expected.topLeft());
        REQUIRE_EQUAL(actual.size(), expected.size());
    }
};
