// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Backend_fwd.hpp"
#include "BlockAttributes.hpp"
#include "Color.hpp"
#include "Input.hpp"
#include "MoveMode.hpp"
#include "TerminalFlags.hpp"

#include "../bgeo/BlockSize.hpp"
#include "../text/String.hpp"
#include "../text/StringEditor.hpp"

#include <memory>
#include <optional>

namespace erbsland::cterm {

/// The interface to the underlying platform.
/// This library expects that the platform implementation correctly handles UTF-8 encoding and
/// VT100 ANSI control codes.
class Backend : public std::enable_shared_from_this<Backend> {
public:
    // defaults
    virtual ~Backend() = default;

public:
    /// Initialize the platform.
    /// This method is called from `Terminal::initializeScreen()`.
    virtual void initializePlatform() = 0;

    /// Restore the platform.
    /// This method is called from `Termina::restoreScreen()`.
    virtual void restorePlatform() = 0;

    /// Test if the platform supports color output.
    /// This just tests for color support, not cursor movement.
    /// If you return `false`, no ANSI color codes are emitted and the line buffer is disabled.
    /// Instead, the `emitColor()` method is called on the backend.
    /// @note Before each call to `emitColor()`, the text from the line buffer is emitted.
    [[nodiscard]] virtual auto supportsColorCodes() const noexcept -> bool = 0;

    /// Test if the platform supports cursor movement.
    /// This includes cursor position, clearing the terminal and other cursor-related operations.
    /// If you return `false`, no cursor movement commands are emitted and the line buffer is disabled.
    /// Instead, the `moveCursor()` etc. methods are called on the backend.
    /// @note Before each call of the cursor methods, the text from the line buffer is emitted.
    [[nodiscard]] virtual auto supportsCursorCodes() const noexcept -> bool = 0;

    /// Test if the backend supports cursor visible/invisible ANSI sequences.
    /// If this method returns `true`, the ANSI codes `ESC[?25l` and `ESC[?25h` to control the cursor visibility.
    /// If it returns `false`, `Terminal` calls `setCursorVisible()` instead.
    [[nodiscard]] virtual auto supportsCursorVisibilityCodes() const noexcept -> bool { return true; }

    /// Test if the backend supports alternate screen buffer codes.
    /// If this method returns `true`, the ANSI codes `ESC[?1049h` and `ESC[?1049l` are sent to the backend to
    /// switch to the alternate screen buffer. *Additionally* `setAlternateScreenBuffer()` is called to notify
    /// the backend about the change.
    /// If this method returns `false`, `Terminal` *only* calls `setAlternateScreenBuffer()` instead.
    [[nodiscard]] virtual auto supportsAlternateScreenBufferCodes() const noexcept -> bool { return true; }

    /// Check if an interactive terminal is attached to the process.
    /// This state is typically established during `Terminal::initializeScreen()`.
    /// @return `true` if interactive terminal features such as resize and cursor control are available.
    [[nodiscard]] virtual auto isInteractive() const noexcept -> bool = 0;

    /// Detect the current terminal screen size.
    ///
    /// This method is invoked by `Terminal::initializeScreen()` and `Terminal::testScreenSize()`
    /// when size detection is enabled. Interactive applications may call `testScreenSize()`
    /// before every screen update, so implementations should be efficient or perform updates
    /// asynchronously in the background.
    ///
    /// The returned size should represent the visible terminal area available for output.
    /// A safety margin of one column and one row is applied by `Terminal` automatically.
    ///
    /// @return The detected screen size, or `std::nullopt` if detection failed.
    [[nodiscard]] virtual auto detectScreenSize() -> std::optional<bgeo::BlockSize> = 0;

