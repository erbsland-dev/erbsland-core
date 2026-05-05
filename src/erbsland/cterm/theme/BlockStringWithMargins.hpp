// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BlockString.hpp"

#include "../../bgeo/BlockMargins.hpp"

namespace erbsland::cterm::theme {

/// A string with surrounding margins.
/// Padding is part of the string.
/// Only horizontal margins are relevant.
class BlockStringWithMargins final {
public:
    BlockStringWithMargins() = default;
    BlockStringWithMargins(BlockString text, const bgeo::BlockMargins margins) noexcept :
        _text(std::move(text)), _margins(margins.expandedPositive()) {}

    // defaults
    BlockStringWithMargins(BlockStringWithMargins const &) = default;
    BlockStringWithMargins(BlockStringWithMargins &&) noexcept = default;
    auto operator=(BlockStringWithMargins const &) -> BlockStringWithMargins & = default;
    auto operator=(BlockStringWithMargins &&) noexcept -> BlockStringWithMargins & = default;
    ~BlockStringWithMargins() = default;

public:
    /// Get the text.
    [[nodiscard]] auto blockString() const noexcept -> BlockString const & { return _text; }
    /// Get the margins.
    /// bgeo::BlockMargins are always zero or positive.
    [[nodiscard]] auto margins() const noexcept -> bgeo::BlockMargins const & { return _margins; }
    /// Get the text display width, without margins.
    /// @return The display width of the text, without outer margins.
    [[nodiscard]] auto displayWidth() const noexcept -> bgeo::BlockCoordinate {
        return bgeo::BlockCoordinate{_text.displayWidth()};
    }
    /// Join this tex with another one.
    /// The inner margins get collapsed and filled with transparent spaces.
    /// The first and last margins are returned in the result.
    /// This can be sightly more efficient than `joined()`.
    /// @param other The other string with margins to join with.
    void join(const BlockStringWithMargins &other) noexcept;
    /// Get the display width if another text is joined with this one.
    /// @param other The other string with margins to join with.
    /// @return The display width of the joined strings (without outer margins).
    [[nodiscard]] auto joinedDisplayWidth(const BlockStringWithMargins &other) const noexcept -> bgeo::BlockCoordinate {
        return bgeo::BlockCoordinate{_text.displayWidth()} + bgeo::BlockCoordinate{other._text.displayWidth()} +
            std::max(_margins.right(), other._margins.left());
    }
    /// Return this text, joined with another one.
    /// The inner margins get collapsed and filled with transparent spaces.
    /// The first and last margins are returned in the result.
    /// @param other The other string with margins to join with.
    /// @return The joined string with margins.
    [[nodiscard]] auto joined(const BlockStringWithMargins &other) const noexcept -> BlockStringWithMargins;

private:
    BlockString _text;           ///< The text with padding.
    bgeo::BlockMargins _margins; ///< The margins around this text.
};

}
