// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../block/Coordinate.hpp"
#include "../block/Size.hpp"

namespace erbsland::cterm {

/// Construction policies for incremental terminal emulation.
/// @seedoc{/reference/cterm/terminal_emulation}
/// @tested{TerminalEmulationTest}
class TerminalEmulationOptions final {
public:
    // defaults
    TerminalEmulationOptions() = default;

public: // accessors
    /// Get the fixed regular and alternate screen size.
    [[nodiscard]] auto size() const noexcept -> block::Size { return _size; }
    /// Set the screen size; dimensions must be between 1 and 10,000.
    auto setSize(block::Size size) -> TerminalEmulationOptions &;
    /// Get the maximum number of retained scrollback rows.
    [[nodiscard]] auto scrollbackLimit() const noexcept -> block::Coordinate { return _scrollbackLimit; }
    /// Set the scrollback limit, between 1 and 10,000 rows.
    auto setScrollbackLimit(block::Coordinate limit) -> TerminalEmulationOptions &;
    /// Test whether printing at the right edge wraps to the next line.
    [[nodiscard]] auto autoWrap() const noexcept -> bool { return _autoWrap; }
    /// Select automatic wrapping at the right edge.
    auto setAutoWrap(bool enabled) noexcept -> TerminalEmulationOptions & {
        _autoWrap = enabled;
        return *this;
    }
    /// Get the distance between horizontal tab stops.
    [[nodiscard]] auto tabWidth() const noexcept -> block::Coordinate { return _tabWidth; }
    /// Set the tab width, between 1 and 10,000.
    auto setTabWidth(block::Coordinate width) -> TerminalEmulationOptions &;
    /// Test whether LF also returns the cursor to column zero.
    [[nodiscard]] auto lineFeedReturnsToColumnZero() const noexcept -> bool { return _lineFeedReturnsToColumnZero; }
    /// Select whether LF also returns the cursor to column zero.
    auto setLineFeedReturnsToColumnZero(bool enabled) noexcept -> TerminalEmulationOptions & {
        _lineFeedReturnsToColumnZero = enabled;
        return *this;
    }

private:
    block::Size _size{80, 25};                  ///< Initial screen dimensions.
    block::Coordinate _scrollbackLimit{10'000}; ///< Maximum retained scrollback rows.
    block::Coordinate _tabWidth{8};             ///< Horizontal tab interval.
    bool _autoWrap{true};                       ///< Initial right-edge wrapping state.
    bool _lineFeedReturnsToColumnZero{true};    ///< LF also performs CR.
};

}
