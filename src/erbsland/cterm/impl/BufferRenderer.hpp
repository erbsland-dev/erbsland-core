// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ReadableBuffer.hpp"

#include "../../text/StringEditor.hpp"

#include <optional>
#include <vector>

namespace erbsland::cterm::impl {

/// Measure logical rows and serialize their visible cells with resolved presentation.
/// @tested{BufferRenderTest}
class BufferRenderer final {
public:
    /// Bind a buffer and copy the selected output policies.
    /// @param buffer Source buffer, retained by reference.
    /// @param options Rendering policies.
    BufferRenderer(const ReadableBuffer &buffer, BufferRenderOptions options);

public: // rendering
    /// Render the buffer without modifying its content or cursor.
    /// @return Serialized logical rows.
    auto render() -> text::String;

private:
    /// Normalize clipped wide cells and omit in-view continuation cells.
    auto cell(block::Position position) const -> std::optional<Block>;
    /// Find the last meaningful column, preserving styled blanks.
    auto contentWidth(block::Coordinate row) const -> block::Coordinate;
    /// Measure row widths and select the bottom row retained by the trimming policy.
    auto measure() -> block::Coordinate;
    /// Append one measured row.
    void appendRow(block::Coordinate row);
    /// Encode presentation changes from the previously emitted cell.
    void appendStyle(BlockStyle style);

private:
    const ReadableBuffer &_buffer;          ///< Borrowed source.
    BufferRenderOptions _options;           ///< Copied policies.
    std::vector<block::Coordinate> _widths; ///< Measured output widths.
    text::StringEditor _output;             ///< Serialized output.
    BlockStyle _style{BlockStyle::reset()}; ///< Previously emitted presentation.
};

}
