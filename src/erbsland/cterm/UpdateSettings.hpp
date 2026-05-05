// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "BlockString.hpp"

#include "../bgeo/BlockSize.hpp"

#include <utility>

namespace erbsland::cterm {

class BufferViewBase;

/// Settings controlling how `Terminal::updateScreen()` renders a buffer.
class UpdateSettings final {
public:
    /// Create default screen update settings.
    UpdateSettings() = default;

    // defaults
    ~UpdateSettings() = default;
    UpdateSettings(const UpdateSettings &) = default;
    UpdateSettings(UpdateSettings &&) noexcept = default;
    auto operator=(const UpdateSettings &) -> UpdateSettings & = default;
    auto operator=(UpdateSettings &&) -> UpdateSettings & = default;

public:
    /// Get the minimum terminal size required for rendering the buffer.
    /// @return The minimum supported terminal size.
    [[nodiscard]] auto minimumSize() const noexcept -> bgeo::BlockSize;
    /// Set the minimum terminal size required for rendering the buffer.
    /// @param minimum The minimum supported terminal size.
    void setMinimumSize(bgeo::BlockSize minimumSize) noexcept;
    /// Get the background character used if the terminal is too small.
    [[nodiscard]] auto minimumSizeBackground() const noexcept -> const Block &;
    /// Set the background fill character when the terminal is too small.
    /// @param character The background character
    void setMinimumSizeBackground(Block character) noexcept;
    /// Get the message displayed if the terminal size is too small
    [[nodiscard]] auto minimumSizeMessage() const noexcept -> const BlockString &;
    /// Set the message displayed if the terminal is too small.
    /// @param message The displayed message.
    void setMinimumSizeMessage(BlockString message) noexcept;
    /// Check if crop marks are enabled.
    [[nodiscard]] auto showCropMarks() const noexcept -> bool;
    /// Enable or disable crop marks.
    /// @param showCropMarks `true` to render crop marks for truncated content.
    void setShowCropMarks(bool showCropMarks) noexcept;
    /// Get the mark rendered when content is cropped on the right.
    [[nodiscard]] auto cropMarkRight() const noexcept -> const Block &;
    /// Set the mark rendered if the content is cropped on the right.
    /// @param cropMarkRight The right crop mark.
    void setCropMarkRight(Block cropMarkRight) noexcept;
    /// Get the mark rendered in the bottom right corner when content is cropped.
    [[nodiscard]] auto cropMarkBottomRight() const noexcept -> const Block &;
    /// Set the mark in the bottom right corner if content is cropped on the bottom and right.
    /// @param cropMarkBottomRight The bottom-right crop mark.
    void setCropMarkBottomRight(Block cropMarkBottomRight) noexcept;
    /// Get the mark rendered when content is cropped at the bottom.
    [[nodiscard]] auto cropMarkBottom() const noexcept -> const Block &;
    /// Set the mark rendered if the content is cropped at the bottom.
    /// @param cropMarkBottom The bottom crop mark.
    void setCropMarkBottom(Block cropMarkBottom) noexcept;
    /// Test if the update shall switch to the alternate screen buffer.
    [[nodiscard]] auto switchToAlternateBuffer() const noexcept -> bool;
    /// Enable or disable switching to the alternate screen buffer.
    /// @param switchToAlternateBuffer `true` to switch to the alternate screen buffer.
    void setSwitchToAlternateBuffer(bool switchToAlternateBuffer) noexcept;

public: // tools
    /// Apply these settings to a BufferView
    void applyTo(BufferViewBase &view) const noexcept;
    /// Shared default value.
    [[nodiscard]] static auto defaultSettings() noexcept -> const UpdateSettings &;

public: // compatibility
    /// Set the minimum-size background character through the legacy name.
    /// @deprecated Use `setMinimumSizeBackground()` instead.
    /// @param minimumSizeMark The background character shown when the terminal is too small.
    [[deprecated("Use setMinimumSizeBackground() instead.")]]
    void setMinimumSizeMark(Block minimumSizeMark) noexcept;
    /// Get the minimum-size background character through the legacy name.
    /// @deprecated Use `minimumSizeBackground()` instead.
    /// @return The background character shown when the terminal is too small.
    [[deprecated("Use minimumSizeBackground() instead.")]] [[nodiscard]] auto minimumSizeMark() const noexcept
        -> const Block &;
    /// Construct update settings using the deprecated aggregate-style compatibility constructor.
    /// @deprecated Construct `UpdateSettings{}` and configure it with setters instead.
    /// @param minimum The minimum terminal size required for normal rendering.
    /// @param minimumSizeBackground The fill character for the size-too-small background.
    /// @param showCropMarks `true` to show crop marks for truncated content.
    /// @param cropMarkRight The crop mark to draw at the right edge.
    /// @param cropMarkBottom The crop mark to draw at the bottom edge.
    [[deprecated("Construct and use setters to change defaults.")]]
    UpdateSettings(
        bgeo::BlockSize minimumSize,
        Block minimumSizeBackground,
        bool showCropMarks,
        Block cropMarkRight,
        Block cropMarkBottom) noexcept;

private:
    bgeo::BlockSize _minimumSize{};               ///< The minimum required terminal size.
    Block _minimumSizeBackground{Block::space()}; ///< Background character if the terminal size is too small.
    BlockString _minimumSizeMessage;     ///< A message that is displayed centered if the terminal size is too small.
    bool _showCropMarks{false};          ///< If crop marks are enabled.
    Block _cropMarkRight{U'▶'};          ///< Crop mark at the right edge.
    Block _cropMarkBottomRight{U'◢'};    ///< Crop mark in the bottom-right corner.
    Block _cropMarkBottom{U'▼'};         ///< Crop mark at the bottom edge.
    bool _switchToAlternateBuffer{true}; ///< Switches to the alternate screen buffer if supported.
};

}
