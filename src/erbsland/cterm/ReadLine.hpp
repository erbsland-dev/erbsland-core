// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReadLine_fwd.hpp"
#include "ReadLineOptions.hpp"
#include "ReadLineResult.hpp"
#include "Terminal_fwd.hpp"

namespace erbsland::cterm {

/// An interactive terminal line editor.
/// A read-line operation exclusively owns terminal output between `start()` and `stop()`.
/// @seedoc{/topics/cterm/input}
/// @tested{ReadLineTest}
class ReadLine {
public:
    /// Destroy the line editor, safely stopping an active operation.
    virtual ~ReadLine() = default;

    // deletions
    ReadLine(const ReadLine &) = delete;
    ReadLine(ReadLine &&) = delete;
    auto operator=(const ReadLine &) -> ReadLine & = delete;
    auto operator=(ReadLine &&) -> ReadLine & = delete;

public: // factory methods
    /// Create an interactive terminal line editor.
    /// Text and history are copied and normalized to the configured limits.
    /// @param terminal The terminal retained for the lifetime of this object.
    /// @param options The appearance and editing options.
    /// @return The new line editor.
    /// @throws err::ParameterError if `terminal` is null or `options` is structurally invalid.
    [[nodiscard]] static auto create(TerminalPtr terminal, ReadLineOptions options = {}) -> ReadLinePtr;

public: // operation
    /// Start a non-blocking read-line operation.
    /// @throws err::RuntimeError if already active or interactive full-control output is unavailable.
    virtual void start() = 0;
    /// Poll and update an active read-line operation.
    /// Terminal results remain latched until `stop()` is called.
    /// @return `Idle`, `Committed`, `Cancelled`, or `Timeout`.
    /// @throws err::RuntimeError if no operation is active.
    [[nodiscard]] virtual auto update() -> ReadLineResult = 0;
    /// Run a complete blocking read-line operation.
    /// This method never returns `Idle` and always stops the operation before returning or propagating an exception.
    /// @return `Committed`, `Cancelled`, or `Timeout`.
    /// @throws err::RuntimeError if already active or interactive full-control output is unavailable.
    [[nodiscard]] virtual auto waitForInput() -> ReadLineResult = 0;
    /// Stop the operation and restore or retain the rendered input area according to the cleanup option.
    /// Calling this method while inactive has no effect.
    virtual void stop() noexcept = 0;

public: // tests
    /// Test if an operation is active.
    [[nodiscard]] virtual auto isActive() const noexcept -> bool = 0;

protected:
    ReadLine() = default;
};

}
