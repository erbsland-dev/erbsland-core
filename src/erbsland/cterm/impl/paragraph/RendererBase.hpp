// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Layout.hpp"

#include "../../ParagraphBackgroundMode.hpp"

namespace erbsland::cterm::impl::paragraph {

/// Shared paragraph placement and background-fill rules for painters and printers.
class RendererBase {
protected:
    /// Precomputed geometry for one rendered paragraph line.
    struct LinePlacement final {
        int textX = 0;        ///< The left column where the line text starts.
        int textWidth = 0;    ///< The display width of the line text.
        int endMarkX = 0;     ///< The left column where the line end mark starts.
        int endMarkWidth = 0; ///< The display width of the line end mark.
    };

protected:
    /// Create common state for paragraph renderers.
    RendererBase(
        const bgeo::Alignment alignment,
        const LayoutResult &layout,
        const BlockString &sourceText,
        const ParagraphOptions &options,
        ParagraphBackgroundMode backgroundMode) noexcept :
        _alignment{alignment},
        _layout{layout},
        _sourceText{sourceText},
        _options{options},
        _backgroundMode{backgroundMode} {}

public: // defaults/deletions
    ~RendererBase() = default;
    RendererBase(const RendererBase &) = delete;
    RendererBase(RendererBase &&) = delete;
    auto operator=(const RendererBase &) -> RendererBase & = delete;
    auto operator=(RendererBase &&) -> RendererBase & = delete;

protected:
    /// Get the paragraph alignment.
    [[nodiscard]] auto alignment() const noexcept -> bgeo::Alignment { return _alignment; }
    /// Get the completed paragraph layout.
    [[nodiscard]] auto layout() const noexcept -> const LayoutResult & { return _layout; }
    /// Get the source block string.
    [[nodiscard]] auto sourceText() const noexcept -> const BlockString & { return _sourceText; }
    /// Get the paragraph layout options.
    [[nodiscard]] auto options() const noexcept -> const ParagraphOptions & { return _options; }
    /// Test if background fill is required left of text.
    [[nodiscard]] auto usesLeftFill() const noexcept -> bool {
        return _backgroundMode == ParagraphBackgroundMode::WrappedLeft ||
            _backgroundMode == ParagraphBackgroundMode::WrappedBoth ||
            _backgroundMode == ParagraphBackgroundMode::FullBoth;
    }
    /// Test if background fill is required right of text.
    [[nodiscard]] auto usesRightFill() const noexcept -> bool {
        return _backgroundMode == ParagraphBackgroundMode::WrappedRight ||
            _backgroundMode == ParagraphBackgroundMode::WrappedBoth ||
            _backgroundMode == ParagraphBackgroundMode::FullRight ||
            _backgroundMode == ParagraphBackgroundMode::FullBoth;
    }
    /// Test if background fill is required right of one line.
    [[nodiscard]] auto usesRightFillForLine(const LayoutLine &line) const noexcept -> bool {
        if (_backgroundMode == ParagraphBackgroundMode::FullRight ||
            _backgroundMode == ParagraphBackgroundMode::FullBoth) {
            return true;
        }
        return line.wrapsToNext &&
            (_backgroundMode == ParagraphBackgroundMode::WrappedRight ||
                _backgroundMode == ParagraphBackgroundMode::WrappedBoth);
    }
    /// Calculate placement geometry for one layout line.
    [[nodiscard]] auto linePlacement(const LayoutLine &line, int x1, int width) const noexcept -> LinePlacement;

private:
    bgeo::Alignment _alignment;
    const LayoutResult &_layout;
    const BlockString &_sourceText;
    const ParagraphOptions &_options;
    ParagraphBackgroundMode _backgroundMode;
};

}
