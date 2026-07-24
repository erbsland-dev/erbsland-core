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
    static constexpr auto cMaximumLength = unit::CpLength{1024U};

public:
    virtual ~ReadSecret() = default;
    ReadSecret(const ReadSecret &) = delete;
    ReadSecret(ReadSecret &&) = delete;
    auto operator=(const ReadSecret &) -> ReadSecret & = delete;
    auto operator=(ReadSecret &&) -> ReadSecret & = delete;

public:
    /// Create a protected single-line terminal editor.
    /// The requested maximum length is capped at 1024 code points. History and initial text must be empty.
    /// @throws err::ParameterError If the terminal is null or unsupported options contain history or initial text.
    [[nodiscard]] static auto create(TerminalPtr terminal, ReadLineOptions options = {}) -> ReadSecretPtr;

public:
    virtual void start() = 0;
    [[nodiscard]] virtual auto update() -> ReadLineResult = 0;
    [[nodiscard]] virtual auto waitForInput() -> ReadLineResult = 0;
    virtual void stop() noexcept = 0;
    [[nodiscard]] virtual auto isActive() const noexcept -> bool = 0;

protected:
    ReadSecret() = default;
};

}
