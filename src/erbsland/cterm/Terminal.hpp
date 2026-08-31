// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Backend.hpp"
#include "Block.hpp"
#include "BlockStringEditor.hpp"
#include "Color.hpp"
#include "CursorWriter.hpp"
#include "MoveMode.hpp"
#include "ParagraphOptions.hpp"
#include "Terminal_fwd.hpp"
#include "TerminalFlags.hpp"
#include "TerminalOutputGuard.hpp"
#include "TypeTraits.hpp"
#include "UpdateSettings.hpp"
#include "WritableBuffer.hpp"

#include "impl/InputBackend.hpp"
#include "impl/LineBuffer.hpp"

#include "../bgeo/BlockSize.hpp"

#include <functional>
#include <memory>

namespace erbsland::cterm {

/// High-level terminal interface for screen control, color output, and key input.
class Terminal final : public CursorWriter {
    /// The minimum supported size of a terminal.
    constexpr static auto cMinimumSize = bgeo::BlockSize{1, 1};
    /// The maximum supported size of a terminal.
    constexpr static auto cMaximumSize = bgeo::BlockSize{2048, 2048};

public:
    /// Callback invoked when the drawable terminal size changes or is initialized.
    using ScreenSizeChangedCallback = std::function<void(bgeo::BlockSize)>;

    /// Screen clearing strategy used between rendered frames.
    enum class RefreshMode : uint8_t {
        /// Do not emit cursor or clear-screen control sequences automatically.
        Keep,
        /// Clear the full screen before rendering the next frame.
        Clear,
        /// Move the cursor to the top-left corner before rendering the next frame.
        Overwrite,
    };

