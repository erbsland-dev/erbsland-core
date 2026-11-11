// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/block/Coordinate.hpp>
#include <erbsland/CpCount.hpp>

#include <cstddef>

namespace demo {

using el::block::Coordinate;

/// Keep a scroll position in fixed-width digit rows independently of terminal rendering.
/// @tested{DigitHistoryTest}
class DigitViewport final {
public:
    /// Fractional digits per displayed row.
    static constexpr auto cDigitsPerLine = el::CpCount{50U};

public: // accessors
    /// Get the first visible logical row.
    [[nodiscard]] auto firstLine() const noexcept -> Coordinate { return _firstLine; }
    /// Test whether new digits move the viewport to the end.
    [[nodiscard]] auto isFollowing() const noexcept -> bool { return _following; }
    /// Get the visible row count, at least one.
    [[nodiscard]] auto visibleLines() const noexcept -> Coordinate { return _visibleLines; }

public: // navigation
    /// Apply a changed history size or terminal height and clamp the retained scroll position.
    /// @param digits Total fractional digits available.
    /// @param visibleLines Rows available for digits, normalized to at least one.
    void update(el::CpCount digits, Coordinate visibleLines) noexcept;
    /// Move toward earlier rows and disable following.
    /// @param lines Number of logical rows to move.
    void up(Coordinate lines) noexcept;
    /// Move toward later rows and disable following.
    /// @param lines Number of logical rows to move.
    void down(Coordinate lines) noexcept;
    /// Move to the beginning and disable following.
    void home() noexcept;
    /// Move to the last page, without following subsequent updates.
    void end() noexcept;
    /// Move to the last page and continue following subsequent updates.
    void follow() noexcept;

private:
    /// Get the latest valid first row for the current history and terminal height.
    [[nodiscard]] auto maximumFirstLine() const noexcept -> Coordinate;

private:
    Coordinate _lineCount{0};    ///< Logical rows in the full history.
    Coordinate _visibleLines{1}; ///< Current terminal capacity.
    Coordinate _firstLine{0};    ///< First visible row, clamped on resize.
    bool _following{true};       ///< Follow latest digits until manual navigation.
};

}
