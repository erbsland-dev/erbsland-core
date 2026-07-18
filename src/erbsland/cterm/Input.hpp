// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Key.hpp"

#include "../text/StringEditor.hpp"

#include <chrono>
#include <cstdint>
#include <memory>

namespace erbsland::cterm {

class Input;
using InputPtr = std::shared_ptr<Input>;

/// The input interface.
class Input {
public:
    /// Supported reading modes for the input backend.
    enum class Mode : uint8_t {
        /// Read one full line from standard input.
        ReadLine,
        /// Read raw key presses from the terminal backend.
        Key,
    };

public:
    /// Destroy the input object.
    virtual ~Input() = default;

public:
    /// Get the current reading mode.
    [[nodiscard]] virtual auto mode() const noexcept -> Mode = 0;
    /// Set the current reading mode.
    /// @param mode The new input mode.
    virtual void setMode(Mode mode) = 0;
    /// Read one key event without blocking longer than the given timeout.
    /// In `Mode::Key`, any timeout less than or equal to zero is normalized to zero and performs a non-blocking poll.
    /// In `Mode::ReadLine`, the timeout is ignored and this call behaves like a blocking line read converted into
    /// `Key`.
    /// @param timeout Maximum wait time in `Mode::Key`.
    /// @return The parsed key event, or an invalid key if no supported input was read before the timeout expired.
    [[nodiscard]] auto readKey(std::chrono::milliseconds timeout = {}) const -> Key {
        if (timeout < std::chrono::milliseconds::zero()) {
            timeout = std::chrono::milliseconds::zero();
        }
        return readKeyImpl(timeout);
    }
    /// Wait until one key event is available.
    /// In `Mode::ReadLine`, this call blocks until one line was entered and returns the converted key.
    /// @return The parsed key event.
    [[nodiscard]] auto waitForKey() const -> Key { return waitForKeyImpl(); }
    /// Deprecated wrapper for `readKey()`.
    /// In `Mode::Key`, any timeout less than or equal to zero performs a blocking call.
    /// @param timeout Maximum wait time in `Mode::Key`.
    /// @return The parsed key event, or an invalid key if no supported input was read before the timeout expired.
    [[deprecated("Use readKey(timeout) for polling or waitForKey() for blocking input.")]] [[nodiscard]] auto read(
        const std::chrono::milliseconds timeout = {}) const -> Key {
        return timeout <= std::chrono::milliseconds::zero() ? waitForKey() : readKey(timeout);
    }
    /// Read a line of text from the terminal.
    [[nodiscard]] virtual auto readLine() -> text::String = 0;

protected:
    /// Implement `readKey()`.
    /// The public `readKey()` wrapper normalizes negative timeouts to zero before calling this method.
    /// @param timeout Maximum wait time in `Mode::Key`; a value of zero performs a non-blocking poll.
    /// Ignored in `Mode::ReadLine`.
    /// @return The parsed key event, or an invalid key if no supported input was read.
    [[nodiscard]] virtual auto readKeyImpl(std::chrono::milliseconds timeout) const -> Key = 0;
    /// Implement `waitForKey()`.
    /// @return The parsed key event.
    [[nodiscard]] virtual auto waitForKeyImpl() const -> Key = 0;
};

}
