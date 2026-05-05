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
    RendererBase(
        const bgeo::Alignment alignment,
        const LayoutResult &layout,
        const BlockStringView &sourceText,
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
    [[nodiscard]] auto alignment() const noexcept -> bgeo::Alignment { return _alignment; }
    [[nodiscard]] auto layout() const noexcept -> const LayoutResult & { return _layout; }
    [[nodiscard]] auto sourceText() const noexcept -> const BlockStringView & { return _sourceText; }
    [[nodiscard]] auto options() const noexcept -> const ParagraphOptions & { return _options; }
    [[nodiscard]] auto usesLeftFill() const noexcept -> bool {
        return _backgroundMode == ParagraphBackgroundMode::WrappedLeft ||
            _backgroundMode == ParagraphBackgroundMode::WrappedBoth ||
            _backgroundMode == ParagraphBackgroundMode::FullBoth;
    }
    [[nodiscard]] auto usesRightFill() const noexcept -> bool {
        return _backgroundMode == ParagraphBackgroundMode::WrappedRight ||
            _backgroundMode == ParagraphBackgroundMode::WrappedBoth ||
            _backgroundMode == ParagraphBackgroundMode::FullRight ||
            _backgroundMode == ParagraphBackgroundMode::FullBoth;
    }
    [[nodiscard]] auto usesRightFillForLine(const LayoutLine &line) const noexcept -> bool {
        if (_backgroundMode == ParagraphBackgroundMode::FullRight ||
            _backgroundMode == ParagraphBackgroundMode::FullBoth) {
            return true;
        }
        return line.wrapsToNext &&
            (_backgroundMode == ParagraphBackgroundMode::WrappedRight ||
                _backgroundMode == ParagraphBackgroundMode::WrappedBoth);
    }
    [[nodiscard]] auto linePlacement(const LayoutLine &line, int x1, int width) const noexcept -> LinePlacement;

private:
    bgeo::Alignment _alignment;
    const LayoutResult &_layout;
    const BlockStringView &_sourceText;
    const ParagraphOptions &_options;
    ParagraphBackgroundMode _backgroundMode;
};

}
