// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PasswordHandlerApp.hpp"
#include "PasswordPrompt.hpp"
#include "PepperStore.hpp"
#include "UserDatabase.hpp"

#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/path/PathCollisionMode.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/stream/StandardStreams.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/ElementCount.hpp>

namespace demo {

using namespace el::text::literals;

auto PasswordHandlerApp::listUsers(const el::OptionValuesPtr &values) -> el::ExitCode {
    const auto paths = storagePaths(values);
    initializeStorage(paths);
    const auto database = UserDatabase::load(paths.database);
    if (database.users().count() == el::ElementCount{0U}) {
        el::io::printLine("No users are stored."_el);
        return el::ExitCode::success();
    }
    for (const auto &[username, passwordHash] : database.users()) {
        static_cast<void>(passwordHash);
        el::io::printLine(displayUsername(username));
    }
    return el::ExitCode::success();
}

auto PasswordHandlerApp::removeUser(const el::OptionValuesPtr &values) -> el::ExitCode {
    const auto username = values->getText("username"_el);
    requireUsername(username);
    const auto paths = storagePaths(values);
    initializeStorage(paths);
    auto database = UserDatabase::load(paths.database);
    if (!database.tryRemove(username)) {
        throw el::ApplicationError{"The exact username does not exist."_el};
    }
    database.save(paths.database, el::PathCollisionMode::Overwrite);
    el::io::printLine("User removed: "_el, displayUsername(username));
    return el::ExitCode::success();
}

auto PasswordHandlerApp::setPassword(const el::OptionValuesPtr &values) -> el::ExitCode {
    requireInteractiveTerminal();
    const auto username = values->getText("username"_el);
    requireUsername(username);
    const auto paths = storagePaths(values);
    validateStorageState(paths);
    auto database = paths.database.info().exists() ? UserDatabase::load(paths.database) : UserDatabase{};
    if (!database.contains(username)) {
        throw el::ApplicationError{"The exact username does not exist."_el};
    }

    const auto password = PasswordPrompt::readNewPassword(terminal());
    initializeStorage(paths);
    auto hasher = PepperStore::loadHasher(paths.pepper);
    const auto passwordHash = hasher.hash(password);
    static_cast<void>(database.trySetPassword(username, passwordHash.toString()));
    database.save(paths.database, el::PathCollisionMode::Overwrite);
    el::io::printLine("Password replaced for: "_el, displayUsername(username));
    return el::ExitCode::success();
}

}