    /// Change the current color.
    /// Only called if `supportsColorCodes()` returns `false`.
    /// @param color The new color to set.
    virtual void emitColor([[maybe_unused]] Color color) {}
    /// Get the character attributes supported by this backend.
    /// The returned value is a fully specified bit mask.
    /// @return The supported character attributes.
    [[nodiscard]] virtual auto supportedBlockAttributes() const noexcept -> BlockAttributes {
        return BlockAttributes::all();
    }
    /// Get the character attributes that can be emitted directly via ANSI codes.
    /// The returned value is a fully specified bit mask.
    /// @return The character attributes supported through ANSI codes.
    [[nodiscard]] virtual auto supportedBlockAttributeCodes() const noexcept -> BlockAttributes {
        return supportedBlockAttributes();
    }
    /// Change the current character attributes.
    /// Only called if one or more supported attributes cannot be emitted through ANSI codes.
    /// The backend receives the current supported attribute state and can diff it against its own local state.
    /// @param attributes The current supported attribute state.
    virtual void emitBlockAttributes([[maybe_unused]] BlockAttributes attributes) {}

    /// Move the cursor.
    /// Only called if `supportsCursorCodes()` returns `false`.
    /// @param posOrDelta The absolute or relative movement for the cursor. (0,0) = top-left corner.
    /// @param mode The move mode, either absolute or relative.
    virtual void moveCursor([[maybe_unused]] bgeo::BlockPosition posOrDelta, [[maybe_unused]] MoveMode mode) {}

    /// Clear the screen and move the cursor to (0,0).
    /// Only called if `supportsCursorCodes()` returns `false`.
    virtual void clearScreen() {}

    /// Control if the cursor is visible on the terminal.
    /// If `supportsCursorVisibilityCodes()` returns `false`, this method is called to control the cursor visibility.
    virtual void setCursorVisible([[maybe_unused]] bool visible) {}

    /// Enables/disables the alternate screen buffer or notifies the backend about the change.
    virtual void setAlternateScreenBuffer([[maybe_unused]] bool enabled) {}

    /// Emit the given UTF-8 encoded text.
    /// If the backend does not support UTF-8, you are responsible to convert the text to the expected encoding.
    /// Line breaks are usually just NL and not CRLF.
    /// UTF-8 sequences, and ANSI sequences are always complete in one call of this method.
    /// @param text The UTF-8 encoded text to emit.
    virtual void emitText(const text::String &text) = 0;

    /// Flush the output buffer.
    /// After a call of this method, the backend must flush all previously emitted text and control sequences
    /// to the terminal.
    virtual void emitFlush() = 0;

public: // input handling.
    /// Get the current input mode.
    [[nodiscard]] virtual auto inputMode() const noexcept -> Input::Mode = 0;
    /// Set the current input mode.
    /// @param mode The new input mode.
    virtual void setInputMode(Input::Mode mode) = 0;
    /// Read one key event without blocking longer than the given timeout.
    /// In `Input::Mode::Key`, negative timeouts must be normalized to zero and any timeout less than or equal to
    /// zero must perform a non-blocking poll.
    /// In `Input::Mode::ReadLine`, the timeout is ignored and this call may block until a line was entered.
    /// @param timeout Maximum wait time in `Mode::Key`.
    /// @return The parsed key event, or an invalid key if no supported input was read before the timeout expired.
    [[nodiscard]] virtual auto readKey(std::chrono::milliseconds timeout) -> Key = 0;
    /// Wait until one key event is available.
    /// In `Input::Mode::ReadLine`, this call blocks until a line was entered and returns the converted key.
    /// @return The parsed key event.
    [[nodiscard]] virtual auto waitForKey() -> Key = 0;
    /// Read text input in the terminal.
    /// Can be ignored in `Input::Mode::Key` mode.
    /// @return The read text, without line breaks.
    [[nodiscard]] virtual auto readLine() -> text::String = 0;
    /// Securely discard pending key input and partial native decoder state.
    virtual void purgePendingInput() noexcept {}

public:
    /// Create the default backend for this platform.
    [[nodiscard]] static auto createPlatformDefault(TerminalFlags terminalFlags) -> BackendPtr;
};

}