    /// The output mode for the terminal.
    enum class OutputMode : uint8_t {
        /// Only output plain text with no colors and no cursor control
        BlockText,
        /// Full control over the terminal, including colors and cursor control.
        FullControl,
    };

public:
    /// Create a new terminal instance with default values.
    explicit Terminal();
    /// Create a new terminal instance.
    /// @param flags The terminal flags to use.
    explicit Terminal(TerminalFlags flags);
    /// Create a new terminal instance.
    /// The size is automatically bounded to the minimum and maximum supported sizes.
    /// @param size The fallback terminal size used when automatic detection is unavailable.
    /// @param flags The terminal flags to use.
    explicit Terminal(bgeo::BlockSize size, TerminalFlags flags = {});
    /// Create a new terminal instance with a custom backend.
    /// The size is automatically bounded to the minimum and maximum supported sizes.
    /// @param backend The backend to use for the terminal.
    /// @param size The fallback terminal size used when automatic detection is unavailable.
    explicit Terminal(BackendPtr backend, bgeo::BlockSize size = {80, 25});

public: // implements CursorWriter
    using CursorWriter::setColor;
    using CursorWriter::write;
    using CursorWriter::writeLineBreak;
    [[nodiscard]] auto size() const noexcept -> bgeo::BlockSize override { return _size; }
    [[nodiscard]] auto color() const noexcept -> Color override;
    [[nodiscard]] auto blockAttributes() const noexcept -> BlockAttributes override;
    void setColor(Color color) noexcept override;
    void setBlockAttributes(BlockAttributes attributes) noexcept override;
    void setForeground(Foreground color) noexcept override;
    void setBackground(Background color) noexcept override;
    [[nodiscard]] auto supportedBlockAttributes() const noexcept -> BlockAttributes override;
    void moveLeft(bgeo::BlockCoordinate count) noexcept override;
    void moveRight(bgeo::BlockCoordinate count) noexcept override;
    void moveUp(bgeo::BlockCoordinate count) noexcept override;
    void moveDown(bgeo::BlockCoordinate count) noexcept override;
    void moveTo(bgeo::BlockPosition pos) noexcept override;
    void moveHome() noexcept override;
    void moveCursor(bgeo::BlockPosition posOrDelta, MoveMode mode) noexcept override;
    void setAutoWrap(bool enabled) noexcept override;
    void setCursorVisible(bool visible) noexcept override;
    void write(const Block &character) noexcept override;
    void write(const BlockString &str) noexcept override;
    void writeResolved(const Block &character) noexcept override;
    void writeResolved(const BlockString &str) noexcept override;
    void write(const ReadableBuffer &buffer) noexcept override;
    void writeLineBreak() noexcept override;

public: // settings
    /// Modify the size of the terminal.
    /// The size is automatically bounded to the minimum and maximum supported sizes.
    /// If size detection is enabled, the terminal size will be automatically detected and updated.
    /// @param size The new terminal size.
    void setSize(bgeo::BlockSize size) noexcept;
    /// Get the refresh mode.
    [[nodiscard]] auto refreshMode() const noexcept -> RefreshMode { return _refreshMode; }
    /// Set the refresh mode.
    /// @param mode The screen refresh strategy to use.
    void setRefreshMode(const RefreshMode mode) noexcept { _refreshMode = mode; }
    /// Get the current output mode for the terminal.
    [[nodiscard]] auto outputMode() const noexcept -> OutputMode { return _outputMode; }
    /// Set the output mode.
    /// Switching to `OutputMode::BlockText` disables size detection, refresh modes, and back-buffer updates.
    /// @param outputMode The output mode to set.
    void setOutputMode(OutputMode outputMode) noexcept;
    /// Check whether dynamic terminal size detection is enabled.
    [[nodiscard]] auto sizeDetectionEnabled() const noexcept -> bool;
    /// Set if dynamic terminal size detection is enabled.
    /// Can only be enabled while the output mode is `OutputMode::FullControl`.
    /// @param enabled `true` to enable automatic size detection.
    void setSizeDetectionEnabled(bool enabled) noexcept;
    /// Check whether line buffering is enabled for incremental writes.
    /// @return `true` if text output is collected until a newline or `flush()`.
    [[nodiscard]] auto lineBufferEnabled() const noexcept -> bool;
    /// Enable or disable line buffering for incremental writes.
    /// When enabled, output is accumulated until a newline or `flush()` is reached.
    /// Line buffering can only be enabled if the backend supports both color and cursor ANSI codes.
    /// @param enabled `true` to enable buffered writes.
    void setLineBufferEnabled(bool enabled) noexcept;
    /// Check whether the compatibility safe margin is enabled.
    /// @return `true` if one column and one row are reserved from the detected terminal size.
    [[nodiscard]] auto safeMarginEnabled() const noexcept -> bool;
    /// Enable or disable the compatibility safe margin.
    /// When enabled, the reported drawable size is reduced by one column and one row.
    /// Disable this only when the terminal should use its full detected size and newline-free screen updates.
    /// @param enabled `true` to reserve one column and one row from the terminal size.
    void setSafeMarginEnabled(bool enabled) noexcept;
    /// Check whether the optional back buffer is enabled for smart overwrite updates.
    /// @return `true` if `updateScreen()` keeps the previous rendered frame for diff-based updates.
    [[nodiscard]] auto backBufferEnabled() const noexcept -> bool;
    /// Enable or disable the optional back buffer used by smart overwrite updates.
    /// Enabling the back buffer forces the next `updateScreen()` call to redraw the full frame once.
    /// Can only be enabled while the output mode is `OutputMode::FullControl`.
    /// @param enabled `true` to enable the back buffer.
    void setBackBufferEnabled(bool enabled) noexcept;
    /// Set a custom backend for the terminal.
    /// @param backend The backend to use for the terminal. If `nullptr` is passed, the default backend is restored.
    void setBackend(BackendPtr backend) noexcept;

public: // input handling.
    /// Access the input interface.
    /// @return The platform-specific input backend owned by this terminal.
    [[nodiscard]] auto input() noexcept -> Input &;

public: // initialization
    /// Initialize the console once before the application starts.
    /// Applies platform-specific setup, optionally clears the screen, and tests for the initial screen size.
    /// Also hides the cursor by default, as it is usually only made visible when the user makes input.
    /// Call this at the start of your application.
    void initializeScreen() noexcept;
    /// Check whether an interactive terminal is attached to the process.
    /// Call this after `initializeScreen()` to see if screen-size detection and terminal control features are active.
    /// @return `true` if the backend detected an interactive terminal.
    [[nodiscard]] auto isInteractive() const noexcept -> bool;
    /// Detect terminal resize changes.
    /// After calling this method, `size()` returns a safe size for the terminal.
    void testScreenSize() noexcept;
    /// Restore terminal settings when the application is quit.
    /// Call this at the end of your application.
    /// This should restore the terminal to its original state, including cursor visibility and any other
    /// settings that were modified during initialization.
    void restoreScreen() noexcept;

public: // screen handling
    /// Exclusively synchronize a sequence of output operations on this terminal.
    /// Individual terminal calls stay independently thread-safe and do not acquire this optional guard.
    /// @return A move-only guard retaining the recursive output lock until its destruction.
    [[nodiscard]] auto synchronizeOutput() const -> TerminalOutputGuard;
    /// Clears the screen.
    /// In `OutputMode::BlockText`, this method has no effect.
    /// If you need the screen cleared immediately, call `flush()` after this method.
    void clearScreen() noexcept override;
    /// Test if the alternate screen is active.
    /// This is no terminal detection, it just returns the internal state.
    [[nodiscard]] auto isAlternateScreenActive() const noexcept -> bool;
    /// Activate or deactivate the alternate screen.
    /// If activated or deactivated, the buffer is immediately flushed to the terminal.
    void setAlternateScreen(bool enabled) noexcept;
    /// Render a buffer onto the terminal.
    /// The buffer is clipped to the drawable area reported by `size()` and optionally annotated with crop marks.
    /// If the terminal is smaller than the configured minimum size, only the minimum-size marker is rendered.
    /// When `switchToAlternateBuffer` is `true` and the alternate screen is not active, this call first
    /// switches to the alternate screen and then renders the buffer.
    /// @param buffer The buffer to render.
    /// @param settings Additional rendering settings for crop marks and minimum terminal size handling.
    void updateScreen(const ReadableBuffer &buffer, const UpdateSettings &settings = {}) noexcept;
    /// Flush the all buffer immediately to the terminal.
    void flush() noexcept;

public: // backward compatibility.
    /// Write a terminal line break.
    /// @deprecated Use `writeLineBreak()` instead.
    [[deprecated("use writeLineBreak()")]]
    void lineBreak() noexcept {
        writeLineBreak();
    }
    /// Test if non-text output mode is active.
    /// @deprecated Use `outputMode()` instead.
    /// @return `true` if the terminal is not in `OutputMode::BlockText`.
    [[deprecated("use outputMode()")]] [[nodiscard]] auto colorEnabled() const noexcept -> bool {
        return _outputMode != OutputMode::BlockText;
    }
    /// Enable or disable text-only output mode through the legacy boolean API.
    /// @deprecated Use `setOutputMode()` instead.
    /// @param enabled `true` to allow color/control output, `false` for plain text mode.
    [[deprecated("use setOutputMode()")]]
    void setColorEnabled(bool enabled) noexcept;

protected: // implements CursorWriter
    auto createPrintContext() noexcept -> BlockPrintContextPtr override;
    auto printParagraphImpl(const BlockString &paragraph, const ParagraphOptions &options) noexcept -> int override;

private:
    /// Test whether the current backend combination supports full ANSI buffering.
    [[nodiscard]] auto canUseLineBuffer() const noexcept -> bool;
    /// Normalize requested attributes to the supported fully specified state.
    [[nodiscard]] auto normalizedSupportedAttributes(BlockAttributes attributes) const noexcept -> BlockAttributes;
    /// Emit ANSI SGR codes for character attribute changes.
    /// @param previousAttributes The previous supported ANSI attribute state.
    /// @param newAttributes The new supported ANSI attribute state.
    void emitCharAttributeCodes(BlockAttributes previousAttributes, BlockAttributes newAttributes) noexcept;
    /// Either clears the screen or moves the cursor to the home position.
    /// Depends on the refresh mode set.
    void refreshScreen() noexcept;
    /// Update the screen using a back buffer.
    void updateScreenWithBackBuffer(const ReadableBuffer &buffer, const UpdateSettings &settings);
    /// Update and create a new back buffer.
    void updateScreenAndCreateNewBackBuffer(const ReadableBuffer &buffer, const UpdateSettings &settings);
    /// Update and resize the back buffer.
    void updateScreenAndResizeBackBuffer(const ReadableBuffer &buffer, const UpdateSettings &settings);
    /// Partially update the screen using the difference to the back buffer.
    void updateScreenPartialWithBackBuffer(const ReadableBuffer &view);
    /// Update the screen without a back buffer.
    void updateScreenWithoutBackBuffer(const ReadableBuffer &buffer, const UpdateSettings &settings);
    /// Update the minimum size buffer.
    void updateSizeTooSmallBuffer(const UpdateSettings &settings) noexcept;
    /// The implementation of the write function.
    /// @param buffer The buffer to write to the terminal.
    /// @param withRowMove Whether to move the cursor to the next row after writing (`true`)
    ///                    or use newline for the next row.
    void writeImpl(const ReadableBuffer &buffer, bool withRowMove) noexcept;
    /// Print a paragraph using plain terminal output as fallback.
    /// @param paragraph The paragraph to write.
    /// @param options The paragraph options for error handling and spacing.
    /// @return The number of terminal lines written.
    [[nodiscard]] auto printParagraphPlainOutput(const BlockString &paragraph, const ParagraphOptions &options) noexcept
        -> int;
    /// Finish a paragraph by writing one or two explicit line breaks.
    /// @param renderedLines The number of already rendered paragraph lines.
    /// @param paragraphSpacing The spacing to append after the paragraph.
    /// @return The total number of paragraph lines written.
    [[nodiscard]] auto finishParagraphWithExplicitLineBreaks(
        int renderedLines, ParagraphSpacing paragraphSpacing) noexcept -> int;
    /// Get the safety margins applied to the terminal size.
    [[nodiscard]] auto applySafeMargin(bgeo::BlockSize terminalSize) const noexcept -> bgeo::BlockSize;

private:
    TerminalFlags _flags;                             ///< Flags for the terminal behaviour.
    BackendPtr _backend;                              ///< The backend that is used by the terminal.
    OutputMode _outputMode{OutputMode::FullControl};  ///< The current output mode.
    bool _sizeDetectionEnabled{true};                 ///< If size detection is enabled.
    bool _safeMarginEnabled{true};                    ///< If the compatibility safe margin is enabled.
    bool _afterResize{false};                         ///< If the screen should be cleared after a resize.
    RefreshMode _refreshMode{RefreshMode::Overwrite}; ///< The refresh mode to use.
    bgeo::BlockSize _size;                            ///< The configured size of the terminal that is safe to use.
    bgeo::BlockSize _terminalSize;          ///< bgeo::BlockSize of the terminal. Zero if we have no detected size.
    BlockStyle _style{BlockStyle::reset()}; ///< The current 'cursor' style.
    bool _backBufferEnabled{false};         ///< If the back buffer feature is enabled.
    bool _isAlternateScreenActive{false};   ///< If the alternate screen is active or not.
    WritableBufferPtr _sizeTooSmallBuffer;  ///< Buffer for size too small message.
    WritableBufferPtr _backBuffer;          ///< The back buffer.
    impl::InputBackend _input;              ///< The input backend.
    impl::LineBuffer _lineBuffer;           ///< The line buffer.
    std::shared_ptr<std::recursive_mutex> _outputMutex{
        std::make_shared<std::recursive_mutex>()}; ///< Output guard mutex.
};

}
