// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/text/String.hpp>

namespace demo {

/// Reads masked password text from a fresh interactive terminal editor.
/// Each prompt is bound to one terminal and creates a temporary editor for each password entry.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
class PasswordPrompt final {
public:
    /// Create a password prompt that reads from the given terminal.
    explicit PasswordPrompt(el::cterm::TerminalPtr terminal);

public:
    /// Read one existing password without applying a creation policy.
    [[nodiscard]] auto readPassword() const -> el::String;
    /// Read and confirm a new password that satisfies the demo policy.
    /// New passwords require at least 15 Unicode code points and are otherwise preserved exactly as entered.
    [[nodiscard]] auto readNewPassword() const -> el::String;

private:
    /// Read one masked password with the given editor title.
    [[nodiscard]] auto read(const el::String &title) const -> el::String;

private:
    el::cterm::TerminalPtr _terminal; ///< The terminal used for all password editors.
};

}
