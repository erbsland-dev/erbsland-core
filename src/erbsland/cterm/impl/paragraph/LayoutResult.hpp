// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LayoutLine.hpp"

#include <cstddef>
#include <vector>

namespace erbsland::cterm::impl::paragraph {

/// The complete wrapped paragraph layout result.
class LayoutResult final {
public:
    /// Create an invalid layout result.
    LayoutResult() = default;

    // defaults
    ~LayoutResult() = default;
    LayoutResult(const LayoutResult &) = delete;
    LayoutResult(LayoutResult &&) noexcept = default;
    auto operator=(const LayoutResult &) -> LayoutResult & = delete;
    auto operator=(LayoutResult &&) noexcept -> LayoutResult & = default;

public:
    /// Test whether paragraph rendering is possible.
    /// @return `true` if paragraph rendering is possible.
    [[nodiscard]] auto valid() const noexcept -> bool { return _valid; }
    /// Access the rendered physical lines.
    /// @return The rendered physical lines.
    [[nodiscard]] auto lines() noexcept -> std::vector<LayoutLine> & { return _lines; }
    /// Access the rendered physical lines.
    /// @return The rendered physical lines.
    [[nodiscard]] auto lines() const noexcept -> const std::vector<LayoutLine> & { return _lines; }
    /// Test whether the layout contains any rendered lines.
    /// @return `true` if the layout contains no rendered lines.
    [[nodiscard]] auto empty() const noexcept -> bool { return _lines.empty(); }
    /// Get the number of rendered physical lines.
    /// @return The number of rendered physical lines.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return _lines.size(); }
    /// Append an empty rendered line.
    void appendEmptyLine();
    /// Append one rendered line.
    /// @param line The rendered line to append.
    void appendLine(LayoutLine line);

public:
    /// Create a valid layout result.
    /// @return A valid layout result ready to receive rendered lines.
    [[nodiscard]] static auto create() noexcept -> LayoutResult {
        auto result = LayoutResult{};
        result._valid = true;
        return result;
    }
    /// Create an invalid layout result.
    /// @return An invalid layout result.
    [[nodiscard]] static auto invalid() noexcept -> LayoutResult { return {}; }

private:
    bool _valid = false;
    std::vector<LayoutLine> _lines;
};

}
