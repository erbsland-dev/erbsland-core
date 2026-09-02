// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLine_fwd.hpp"
#include "LogLineSegment.hpp"

#include <vector>

namespace erbsland::log {

/// A formatted log line with semantic segments for styled writers.
/// @tested{LogCoreTest LogWriterTest}
class LogLine final {
public:
    /// Create an empty formatted line.
    LogLine() = default;
    /// Create a line from ordered semantic segments.
    /// @param segments The rendered segments in output order.
    explicit LogLine(std::vector<LogLineSegment> segments);

    /// Get the complete rendered text.
    [[nodiscard]] auto text() const noexcept -> const text::String & { return _text; }
    /// Get the ordered semantic segments.
    [[nodiscard]] auto segments() const noexcept -> const std::vector<LogLineSegment> & { return _segments; }

private:
    std::vector<LogLineSegment> _segments; ///< Rendered segments in output order.
    text::String _text;                    ///< Concatenated text of all segments.
};

}
