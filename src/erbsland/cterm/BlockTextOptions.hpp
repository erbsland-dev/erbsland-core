// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockTextAnimation.hpp"
#include "BlockTextOptions_fwd.hpp"
#include "ColorSequence.hpp"
#include "Font_fwd.hpp"
#include "ParagraphOptions.hpp"

#include "../geometry/Alignment.hpp"

namespace erbsland::cterm {

/// Options for text rendering.
///
/// This class combines text color, font, animation, and paragraph layout settings into one reusable configuration
/// object that can be attached to a `BlockText` instance.
///
/// @note Creating and copying text instances is expensive. Please keep and reuse created instances.
class BlockTextOptions final {
public:
    /// Create a text options instance with the given alignment.
    /// @param alignment The text alignment inside the rectangle.
    explicit BlockTextOptions(const geometry::Alignment alignment) noexcept : _paragraphOptions(alignment) {}

    // defaults
    BlockTextOptions() = default;
    ~BlockTextOptions() = default;
    BlockTextOptions(const BlockTextOptions &) = default;
    BlockTextOptions(BlockTextOptions &&) = default;
    auto operator=(const BlockTextOptions &) -> BlockTextOptions & = default;
    auto operator=(BlockTextOptions &&) -> BlockTextOptions & = default;

public:
    /// Get the optional text color sequence.
    [[nodiscard]] auto colorSequence() const noexcept -> const ColorSequence & { return _colorSequence; }
    /// Set the text color sequence.
    /// @param colorSequence The sequence of colors to apply while rendering the text.
    void setColorSequence(ColorSequence colorSequence) noexcept { _colorSequence = std::move(colorSequence); }
    /// Get the first text color from the configured sequence.
    /// @return The first sequence color, or the inherited color if no sequence is configured.
    [[nodiscard]] auto color() const noexcept -> Color {
        if (_colorSequence.empty()) {
            return {};
        }
        return _colorSequence.color(0);
    }
    /// Set a single text color.
    /// @param color The single base color to use for the rendered text.
    void setColor(const Color color) noexcept { _colorSequence = ColorSequence{color}; }
    /// Get the character attributes
    [[nodiscard]] auto blockAttributes() const noexcept -> BlockAttributes { return _blockAttributes; }
    /// Set the char attributes
    void setBlockAttributes(const BlockAttributes attributes) noexcept { _blockAttributes = attributes; }
    /// Get the optional font. If this is empty, regular text rendering is used.
    [[nodiscard]] auto font() const noexcept -> const FontPtr & { return _font; }
    /// Set the font.
    /// @param font The font to use, or an empty pointer for regular terminal text rendering.
    void setFont(const FontPtr &font) noexcept { _font = font; }
    /// Get the animation mode.
    [[nodiscard]] auto animation() const noexcept -> BlockTextAnimation { return _animation; }
    /// Set the animation mode.
    /// @param animation The animation mode to apply while rendering the text.
    void setAnimation(const BlockTextAnimation animation) noexcept { _animation = animation; }
    /// Get the paragraph options.
    /// @return The paragraph layout settings used for this text.
    [[nodiscard]] auto paragraphOptions() const noexcept -> const ParagraphOptions & { return _paragraphOptions; }
    /// Set the paragraph options.
    /// @param options The paragraph layout settings to copy into this text configuration.
    void setParagraphOptions(const ParagraphOptions &options) noexcept { _paragraphOptions = options; }

public: // wrappers for common paragraph options
    /// @copydoc ParagraphOptions::alignment
    [[nodiscard]] auto alignment() const noexcept -> geometry::Alignment { return _paragraphOptions.alignment(); }
    /// @copydoc ParagraphOptions::setAlignment
    void setAlignment(const geometry::Alignment alignment) noexcept { _paragraphOptions.setAlignment(alignment); }
    /// @copydoc ParagraphOptions::lineIndent
    [[nodiscard]] auto lineIndent() const noexcept -> int { return _paragraphOptions.lineIndent(); }
    /// @copydoc ParagraphOptions::setLineIndent
    void setLineIndent(const int indent) noexcept { _paragraphOptions.setLineIndent(indent); }
    /// @copydoc ParagraphOptions::firstLineIndent
    [[nodiscard]] auto firstLineIndent() const noexcept -> int { return _paragraphOptions.firstLineIndent(); }
    /// @copydoc ParagraphOptions::setFirstLineIndent
    void setFirstLineIndent(const int indent) noexcept { _paragraphOptions.setFirstLineIndent(indent); }
    /// @copydoc ParagraphOptions::wrappedLineIndent
    [[nodiscard]] auto wrappedLineIndent() const noexcept -> int { return _paragraphOptions.wrappedLineIndent(); }
    /// @copydoc ParagraphOptions::setWrappedLineIndent
    void setWrappedLineIndent(const int indent) noexcept { _paragraphOptions.setWrappedLineIndent(indent); }
    /// @copydoc ParagraphOptions::margins
    [[nodiscard]] auto margins() const noexcept -> const block::Margins & { return _paragraphOptions.margins(); }
    /// @copydoc ParagraphOptions::setMargins
    void setMargins(const block::Margins margins) noexcept { _paragraphOptions.setMargins(margins); }
    /// @copydoc ParagraphOptions::backgroundMode
    [[nodiscard]] auto backgroundMode() const noexcept -> ParagraphBackgroundMode {
        return _paragraphOptions.backgroundMode();
    }
    /// @copydoc ParagraphOptions::setBackgroundMode
    void setBackgroundMode(const ParagraphBackgroundMode backgroundMode) noexcept {
        _paragraphOptions.setBackgroundMode(backgroundMode);
    }
    /// @copydoc ParagraphOptions::lineBreakEndMark
    [[nodiscard]] auto lineBreakEndMark() const noexcept -> const BlockString & {
        return _paragraphOptions.lineBreakEndMark();
    }
    /// @copydoc ParagraphOptions::setLineBreakEndMark
    void setLineBreakEndMark(BlockString mark) { _paragraphOptions.setLineBreakEndMark(std::move(mark)); }
    /// @copydoc ParagraphOptions::lineBreakStartMark
    [[nodiscard]] auto lineBreakStartMark() const noexcept -> const BlockString & {
        return _paragraphOptions.lineBreakStartMark();
    }
    /// @copydoc ParagraphOptions::setLineBreakStartMark
    void setLineBreakStartMark(BlockString mark) { _paragraphOptions.setLineBreakStartMark(std::move(mark)); }
    /// @copydoc ParagraphOptions::paragraphSpacing
    [[nodiscard]] auto paragraphSpacing() const noexcept -> ParagraphSpacing {
        return _paragraphOptions.paragraphSpacing();
    }
    /// @copydoc ParagraphOptions::setParagraphSpacing
    void setParagraphSpacing(const ParagraphSpacing spacing) noexcept {
        _paragraphOptions.setParagraphSpacing(spacing);
    }
    /// @copydoc ParagraphOptions::wordSeparators
    [[nodiscard]] auto wordSeparators() const -> text::U32String { return _paragraphOptions.wordSeparators(); }
    /// @copydoc ParagraphOptions::setWordSeparators
    void setWordSeparators(const text::U32String &separators) { _paragraphOptions.setWordSeparators(separators); }
    /// @copydoc ParagraphOptions::wordBreakMark
    [[nodiscard]] auto wordBreakMark() const noexcept -> const Block & { return _paragraphOptions.wordBreakMark(); }
    /// @copydoc ParagraphOptions::setWordBreakMark
    void setWordBreakMark(Block mark) noexcept { _paragraphOptions.setWordBreakMark(mark); }
    /// @copydoc ParagraphOptions::maximumLineWraps
    [[nodiscard]] auto maximumLineWraps() const noexcept -> int { return _paragraphOptions.maximumLineWraps(); }
    /// @copydoc ParagraphOptions::setMaximumLineWraps
    void setMaximumLineWraps(const int lines) noexcept { _paragraphOptions.setMaximumLineWraps(std::max(lines, 0)); }
    /// @copydoc ParagraphOptions::paragraphEllipsisMark
    [[nodiscard]] auto paragraphEllipsisMark() const noexcept -> const BlockString & {
        return _paragraphOptions.paragraphEllipsisMark();
    }
    /// @copydoc ParagraphOptions::setParagraphEllipsisMark
    void setParagraphEllipsisMark(BlockString mark) noexcept {
        _paragraphOptions.setParagraphEllipsisMark(std::move(mark));
    }
    /// @copydoc ParagraphOptions::tabStops
    [[nodiscard]] auto tabStops() const noexcept -> const std::vector<int> & { return _paragraphOptions.tabStops(); }
    /// @copydoc ParagraphOptions::setTabStops
    void setTabStops(std::vector<int> tabStops) noexcept { _paragraphOptions.setTabStops(std::move(tabStops)); }
    /// @copydoc ParagraphOptions::tabOverflowBehavior
    [[nodiscard]] auto tabOverflowBehavior() const noexcept -> TabOverflowBehavior {
        return _paragraphOptions.tabOverflowBehavior();
    }
    /// @copydoc ParagraphOptions::setTabOverflowBehavior
    void setTabOverflowBehavior(const TabOverflowBehavior behavior) noexcept {
        _paragraphOptions.setTabOverflowBehavior(behavior);
    }
    /// @copydoc ParagraphOptions::onError
    [[nodiscard]] auto onError() const noexcept -> ParagraphOnError { return _paragraphOptions.onError(); }
    /// @copydoc ParagraphOptions::setOnError
    void setOnError(const ParagraphOnError onError) noexcept { _paragraphOptions.setOnError(onError); }

private:
    /// Access the global static default values.
    [[nodiscard]] static auto defaultOptions() noexcept -> const BlockTextOptions &;

private:
    ColorSequence _colorSequence;
    BlockAttributes _blockAttributes;
    FontPtr _font;
    BlockTextAnimation _animation{BlockTextAnimation::None};
    ParagraphOptions _paragraphOptions;
};

}
