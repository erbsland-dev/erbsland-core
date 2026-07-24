// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PasswordHandlerApp.hpp"
#include "PasswordPrompt.hpp"
#include "PepperStore.hpp"
#include "UserDatabase.hpp"

#include <erbsland/cryptology/PasswordHash.hpp>
#include <erbsland/path/PathCollisionMode.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/stream/StandardStreams.hpp>
#include <erbsland/text/Literals.hpp>

namespace demo {

using namespace el::text::literals;

/// Verify a login and persist any replacement hash before reporting success.
///
/// Unknown usernames and malformed records use the same invalid `PasswordHash` sentinel and always reach
/// `PasswordHasher::verify()`. Rejected attempts therefore share one message and status. A successful verification
/// through a fallback pepper returns a replacement record made with the active pepper.
auto PasswordHandlerApp::login(const el::OptionValuesPtr &values) -> el::ExitCode {
    requireInteractiveTerminal();
    const auto username = values->getText("username"_el);
    requireUsername(username);
    const auto paths = storagePaths(values);
    validateStorageState(paths);
    auto database = paths.database.info().exists() ? UserDatabase::load(paths.database) : UserDatabase{};
    const auto storedHash = database.contains(username) ? el::PasswordHash::fromString(database.passwordHash(username))
                                                        : el::PasswordHash{};
    const auto password = PasswordPrompt::readPassword(terminal());
    initializeStorage(paths);
    auto hasher = PepperStore::loadHasher(paths.pepper);
    const auto verification = hasher.verify(password, storedHash);
    if (verification.isRejected()) {
        el::io::printLine("Login rejected."_el);
        return el::ExitCode::failure();
    }
    if (verification.replacementHash().has_value()) {
        static_cast<void>(database.trySetPassword(username, verification.replacementHash()->toString()));
        database.save(paths.database, el::PathCollisionMode::Overwrite);
    }
    el::io::printLine("Login accepted."_el);
    return el::ExitCode::success();
}

}
