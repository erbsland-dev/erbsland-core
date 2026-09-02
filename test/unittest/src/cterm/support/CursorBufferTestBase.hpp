// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TestHelper.hpp"

#include <erbsland/cterm/all.hpp>
#include <erbsland/cterm/CursorBuffer.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <string>
#include <string_view>
#include <vector>

/// Shared fixture for cursor-buffer unit tests.
/// @notest{Used only by cursor-buffer unit tests.}
class CursorBufferTestBase : public el::UnitTest {
public:
    using Lines = std::vector<std::string>;

public:
    CursorBuffer buffer{block::Size{20, 5}, CursorBuffer::OverflowMode::Wrap};

public:
    /// Get the buffer contents as raw test lines.
    [[nodiscard]] auto rawLinesFromBuffer() const -> Lines {
        std::vector<std::string> lines;
        for (int y = 0; y < buffer.size().height().toRawValue(); ++y) {
            std::string line;
            line.reserve(buffer.size().width().toSizeT() * 2);
            for (int x = 0; x < buffer.size().width().toRawValue(); ++x) {
                line += blockToStdString(buffer.get(block::Position{x, y}));
            }
            lines.emplace_back(std::move(line));
        }
        return lines;
    }
};
