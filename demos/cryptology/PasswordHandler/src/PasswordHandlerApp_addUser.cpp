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

namespace demo {

using namespace el::text::literals;

/// Hash a new user's password and store only its canonical password-hash record.
///
/// The exact username is checked before prompting. The password remains in protected storage after the prompt's
/// unavoidable ordinary-memory handoff. `PasswordHasher::hash()` creates a fresh salt and applies the active pepper;
/// only the returned canonical record is written to the user database.
auto PasswordHandlerApp::addUser(const el::OptionValuesPtr &values) -> el::ExitCode {
    requireInteractiveTerminal();
    const auto username = values->getText("username"_el);
    requireUsername(username);
    const auto paths = storagePaths(values);
    validateStorageState(paths);
    auto database = paths.database.info().exists() ? UserDatabase::load(paths.database) : UserDatabase{};
    if (database.contains(username)) {
        throw el::ApplicationError{"The exact username already exists."_el};
    }

    const auto password = PasswordPrompt::readNewPassword(terminal());
    initializeStorage(paths);
    auto hasher = PepperStore::loadHasher(paths.pepper);
    const auto passwordHash = hasher.hash(password);
    static_cast<void>(database.tryAdd(username, passwordHash.toString()));
    database.save(paths.database, el::PathCollisionMode::Overwrite);
    el::io::printLine("User added: "_el, displayUsername(username));
    return el::ExitCode::success();
}

}
