// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WindowsBackendPrivate_fwd.hpp"
#include "WindowsSignalDispatcher_fwd.hpp"

#include "../Backend.hpp"

#include <mutex>

namespace erbsland::cterm::impl {

/// Windows console implementation of the terminal backend.
/// @notest{Platform integration is covered through terminal integration tests.}
class WindowsBackend : public Backend {
public:
    using OptionalTimeout = std::optional<std::chrono::milliseconds>;

    /// Create a Windows terminal backend with requested flags.
    /// @param terminalFlags The requested terminal features.
    explicit WindowsBackend(TerminalFlags terminalFlags);

    // defaults
    ~WindowsBackend() override;

public: // implement backend
    void initializePlatform() override;
    void restorePlatform() override;
    [[nodiscard]] auto supportsColorCodes() const noexcept -> bool override;
    [[nodiscard]] auto supportsCursorCodes() const noexcept -> bool override;
    [[nodiscard]] auto supportsCursorVisibilityCodes() const noexcept -> bool override;
    [[nodiscard]] auto supportsAlternateScreenBufferCodes() const noexcept -> bool override;
    [[nodiscard]] auto isInteractive() const noexcept -> bool override;
    [[nodiscard]] auto detectScreenSize() -> std::optional<block::Size> override;
    void setCursorVisible(bool visible) override;
    void emitText(const text::String &text) override;
    void emitFlush() override;
    void setAlternateScreenBuffer(bool enabled) override;

public: // input
    [[nodiscard]] auto inputMode() const noexcept -> Input::Mode override;
    void setInputMode(Input::Mode mode) override;
    [[nodiscard]] auto readKey(std::chrono::milliseconds timeout) -> Key override;
    [[nodiscard]] auto waitForKey() -> Key override;
    [[nodiscard]] auto readLine() -> text::String override;
    void purgePendingInput() noexcept override;

public:
    /// Create or access the global instance.
    static auto getOrCreate(TerminalFlags terminalFlags) noexcept -> BackendPtr;
    /// Access the global instance.
    static auto instance() noexcept -> WindowsBackend *;
    /// Restore the platform using the global instance.
    static void restoreGlobalPlatform() noexcept;

private:
    /// Enabled UTF-8 in the Windows terminal.
    void enableUtf8Mode();
    /// Enables ANSI mode in the Windows terminal.
    void enableAnsiMode();
    /// Change the cursor visibility.
    /// @param visible true to show the cursor, false to hide it
    /// @return The previous cursor visibility state
    auto changeCursorVisibility(bool visible) -> bool;
    /// Queue a decoded key one or more times.
    void enqueueKey(const Key &key, std::size_t repeatCount = 1);
    /// Flush one buffered Unicode text input into the key queue.
    void flushPendingTextInput();
    /// Append a translated Unicode code point to the buffered text input.
    void appendTextCodePoint(char32_t codePoint, std::size_t repeatCount);
    /// Decode one UTF-16 code unit from the console into a Unicode code point.
    [[nodiscard]] auto decodeUtf16CodeUnit(char16_t codeUnit) -> std::optional<char32_t>;
    /// Translate Windows control-key flags into key modifiers.
    [[nodiscard]] static auto keyModifiersFromControlState(uint32_t controlKeyState) noexcept -> KeyModifiers;
    /// Read one key using either a timeout-based poll or a blocking wait.
    [[nodiscard]] auto readKeyFromConsole(OptionalTimeout timeout) -> Key;
    /// Restore the terminal and terminate the process for one handled event.
    void handleProcessSignal(int exitCode) noexcept;

private:
    static std::mutex _instanceMutex;          ///< The mutex to protect the instance.
    static WindowsBackend *_instance;          ///< The global instance of the PosixBackend.
    std::unique_ptr<WindowsBackendPrivate> _p; ///< The private impl of WindowsBackend.
};

}
