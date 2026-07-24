// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PasswordPrompt.hpp"

#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/core/ApplicationErrorContext.hpp>
#include <erbsland/cterm/ReadLineOptions.hpp>
#include <erbsland/cterm/ReadSecret.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/LineCount.hpp>

namespace demo {

using namespace el::text::literals;

auto PasswordPrompt::readPassword(const el::cterm::TerminalPtr &terminal) -> el::String {
    return read(terminal, "Enter password"_el);
}

/// Read a new password twice and compare the protected values.
///
/// New passwords require at least 15 Unicode code points. Spaces and Unicode are accepted exactly as entered without
/// trimming, case folding, composition rules, or normalization.
auto PasswordPrompt::readNewPassword(const el::cterm::TerminalPtr &terminal) -> el::String {
    auto password = read(terminal, "Enter new password"_el);
    if (password.characterLength() < el::CpLength{15U}) {
        throw el::ApplicationError{el::core::ApplicationErrorContext{
            "Password is too short"_el, "New passwords must contain at least 15 Unicode code points."_el}};
    }
    const auto confirmation = read(terminal, "Confirm new password"_el);
    if (password != confirmation) {
        throw el::ApplicationError{el::core::ApplicationErrorContext{
            "Passwords do not match"_el, "Enter the same password in both prompts."_el}};
    }
    return password;
}

/// Read one masked password through a fresh blocking `ReadSecret`.
auto PasswordPrompt::read(const el::cterm::TerminalPtr &terminal, const el::String &title) -> el::String {
    auto options = el::cterm::ReadLineOptions{}.setTitle(title).setPlaceholder("Password"_el);
    const auto editor = el::cterm::ReadSecret::create(terminal, options);
    auto result = editor->waitForInput();
    if (!result.isCommitted()) {
        throw el::ApplicationError{el::core::ApplicationErrorContext{
            "Password entry cancelled"_el, "No password-storage changes were made."_el}};
    }
    return result.takeData();
}

}
