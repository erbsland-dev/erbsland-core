// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PasswordHandlerApp.hpp"
#include "PepperStore.hpp"
#include "UserDatabase.hpp"

#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/core/ApplicationErrorContext.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/options/OptionModule.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionType.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/path/PathCollisionMode.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathResolveMode.hpp>
#include <erbsland/text/EscapeAmount.hpp>
#include <erbsland/text/EscapeFormat.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/Version.hpp>

namespace demo {

using namespace el::text::literals;

void PasswordHandlerApp::initialize() {
    info().setApplicationName("Password Handler"_el);
    info().setApplicationVersion(el::Version{0, 1, 0});
    info().setAuthorName("Erbsland DEV"_el);
    info().setLicenseText("Apache-2.0"_el);
    enableTerminal();
}

void PasswordHandlerApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpTitle("Password Handler"_el);
    options->setHelpDescription(
        "Demonstrates local password hashing, verification, pepper rotation, and safe persistence."_el);
    options->setHelpEpilog(
        "Passwords are always read from an interactive masked prompt. "
        "Use a production secret manager instead of the demo's local pepper file."_el);
    options->addOption({"-p"_el, "--pepper"_el, "pepper"_el})
        .setType(el::OptionType::Text)
        .setHelpDescription("Path to the separate ELCL pepper file."_el);
    options->addOption({"-d"_el, "--database"_el, "database"_el})
        .setType(el::OptionType::Text)
        .setHelpDescription("Path to the ELCL user database."_el);

    auto listUsersModule = el::OptionModule::create("list-users"_el);
    listUsersModule->setHelpDescription("List every stored username."_el);
    listUsersModule->setMainFn([this](const el::OptionValuesPtr &values) -> el::ExitCode { return listUsers(values); });
    options->addModule(listUsersModule);

    auto addUserModule = el::OptionModule::create("add-user"_el);
    addUserModule->setHelpDescription("Add a user and prompt for a new password."_el);
    addUserModule->addOption("username"_el).setRequired().setHelpDescription("Exact username to add."_el);
    addUserModule->setMainFn([this](const el::OptionValuesPtr &values) -> el::ExitCode { return addUser(values); });
    options->addModule(addUserModule);

    auto removeUserModule = el::OptionModule::create("remove-user"_el);
    removeUserModule->setHelpDescription("Remove a user record."_el);
    removeUserModule->addOption("username"_el).setRequired().setHelpDescription("Exact username to remove."_el);
    removeUserModule->setMainFn(
        [this](const el::OptionValuesPtr &values) -> el::ExitCode { return removeUser(values); });
    options->addModule(removeUserModule);

    auto setPasswordModule = el::OptionModule::create("set-password"_el);
    setPasswordModule->setHelpDescription("Replace an existing user's password."_el);
    setPasswordModule->addOption("username"_el).setRequired().setHelpDescription("Exact username to update."_el);
    setPasswordModule->setMainFn(
        [this](const el::OptionValuesPtr &values) -> el::ExitCode { return setPassword(values); });
    options->addModule(setPasswordModule);

    auto loginModule = el::OptionModule::create("login"_el);
    loginModule->setHelpDescription("Verify one password without revealing account state."_el);
    loginModule->addOption("username"_el).setRequired().setHelpDescription("Exact username to verify."_el);
    loginModule->setMainFn([this](const el::OptionValuesPtr &values) -> el::ExitCode { return login(values); });
    options->addModule(loginModule);
}

auto PasswordHandlerApp::storagePaths(const el::OptionValuesPtr &values) const -> StoragePaths {
    const auto pepperValue = values->value("pepper"_el);
    const auto databaseValue = values->value("database"_el);
    auto home = el::Path{};
    if (pepperValue == nullptr || databaseValue == nullptr) {
        home = el::Path::userHomeDirectoryOrThrow() / ".password-handler"_el;
    }
    auto paths = StoragePaths{
        .pepper = pepperValue == nullptr ? home / "pepper.elcl"_el : el::Path{pepperValue->getText()},
        .database = databaseValue == nullptr ? home / "users.elcl"_el : el::Path{databaseValue->getText()},
    };
    if (paths.pepper.isEmpty() || paths.pepper.isRoot() || paths.database.isEmpty() || paths.database.isRoot()) {
        throw el::ApplicationError{"Pepper and database options require valid file paths."_el};
    }
    paths.pepper = paths.pepper.toAbsoluteOrThrow();
    paths.database = paths.database.toAbsoluteOrThrow();
    if (paths.pepper.resolveOrThrow(el::PathResolveMode::Weak) ==
        paths.database.resolveOrThrow(el::PathResolveMode::Weak)) {
        throw el::ApplicationError{"Pepper and database paths must identify different files."_el};
    }
    return paths;
}

void PasswordHandlerApp::initializeStorage(const StoragePaths &paths) {
    validateStorageState(paths);
    const auto pepperExists = paths.pepper.info().exists();
    const auto databaseExists = paths.database.info().exists();
    if (!pepperExists) {
        auto pepperStore = PepperStore{paths.pepper};
        pepperStore.create();
    }
    if (!databaseExists) {
        UserDatabase{}.save(paths.database, el::PathCollisionMode::Stop);
    }
}

void PasswordHandlerApp::validateStorageState(const StoragePaths &paths) const {
    if (paths.database.info().exists() && !paths.pepper.info().exists()) {
        throw el::ApplicationError{el::core::ApplicationErrorContext{
            "Pepper file is missing"_el,
            "The existing database cannot be verified without its original password-hash key."_el}};
    }
}

void PasswordHandlerApp::requireInteractiveTerminal() const {
    if (!terminal()->isInteractive()) {
        throw el::ApplicationError{el::core::ApplicationErrorContext{
            "Interactive terminal required"_el, "Password commands require a terminal that can mask typed input."_el}};
    }
}

void PasswordHandlerApp::requireUsername(const el::String &username) {
    if (username.isEmpty()) {
        throw el::ApplicationError{"Usernames must not be empty."_el};
    }
}

auto PasswordHandlerApp::displayUsername(const el::String &username) -> el::String {
    return username.toEscaped(el::EscapeFormat::Display, el::EscapeAmount::Required);
}

}
