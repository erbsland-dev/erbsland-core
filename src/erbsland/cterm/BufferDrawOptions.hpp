// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockCombinationStyle.hpp"

#include "../bgeo/Alignment.hpp"
#include "../bgeo/BlockRectangle.hpp"

namespace erbsland::cterm {

/// Options for drawing a buffer onto another buffer.
class BufferDrawOptions final {
public:
    /// Create default draw options with a target position.
    /// @param targetPos The target position.
    explicit BufferDrawOptions(const bgeo::BlockPosition targetPos) noexcept :
        _targetRect(targetPos, bgeo::BlockSize{}) {}

    /// Create default draw options with a target rectangle and alignment.
    /// @param targetRect The target rectangle.
    /// @param sourceRect The source rectangle.
    BufferDrawOptions(const bgeo::BlockRectangle targetRect, const bgeo::BlockRectangle sourceRect) noexcept :
        _targetRect(targetRect), _sourceRect(sourceRect) {}

    // defaults
    BufferDrawOptions() = default;
    BufferDrawOptions(const BufferDrawOptions &) = default;
    BufferDrawOptions(BufferDrawOptions &&) = default;
    auto operator=(const BufferDrawOptions &) -> BufferDrawOptions & = default;
    auto operator=(BufferDrawOptions &&) -> BufferDrawOptions & = default;

public:
    /// Get the target rectangle.
    /// @return The target rectangle.
    [[nodiscard]] auto targetRect() const noexcept -> const bgeo::BlockRectangle & { return _targetRect; }
    /// Test if the target is just a position.
    [[nodiscard]] auto isTargetPosition() const noexcept -> bool { return _targetRect.size().isZero(); }
    /// Set the target rectangle.
    /// @param rect The target rectangle.
    void setTargetRect(const bgeo::BlockRectangle &rect) noexcept { _targetRect = rect; }
    /// Get the source rectangle.
    /// @return The source rectangle.
    [[nodiscard]] auto sourceRect() const noexcept -> const bgeo::BlockRectangle & { return _sourceRect; }
    /// Test if the whole source shall be used.
    [[nodiscard]] auto useFullSource() const noexcept -> bool { return _sourceRect.size().isZero(); }
    /// Set the source rectangle.
    /// @param rect The source rectangle.
    void setSourceRect(const bgeo::BlockRectangle &rect) noexcept { _sourceRect = rect; }
    /// Get the combination style.
    /// If a combination style is set, the characters from the source buffer are combined with the target buffer.
    /// A combination style overrides the `overrideColors` setting.
    /// @return The combination style.
    [[nodiscard]] auto combinationStyle() const noexcept -> const BlockCombinationStylePtr & {
        return _combinationStyle;
    }
    /// Set the combination style.
    /// @param style The combination style.
    void setCombinationStyle(const BlockCombinationStylePtr &style) noexcept { _combinationStyle = style; }
    /// Test if colors should be overwritten 1:1 in the target buffer.
    /// If this is set to `true`, even `Inherit` colors are written as they are in the target buffer.
    /// If this is set to `false`, `Inherit` colors are combined with `useBaseColor(<from target buffer>)`.
    [[nodiscard]] auto overwriteColors() const noexcept -> bool { return _overwrite; }
    /// Set if colors should be overwritten 1:1 in the target buffer.
    /// @param overwrite `true` to overwrite colors in the target buffer.
    void setOverwriteColors(const bool overwrite) noexcept { _overwrite = overwrite; }

private:
    bgeo::BlockRectangle _targetRect;           ///< A target rectangle, zero size if just a position.
    bgeo::BlockRectangle _sourceRect;           ///< A source rectangle, zero size if the whole buffer shall be used.
    BlockCombinationStylePtr _combinationStyle; ///< An optional combination style.
    bool _overwrite{false};                     ///< Flag if `Inherit` colors should be interpreted as `Default`.
};

}
