// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockTextOptions.hpp"

#include "../bgeo/BlockRectangle.hpp"

#include <utility>

namespace erbsland::cterm {

/// Describes a text block to render into a `Buffer`.
/// @note Creating and copying text instances is expensive. Please keep and reuse created instances.
class BlockText final {
public:
    /// Create an empty text description.
    BlockText() = default;
    /// Create a text description for the given content and target rectangle.
    /// @param text The text content to render.
    /// @param rect The target rectangle.
    /// @param alignment The text alignment inside the rectangle.
    BlockText(
        BlockString text,
        bgeo::BlockRectangle rect,
        const bgeo::Alignment alignment = bgeo::Alignment::TopLeft) noexcept :
        _text{std::move(text)}, _rectangle{rect}, _blockTextOptions(alignment) {}

    // defaults
    ~BlockText() = default;
    BlockText(const BlockText &) = default;
    BlockText(BlockText &&) = default;
    auto operator=(const BlockText &) -> BlockText & = default;
    auto operator=(BlockText &&) -> BlockText & = default;

public:
    /// Get the text content.
    [[nodiscard]] auto blockString() const noexcept -> const BlockString & { return _text; }
    /// Set the text content.
    void setBlockString(BlockString text) noexcept { _text = std::move(text); }
    /// Get the target rectangle.
    [[nodiscard]] auto rectangle() const noexcept -> const bgeo::BlockRectangle & { return _rectangle; }
    /// Set the target rectangle.
    void setRectangle(const bgeo::BlockRectangle rect) noexcept { _rectangle = rect; }
    /// Get the text options.
    [[nodiscard]] auto blockTextOptions() const noexcept -> const BlockTextOptions & { return _blockTextOptions; }
    /// Set the text options.
    void setBlockTextOptions(const BlockTextOptions &options) noexcept { _blockTextOptions = options; }

public: // wrappers around text options.
    /// @copydoc BlockTextOptions::colorSequence
    [[nodiscard]] auto colorSequence() const noexcept -> const ColorSequence & {
        return _blockTextOptions.colorSequence();
    }
    /// @copydoc BlockTextOptions::setColorSequence
    void setColorSequence(ColorSequence colorSequence) noexcept {
        _blockTextOptions.setColorSequence(std::move(colorSequence));
    }
    /// @copydoc BlockTextOptions::color
    [[nodiscard]] auto color() const noexcept -> Color { return _blockTextOptions.color(); }
    /// @copydoc BlockTextOptions::setColor
    void setColor(const Color color) noexcept { _blockTextOptions.setColor(color); }
    /// @copydoc BlockTextOptions::font
    [[nodiscard]] auto font() const noexcept -> const FontPtr & { return _blockTextOptions.font(); }
    /// @copydoc BlockTextOptions::setFont
    void setFont(const FontPtr &font) noexcept { _blockTextOptions.setFont(font); }
    /// @copydoc BlockTextOptions::animation
    [[nodiscard]] auto animation() const noexcept -> BlockTextAnimation { return _blockTextOptions.animation(); }
    /// @copydoc BlockTextOptions::setAnimation
    void setAnimation(const BlockTextAnimation animation) noexcept { _blockTextOptions.setAnimation(animation); }

public: // wrappers for common paragraph options
    /// @copydoc ParagraphOptions::alignment
    [[nodiscard]] auto alignment() const noexcept -> bgeo::Alignment { return _blockTextOptions.alignment(); }
    /// @copydoc ParagraphOptions::setAlignment
    void setAlignment(const bgeo::Alignment alignment) noexcept { _blockTextOptions.setAlignment(alignment); }
    /// @copydoc ParagraphOptions::lineIndent
    [[nodiscard]] auto lineIndent() const noexcept -> int { return _blockTextOptions.lineIndent(); }
    /// @copydoc ParagraphOptions::setLineIndent
    void setLineIndent(const int indent) noexcept { _blockTextOptions.setLineIndent(indent); }
    /// @copydoc ParagraphOptions::firstLineIndent
    [[nodiscard]] auto firstLineIndent() const noexcept -> int { return _blockTextOptions.firstLineIndent(); }
    /// @copydoc ParagraphOptions::setFirstLineIndent
    void setFirstLineIndent(const int indent) noexcept { _blockTextOptions.setFirstLineIndent(indent); }
    /// @copydoc ParagraphOptions::wrappedLineIndent
    [[nodiscard]] auto wrappedLineIndent() const noexcept -> int { return _blockTextOptions.wrappedLineIndent(); }
    /// @copydoc ParagraphOptions::setWrappedLineIndent
    void setWrappedLineIndent(const int indent) noexcept { _blockTextOptions.setWrappedLineIndent(indent); }
    /// @copydoc ParagraphOptions::margins
    [[nodiscard]] auto margins() const noexcept -> const bgeo::BlockMargins & { return _blockTextOptions.margins(); }
    /// @copydoc ParagraphOptions::setMargins
    void setMargins(const bgeo::BlockMargins margins) noexcept { _blockTextOptions.setMargins(margins); }
    /// @copydoc ParagraphOptions::backgroundMode
    [[nodiscard]] auto backgroundMode() const noexcept -> ParagraphBackgroundMode {
        return _blockTextOptions.backgroundMode();
    }
    /// @copydoc ParagraphOptions::setBackgroundMode
    void setBackgroundMode(const ParagraphBackgroundMode backgroundMode) noexcept {
        _blockTextOptions.setBackgroundMode(backgroundMode);
    }
    /// @copydoc ParagraphOptions::lineBreakEndMark
    [[nodiscard]] auto lineBreakEndMark() const noexcept -> const BlockString & {
        return _blockTextOptions.lineBreakEndMark();
    }
    /// @copydoc ParagraphOptions::setLineBreakEndMark
    void setLineBreakEndMark(BlockString mark) { _blockTextOptions.setLineBreakEndMark(std::move(mark)); }
    /// @copydoc ParagraphOptions::lineBreakStartMark
    [[nodiscard]] auto lineBreakStartMark() const noexcept -> const BlockString & {
        return _blockTextOptions.lineBreakStartMark();
    }
    /// @copydoc ParagraphOptions::setLineBreakStartMark
    void setLineBreakStartMark(BlockString mark) { _blockTextOptions.setLineBreakStartMark(std::move(mark)); }
    /// @copydoc ParagraphOptions::paragraphSpacing
    [[nodiscard]] auto paragraphSpacing() const noexcept -> ParagraphSpacing {
        return _blockTextOptions.paragraphSpacing();
    }
    /// @copydoc ParagraphOptions::setParagraphSpacing
    void setParagraphSpacing(const ParagraphSpacing spacing) noexcept {
        _blockTextOptions.setParagraphSpacing(spacing);
    }
    /// @copydoc ParagraphOptions::wordSeparators
    [[nodiscard]] auto wordSeparators() const -> text::U32String { return _blockTextOptions.wordSeparators(); }
    /// @copydoc ParagraphOptions::setWordSeparators
    void setWordSeparators(const text::U32String &separators) { _blockTextOptions.setWordSeparators(separators); }
    /// @copydoc ParagraphOptions::wordBreakMark
    [[nodiscard]] auto wordBreakMark() const noexcept -> const Block & { return _blockTextOptions.wordBreakMark(); }
    /// @copydoc ParagraphOptions::setWordBreakMark
    void setWordBreakMark(Block mark) noexcept { _blockTextOptions.setWordBreakMark(mark); }
    /// @copydoc ParagraphOptions::maximumLineWraps
    [[nodiscard]] auto maximumLineWraps() const noexcept -> int { return _blockTextOptions.maximumLineWraps(); }
    /// @copydoc ParagraphOptions::setMaximumLineWraps
    void setMaximumLineWraps(const int lines) noexcept { _blockTextOptions.setMaximumLineWraps(std::max(lines, 0)); }
    /// @copydoc ParagraphOptions::paragraphEllipsisMark
    [[nodiscard]] auto paragraphEllipsisMark() const noexcept -> const BlockString & {
        return _blockTextOptions.paragraphEllipsisMark();
    }
    /// @copydoc ParagraphOptions::setParagraphEllipsisMark
    void setParagraphEllipsisMark(BlockString mark) noexcept {
        _blockTextOptions.setParagraphEllipsisMark(std::move(mark));
    }
    /// @copydoc ParagraphOptions::tabStops
    [[nodiscard]] auto tabStops() const noexcept -> const std::vector<int> & { return _blockTextOptions.tabStops(); }
    /// @copydoc ParagraphOptions::setTabStops
    void setTabStops(std::vector<int> tabStops) noexcept { _blockTextOptions.setTabStops(std::move(tabStops)); }
    /// @copydoc ParagraphOptions::tabOverflowBehavior
    [[nodiscard]] auto tabOverflowBehavior() const noexcept -> TabOverflowBehavior {
        return _blockTextOptions.tabOverflowBehavior();
    }
    /// @copydoc ParagraphOptions::setTabOverflowBehavior
    void setTabOverflowBehavior(const TabOverflowBehavior behavior) noexcept {
        _blockTextOptions.setTabOverflowBehavior(behavior);
    }
    /// @copydoc ParagraphOptions::onError
    [[nodiscard]] auto onError() const noexcept -> ParagraphOnError { return _blockTextOptions.onError(); }
    /// @copydoc ParagraphOptions::setOnError
    void setOnError(const ParagraphOnError onError) noexcept { _blockTextOptions.setOnError(onError); }

private:
    BlockString _text{};
    bgeo::BlockRectangle _rectangle{};
    BlockTextOptions _blockTextOptions;
};

}
