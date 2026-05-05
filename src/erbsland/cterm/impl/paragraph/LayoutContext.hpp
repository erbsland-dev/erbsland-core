// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../BlockStringView.hpp"
#include "../../ParagraphOptions.hpp"

#include <algorithm>
#include <cstddef>

namespace erbsland::cterm::impl::paragraph {

/// Shared immutable settings for building physical lines of one paragraph layout.
class LayoutContext final {
public:
    /// Create the build context for one paragraph layout run.
    /// @param text The source text referenced by source-range fragments.
    /// @param width The available paragraph width.
    /// @param options The paragraph options used for layout.
    /// @param leftAligned `true` if left-aligned paragraph behavior is active.
    LayoutContext(
        const BlockStringView &text, const int width, const ParagraphOptions &options, const bool leftAligned) noexcept
        :
        _text{text},
        _width{width},
        _options{options},
        _leftAligned{leftAligned},
        _lineBreakStartMarkWidth{options.lineBreakStartMark().displayWidth()},
        _lineBreakEndMarkWidth{options.lineBreakEndMark().displayWidth()},
        _paragraphEllipsisWidth{options.paragraphEllipsisMark().displayWidth()},
        _wordBreakMarkWidth{options.wordBreakMark().displayWidth()} {}

    // defaults/deletions
    ~LayoutContext() = default;
    LayoutContext(const LayoutContext &) = delete;
    LayoutContext(LayoutContext &&) = delete;
    auto operator=(const LayoutContext &) -> LayoutContext & = delete;
    auto operator=(LayoutContext &&) -> LayoutContext & = delete;

public:
    /// Access the source text.
    /// @return The source text referenced by source-range fragments.
    [[nodiscard]] auto blockString() const noexcept -> const BlockStringView & { return _text; }
    /// Access the paragraph width.
    /// @return The available paragraph width.
    [[nodiscard]] auto width() const noexcept -> int { return _width; }
    /// Access the paragraph options.
    /// @return The paragraph options used for layout.
    [[nodiscard]] auto options() const noexcept -> const ParagraphOptions & { return _options; }
    /// Test whether left-aligned paragraph behavior is active.
    /// @return `true` if left-aligned paragraph behavior is active.
    [[nodiscard]] auto leftAligned() const noexcept -> bool { return _leftAligned; }
    /// Get the display width of the configured wrapped-line start mark.
    /// @return The display width of the configured wrapped-line start mark.
    [[nodiscard]] auto lineBreakStartMarkWidth() const noexcept -> int { return _lineBreakStartMarkWidth; }
    /// Get the display width of the configured wrapped-line end mark.
    /// @return The display width of the configured wrapped-line end mark.
    [[nodiscard]] auto lineBreakEndMarkWidth() const noexcept -> int { return _lineBreakEndMarkWidth; }
    /// Get the display width of the configured paragraph ellipsis mark.
    /// @return The display width of the configured paragraph ellipsis mark.
    [[nodiscard]] auto paragraphEllipsisWidth() const noexcept -> int { return _paragraphEllipsisWidth; }
    /// Get the display width of the configured split-word marker.
    /// @return The display width of the configured split-word marker.
    [[nodiscard]] auto wordBreakMarkWidth() const noexcept -> int { return _wordBreakMarkWidth; }
    /// Resolve a configured tab stop index to an absolute column.
    /// @param tabStopIndex The configured tab stop index.
    /// @return The resolved tab stop column or `0` if none is configured.
    [[nodiscard]] auto resolveTabStop(std::size_t tabStopIndex) const noexcept -> int {
        if (tabStopIndex >= _options.tabStops().size()) {
            return 0;
        }
        const auto configuredStop = _options.tabStops()[tabStopIndex];
        if (configuredStop == ParagraphOptions::cTabWrappedLineIndent) {
            return _options.wrappedLineIndent();
        }
        return std::max(configuredStop, 0);
    }
    /// Calculate the indentation width for a physical line.
    /// @param isFirstPhysicalLine `true` for the first physical line of a source line.
    /// @return The indentation width for the physical line.
    [[nodiscard]] auto calculateIndentWidth(bool isFirstPhysicalLine) const noexcept -> int {
        if (!_leftAligned) {
            return 0;
        }
        return isFirstPhysicalLine ? _options.firstLineIndent() : _options.wrappedLineIndent();
    }

private:
    const BlockStringView &_text;
    int _width;
    const ParagraphOptions &_options;
    bool _leftAligned;
    int _lineBreakStartMarkWidth;
    int _lineBreakEndMarkWidth;
    int _paragraphEllipsisWidth;
    int _wordBreakMarkWidth;
};

}
