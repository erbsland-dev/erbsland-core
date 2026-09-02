// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PosixSignalDispatcher_fwd.hpp"

#include "../Backend.hpp"

#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace erbsland::cterm::impl {

/// POSIX terminal backend implementation.
class PosixBackend final : public Backend {
    using clock = std::chrono::steady_clock;
    constexpr static auto cMinimumDelayBetweenScreenSizeDetection = std::chrono::milliseconds{100};
    constexpr static auto cEscapeSequenceTimeout = std::chrono::milliseconds{25};
    constexpr static auto cMaximumPendingKeyInputSize = 1024;
    constexpr static auto cMaximumInputReadSize = 64;

public:
    /// Result of attempting to detect the terminal size.
    enum class SizeDetectionResult : uint8_t { NoTerminalAttached, NoTerminalSize, Success };
    using OptionalTimeout = std::optional<std::chrono::milliseconds>;

public:
    /// Create a POSIX terminal backend with requested flags.
    /// @param terminalFlags The requested terminal features.
    explicit PosixBackend(TerminalFlags terminalFlags); // must never be called directly
    ~PosixBackend() override;

public:
    void initializePlatform() override;
    void restorePlatform() override;
    [[nodiscard]] auto supportsColorCodes() const noexcept -> bool override;
    [[nodiscard]] auto supportsCursorCodes() const noexcept -> bool override;
    [[nodiscard]] auto isInteractive() const noexcept -> bool override;
    [[nodiscard]] auto detectScreenSize() -> std::optional<block::Size> override;
    void emitText(const text::String &text) override;
    void emitFlush() override;
    void setAlternateScreenBuffer(bool enabled) override;

    // input
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
    static auto instance() noexcept -> PosixBackend *;
    /// Restore the platform using the global instance.
    static void restoreGlobalPlatform() noexcept;
    /// Test whether the given POSIX output descriptor is attached to an interactive terminal.
    /// @param outputFd The descriptor used for terminal output.
    /// @return `true` if the descriptor refers to a terminal.
    [[nodiscard]] static auto isInteractiveOutput(int outputFd) noexcept -> bool;

private:
    /// Detect the screen size.
    [[nodiscard]] auto getScreenSize() -> std::pair<SizeDetectionResult, block::Size>;
    /// Try to detect the size for a file descriptor.
    [[nodiscard]] static auto getScreenSizeForFd(int fd) -> std::pair<SizeDetectionResult, block::Size>;
    /// Initialize a key interactive session.
    void initializeKeyInputSession();
    /// Restore a key interactive session.
    void restoreKeyInputSession();
    /// Wait until stdin becomes readable.
    [[nodiscard]] static auto waitForInput(OptionalTimeout timeout) -> bool;
    /// Poll whether stdin is currently readable without blocking.
    [[nodiscard]] static auto pollForInput() -> bool;
    /// Read one or more available chunks and append them to the pending key buffer.
    void appendInputChunks(OptionalTimeout timeout);
    /// Mark the start time for one pending escape sequence.
    void markPendingEscapeSequence() noexcept;
    /// Test if the pending escape sequence waited long enough for follow-up bytes.
    [[nodiscard]] auto escapeSequenceExpired() const noexcept -> bool;
    /// Calculate the timeout used to collect follow-up escape-sequence bytes.
    [[nodiscard]] auto escapeSequenceWaitTimeout(OptionalTimeout timeout) const noexcept -> std::chrono::milliseconds;
    /// Remove decoded bytes and clear escape sequence state when appropriate.
    void erasePendingKeyInput(std::size_t byteCount);
    /// Decode one pending key using either a timeout-based poll or a blocking wait.
    [[nodiscard]] auto readDecodedKey(OptionalTimeout timeout) -> Key;
    /// Restore the terminal and terminate the process for one handled signal.
    void handleProcessSignal(int signalNumber) noexcept;

private:
    static std::mutex _instanceMutex;                       ///< The mutex to protect the instance.
    static PosixBackend *_instance;                         ///< The global instance of the PosixBackend.

    TerminalFlags _terminalFlags;                           ///< The terminal flags.
    bool _isInitialized = false;                            ///< If the platform was initialized.
    bool _keyInputSessionActive = false;                    ///< If we have an input session that needs to be restored.
    clock::time_point _lastScreenSizeDetection = {};        ///< Time of last screen size detection.
    std::optional<block::Size> _lastScreenSize;             ///< The last cached screen size.
    bool _firstScreenSizeDetection = true;                  ///< A flag to mark the first detection, for extra effort.
    bool _hasNoTerminalAttached = false;                    ///< If standard output is not an interactive terminal.
    Input::Mode _inputMode{Input::Mode::ReadLine};          ///< The current input mode.
    bool _isAlternateScreenActive = false;                  ///< remember if we are in alternate screen mode.
    termios _originalState{};                               ///< state backup.
    std::string _pendingKeyInput;                           ///< Buffered raw input that was not yet decoded.
    std::optional<clock::time_point> _pendingEscapeStarted; ///< Start time for resolving a pending escape sequence.
    std::unique_ptr<PosixSignalDispatcher> _signalHandler;  ///< Helper that forwards termination signals safely.
};

}
