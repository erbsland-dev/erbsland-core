// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/Application.hpp>
#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/unit/ExitCode.hpp>

namespace demo {

/// The resolved paths for the separate pepper and password-hash database files.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
struct StoragePaths {
    el::Path pepper;   ///< The separate password-hash key file.
    el::Path database; ///< The user database containing only canonical hash records.
};

/// A local educational password-storage and verification application.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
class PasswordHandlerApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;

private: // commands
    [[nodiscard]] auto listUsers(const el::OptionValuesPtr &values) -> el::ExitCode;
    [[nodiscard]] auto addUser(const el::OptionValuesPtr &values) -> el::ExitCode;
    [[nodiscard]] auto removeUser(const el::OptionValuesPtr &values) -> el::ExitCode;
    [[nodiscard]] auto setPassword(const el::OptionValuesPtr &values) -> el::ExitCode;
    [[nodiscard]] auto login(const el::OptionValuesPtr &values) -> el::ExitCode;

private: // setup
    [[nodiscard]] auto storagePaths(const el::OptionValuesPtr &values) const -> StoragePaths;
    void validateStorageState(const StoragePaths &paths) const;
    void initializeStorage(const StoragePaths &paths);
    void requireInteractiveTerminal() const;
    static void requireUsername(const el::String &username);
    [[nodiscard]] static auto displayUsername(const el::String &username) -> el::String;
};

}
