// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LayoutRectangle.hpp"
#include "Theme.hpp"

#include "../../bgeo/BlockRectangle.hpp"

#include <cassert>

namespace erbsland::cterm::theme {

/// Read-only theme accessor for the current selector.
class ThemeAccessor final {
public:
    /// Create an accessor for the default theme root.
    ThemeAccessor() noexcept : ThemeAccessor{Theme::dark(), Selector{Element::Base}} {}
    /// Create an accessor.
    /// @param theme The active theme. Must not be null.
    /// @param selector The selector.
    ThemeAccessor(ThemeConstPtr theme, const Selector selector) noexcept :
        _theme{std::move(theme)}, _selector{selector} {
        assert(_theme != nullptr);
    }

public:
    /// Access the active theme.
    [[nodiscard]] auto theme() const noexcept -> const Theme & { return *_theme; }
    /// Access the selected selector.
    [[nodiscard]] auto selector() const noexcept -> Selector { return _selector; }
    /// Create an accessor for another part.
    [[nodiscard]] auto forPart(Part part) const noexcept -> ThemeAccessor;
    /// Create an accessor with additional states.
    [[nodiscard]] auto withStates(States states) const noexcept -> ThemeAccessor;
    /// Create an accessor with additional tags.
    [[nodiscard]] auto withTags(Tags tags) const noexcept -> ThemeAccessor;
    /// Resolve the color for an animation cycle.
    [[nodiscard]] auto color(std::size_t animationCycle = 0) const noexcept -> Color {
        return sheet()->color(animationCycle);
    }
    /// Access the effective color sequence.
    [[nodiscard]] auto colorSequence() const noexcept -> const ColorSequence & { return sheet()->colorSequence(); }
    /// Access the effective attributes.
    [[nodiscard]] auto attributes() const noexcept -> BlockAttributes { return sheet()->attributes(); }
    /// Resolve the style for an animation cycle.
    [[nodiscard]] auto style(const std::size_t animationCycle = 0) const noexcept -> BlockStyle {
        return sheet()->style(animationCycle);
    }
    /// Resolve a block character for a role.
    [[nodiscard]] auto block(
        const BlockRole role = BlockRole::Main, const std::size_t animationCycle = 0) const noexcept -> Block {
        return sheet()->block(role, animationCycle);
    }
    /// Resolve the tile-9 style for an animation cycle.
    [[nodiscard]] auto tile9Style(const std::size_t animationCycle = 0) const noexcept -> Tile9Style {
        return sheet()->tile9Style(animationCycle);
    }
    /// Access the effective parent-owned margins outside the current part.
    /// bgeo::BlockMargins are always zero or positive.
    [[nodiscard]] auto margins() const noexcept -> bgeo::BlockMargins { return sheet()->margins(); }
    /// Access the effective horizontal margins with top and bottom set to zero.
    /// Use this for strict one-line text where vertical margins are intentionally ignored.
    /// bgeo::BlockMargins are always zero or positive.
    [[nodiscard]] auto horizontalMargins() const noexcept -> bgeo::BlockMargins {
        const auto currentMargins = margins();
        return bgeo::BlockMargins{
            bgeo::BlockCoordinate{0}, currentMargins.right(), bgeo::BlockCoordinate{0}, currentMargins.left()};
    }
    /// Access the effective part-owned padding inside the current part.
    /// Padding is always zero or positive.
    [[nodiscard]] auto padding() const noexcept -> bgeo::BlockMargins { return sheet()->padding(); }
    /// Access the effective horizontal padding with top and bottom set to zero.
    /// Use this for strict one-line text where vertical padding is intentionally ignored.
    /// Padding is always zero or positive.
    [[nodiscard]] auto horizontalPadding() const noexcept -> bgeo::BlockMargins {
        const auto currentPadding = padding();
        return bgeo::BlockMargins{
            bgeo::BlockCoordinate{0}, currentPadding.right(), bgeo::BlockCoordinate{0}, currentPadding.left()};
    }
    /// Calculate the size consumed by margins and padding.
    /// Extent is always zero or positive.
    [[nodiscard]] auto extent() const noexcept -> bgeo::BlockSize { return margins().extent() + padding().extent(); }
    /// Calculate the horizontal size consumed by margins and padding.
    /// Use this for strict one-line text where vertical margins and padding are intentionally ignored.
    /// Extent is always zero or positive.
    [[nodiscard]] auto horizontalExtent() const noexcept -> bgeo::BlockSize {
        return horizontalMargins().extent() + horizontalPadding().extent();
    }
    /// Resolve the themed layout rectangles for an assigned rectangle.
    /// @param outerRect The assigned rectangle, including margins.
    /// @return The resolved rectangles.
    [[nodiscard]] auto layout(const bgeo::BlockRectangle outerRect) const noexcept -> LayoutRectangles {
        const auto currentPartRect = outerRect.insetBy(margins());
        return LayoutRectangles{
            .outerRect = outerRect,
            .partRect = currentPartRect,
            .contentRect = currentPartRect.insetBy(padding()),
        };
    }
    /// Resolve horizontal-only themed layout rectangles for an assigned rectangle.
    /// Use this for strict one-line text where vertical margins and padding are intentionally ignored.
    /// @param outerRect The assigned rectangle, including horizontal margins.
    /// @return The resolved rectangles.
    [[nodiscard]] auto horizontalLayout(const bgeo::BlockRectangle outerRect) const noexcept -> LayoutRectangles {
        const auto currentPartRect = outerRect.insetBy(horizontalMargins());
        return LayoutRectangles{
            .outerRect = outerRect,
            .partRect = currentPartRect,
            .contentRect = currentPartRect.insetBy(horizontalPadding()),
        };
    }
    /// Resolve the content rectangle by applying margins.
    /// This only removes the parent-owned margin area. Apply `padding()` separately when resolving the inner content of
    /// a part.
    [[nodiscard]] auto contentRect(const bgeo::BlockRectangle rect) const noexcept -> bgeo::BlockRectangle {
        return rect.insetBy(margins());
    }

private:
    [[nodiscard]] auto sheet() const noexcept -> PropertySheetConstPtr { return _theme->propertySheet(_selector); }

private:
    ThemeConstPtr _theme; ///< The active theme.
    Selector _selector;   ///< The current selector.
};

}
