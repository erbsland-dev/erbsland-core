// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/text/String.hpp>

namespace demo {

/// Reads masked password text from a fresh interactive terminal editor.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
class PasswordPrompt final {
public:
    /// Read one existing password without applying a creation policy.
    [[nodiscard]] static auto readPassword(const el::cterm::TerminalPtr &terminal) -> el::String;
    /// Read and confirm a new password that satisfies the demo policy.
    [[nodiscard]] static auto readNewPassword(const el::cterm::TerminalPtr &terminal) -> el::String;

private:
    [[nodiscard]] static auto read(const el::cterm::TerminalPtr &terminal, const el::String &title) -> el::String;
};

}
