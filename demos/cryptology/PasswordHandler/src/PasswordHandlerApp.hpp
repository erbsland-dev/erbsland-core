// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StoragePaths.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/unit/ExitCode.hpp>

namespace demo {

/// A local educational password-storage and verification application.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
class PasswordHandlerApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    /// Initialize the application identity and command handling.
    void initialize() override;
    /// Register the password-store commands and their arguments.
    void registerCommandLineOptions(const el::OptionsPtr &options) override;

private: // commands
    /// List all stored usernames.
    [[nodiscard]] auto listUsers(const el::OptionValuesPtr &values) -> el::ExitCode;
    /// Create a password-hash record for a new user.
    [[nodiscard]] auto addUser(const el::OptionValuesPtr &values) -> el::ExitCode;
    /// Remove one user's password-hash record.
    [[nodiscard]] auto removeUser(const el::OptionValuesPtr &values) -> el::ExitCode;
    /// Replace an existing user's password-hash record.
    [[nodiscard]] auto setPassword(const el::OptionValuesPtr &values) -> el::ExitCode;
    /// Verify a password and rotate its hash when needed.
    [[nodiscard]] auto login(const el::OptionValuesPtr &values) -> el::ExitCode;

private: // setup
    /// Resolve command-line storage locations.
    [[nodiscard]] auto storagePaths(const el::OptionValuesPtr &values) const -> StoragePaths;
    /// Validate the current pepper and database files before a change.
    void validateStorageState(const StoragePaths &paths) const;
    /// Create missing password-storage files.
    void initializeStorage(const StoragePaths &paths);
    /// Require an interactive terminal before prompting for a password.
    void requireInteractiveTerminal() const;
    /// Reject an empty or otherwise unsupported username.
    static void requireUsername(const el::String &username);
    /// Escape a username for unambiguous terminal output.
    [[nodiscard]] static auto displayUsername(const el::String &username) -> el::String;
};

}
