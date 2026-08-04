// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStringTestHelper.hpp"

#include <erbsland/bgeo/StdFormat.hpp>
#include <erbsland/text/StdFormat.hpp>

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// Provides buffer construction, rendering, and assertion helpers for terminal tests.
/// @notest{This helper is exercised by terminal buffer test suites that inherit from it.}
class BufferTestHelper : public BlockStringTestHelper {
public:
    /// Create a buffer from equal-width text rows.
    auto createBuffer(const std::initializer_list<std::string_view> rows) -> Buffer {
        REQUIRE_FALSE(rows.size() == 0);
        auto width = bgeo::BlockCoordinate{0};
        auto rowIndex = 0;
        for (const auto row : rows) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    REQUIRE_FALSE(row.empty());
                    if (width == 0) {
                        width = bgeo::BlockCoordinate{row.size()};
                    } else {
                        REQUIRE_EQUAL(bgeo::BlockCoordinate{row.size()}, width);
                    }
                },
                [&]() -> std::string {
                    return std::format(
                        "rowIndex = {} / row = \"{}\" / rowSize = {} / expectedWidth = {}",
                        rowIndex,
                        row,
                        row.size(),
                        width.toRawValue());
                });
            rowIndex += 1;
        }
        auto buffer = Buffer{bgeo::BlockSize{width, bgeo::BlockCoordinate{rows.size()}}};
        fillBufferFromRows(buffer, rows);
        return buffer;
    }

    /// Create a shared buffer from equal-width text rows.
    auto createSharedBuffer(const std::initializer_list<std::string_view> rows) -> std::shared_ptr<Buffer> {
        return std::make_shared<Buffer>(createBuffer(rows));
    }

    /// Fill a writable buffer from text rows.
    void fillBufferFromRows(WritableBuffer &buffer, const std::initializer_list<std::string_view> rows) {
        REQUIRE(buffer.size().height() >= bgeo::BlockCoordinate{rows.size()});
        auto y = bgeo::BlockCoordinate{0};
        auto rowIndex = 0;
        for (const auto row : rows) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() { REQUIRE(buffer.size().width() >= bgeo::BlockCoordinate{row.size()}); },
                [&]() -> std::string {
                    return std::format(
                        "rowIndex = {} / row = \"{}\" / rowSize = {} / bufferWidth = {} / bufferHeight = {}",
                        rowIndex,
                        row,
                        row.size(),
                        buffer.size().width().toRawValue(),
                        buffer.size().height().toRawValue());
                });
            for (auto x = bgeo::BlockCoordinate{0}; x < bgeo::BlockCoordinate{row.size()}; ++x) {
                buffer.set(
                    bgeo::BlockPosition{x, y},
                    Block{static_cast<char32_t>(static_cast<unsigned char>(row[x.toSizeT()]))});
            }
            y += 1;
            rowIndex += 1;
        }
    }

    /// Render all rows of a readable buffer.
    [[nodiscard]] static auto renderRows(const auto &buffer) -> std::vector<std::string> {
        auto rows = std::vector<std::string>{};
        rows.reserve(buffer.size().height().toSizeT());
        for (auto y = bgeo::BlockCoordinate{0}; y < buffer.size().height(); ++y) {
            auto row = std::string{};
            for (auto x = bgeo::BlockCoordinate{0}; x < buffer.size().width(); ++x) {
                const auto &block = buffer.get(bgeo::BlockPosition{x, y});
                const auto text = block.toString();
                row += text.isEmpty() ? " " : erbsland::text::StringConverter{text}.toStdString();
            }
            rows.push_back(std::move(row));
        }
        return rows;
    }

    /// Require buffer rows to equal expected text rows.
    void requireRowsEqual(const auto &buffer, const std::initializer_list<std::string_view> expectedRows) {
        auto expected = std::vector<std::string>{};
        expected.reserve(expectedRows.size());
        for (const auto row : expectedRows) {
            expected.emplace_back(row);
        }
        REQUIRE_EQUAL_LINES(renderRows(buffer), expected);
    }
};
