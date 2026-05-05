// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../BlockCount.hpp"
#include "../../BlockIndex.hpp"
#include "../../Color.hpp"

#include <cstdint>

namespace erbsland::cterm::impl::paragraph {

/// One logical piece of a rendered paragraph line.
class LayoutFragment final {
public:
    /// The kind of fragment represented by this entry.
    enum class Type : uint8_t {
        SourceRange,        ///< A range in the original source text.
        Spaces,             ///< One or more rendered spaces.
        LineBreakStartMark, ///< The configured wrapped-line start mark.
        WordBreakMark,      ///< The configured split-word marker.
        ParagraphEllipsis,  ///< The configured truncation marker.
    };

    /// Create one generated literal fragment.
    /// @param type The fragment type.
    /// @param width The rendered display width of the fragment.
    LayoutFragment(const Type type, const int width) noexcept : _type{type}, _width{width} {}

    /// Create one source-text fragment.
    /// @param startIndex The start index in the source text.
    /// @param length The number of source characters.
    /// @param width The rendered display width of the fragment.
    LayoutFragment(const BlockIndex startIndex, const BlockCount length, const int width) noexcept :
        _startIndex{startIndex}, _length{length}, _width{width} {}

    /// Create one generated fragment with color information.
    /// @param type The fragment type.
    /// @param width The rendered display width of the fragment.
    /// @param color The fragment color.
    LayoutFragment(const Type type, const int width, const Color color) noexcept :
        _type{type}, _width{width}, _color{color} {}

    LayoutFragment() = default;

public:
    /// Access the fragment type.
    [[nodiscard]] auto type() const noexcept -> Type { return _type; }
    /// Access the start index in the source text for source-range fragments.
    [[nodiscard]] auto startIndex() const noexcept -> BlockIndex { return _startIndex; }
    /// Access the number of source characters for source-range fragments.
    [[nodiscard]] auto length() const noexcept -> BlockCount { return _length; }
    /// Access the rendered display width of the fragment.
    [[nodiscard]] auto width() const noexcept -> int { return _width; }
    /// Access the fragment color for generated spaces.
    [[nodiscard]] auto color() const noexcept -> Color { return _color; }

    /// Increase the number of source characters for this fragment.
    /// @param length The additional source character count.
    void addLength(const BlockCount length) noexcept { _length += length; }
    /// Increase the rendered display width of this fragment.
    /// @param width The additional rendered width.
    void addWidth(const int width) noexcept { _width += width; }

private:
    Type _type = Type::SourceRange;    ///< The fragment type.
    BlockIndex _startIndex{};          ///< The start index in the source text for source-range fragments.
    BlockCount _length{};              ///< The number of source characters for source-range fragments.
    int _width = 0;                    ///< The rendered display width of the fragment.
    Color _color{Background::Default}; ///< The fragment color for generated spaces.
};

}
