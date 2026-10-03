// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReadLineOptions.hpp"
#include "ReadLineResult.hpp"
#include "ReadSecret_fwd.hpp"
#include "Terminal_fwd.hpp"

namespace erbsland::cterm {

/// An interactive, securely erased single-line secret editor.
/// Entered code points are retained in fixed protected storage and only a fixed bullet mask is rendered.
/// @seedoc{/topics/cterm/input}
/// @tested{ReadSecretTest}
class ReadSecret {
public:
    static constexpr auto cMaximumLength = unit::CpCount{1024U};

public:
    // defaults/deletions
    virtual ~ReadSecret() = default;
    ReadSecret(const ReadSecret &) = delete;
    ReadSecret(ReadSecret &&) = delete;
    auto operator=(const ReadSecret &) -> ReadSecret & = delete;
    auto operator=(ReadSecret &&) -> ReadSecret & = delete;

public:
    /// Create a protected single-line terminal editor.
    /// The requested maximum length is capped at 1024 code points. History and initial text must be empty.
    /// @param terminal The terminal retained by the editor. Must not be null.
    /// @param options The editing options. History and initial text must be empty.
    [[nodiscard]] static auto create(TerminalPtr terminal, ReadLineOptions options = {}) -> ReadSecretPtr;

public:
    /// Start interactive secret input; the operation must be inactive.
    /// @throws TerminalError If interactive full-control output is unavailable.
    virtual void start() = 0;
    /// Process available terminal input and return its result; the operation must be active.
    [[nodiscard]] virtual auto update() -> ReadLineResult = 0;
    /// Wait for input, process it, and return its result; the operation must be inactive.
    /// @throws TerminalError If interactive full-control output is unavailable.
    [[nodiscard]] virtual auto waitForInput() -> ReadLineResult = 0;
    /// Stop interactive secret input.
    virtual void stop() noexcept = 0;
    /// Test if secret input is active.
    [[nodiscard]] virtual auto isActive() const noexcept -> bool = 0;

protected:
    /// Create an inactive secret-input operation.
    ReadSecret() = default;
};

}
