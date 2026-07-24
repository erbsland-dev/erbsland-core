// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StorageFile.hpp"
#include "UserDatabase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/conf/ValueType.hpp>
#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/core/ApplicationErrorContext.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>

namespace demo {

using namespace el::text::literals;

namespace {

/// Throw a database-format error associated with its source path.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
[[noreturn]] void throwInvalidDatabase(const el::Path &path, const el::String &description) {
    auto context = el::core::ApplicationErrorContext{"Invalid user database"_el, description};
    context.setSourcePath(path.toString());
    throw el::ApplicationError{context};
}

}

auto UserDatabase::load(const el::Path &path) -> UserDatabase {
    el::conf::DocumentPtr document;
    try {
        document = el::conf::Parser{}.parseTextOrThrow(path.content().readTextOrThrow());
    } catch (const el::Exception &) {
        throwInvalidDatabase(path, "The file is not a valid ELCL document."_el);
    }

    auto result = UserDatabase{};
    if (document->size() == 0U) {
        return result;
    }
    if (document->size() != 1U || !document->hasValue("Users"_el)) {
        throwInvalidDatabase(path, "The document must contain only the Users section list."_el);
    }
    const auto users = document->valueOrThrow("Users"_el);
    if (users->type() != el::conf::ValueType::SectionList) {
        throwInvalidDatabase(path, "Users must be a section list."_el);
    }
    for (const auto &entry : *users) {
        if (entry->size() != 2U || !entry->hasValue("Username"_el) || !entry->hasValue("Password Hash"_el)) {
            throwInvalidDatabase(
                path, "Every Users entry must contain only Username and Password Hash text values."_el);
        }
        el::String username;
        el::String passwordHash;
        try {
            username = entry->getTextOrThrow("Username"_el);
            passwordHash = entry->getTextOrThrow("Password Hash"_el);
        } catch (const el::Exception &) {
            throwInvalidDatabase(path, "Username and Password Hash must both be text values."_el);
        }
        if (username.isEmpty()) {
            throwInvalidDatabase(path, "Stored usernames must not be empty."_el);
        }
        if (!result._users.tryInsert(username, passwordHash)) {
            throwInvalidDatabase(path, "The database contains a duplicate exact username."_el);
        }
    }
    return result;
}

auto UserDatabase::contains(const el::String &username) const -> bool {
    return _users.contains(username);
}

auto UserDatabase::passwordHash(const el::String &username) const -> el::String {
    return _users.get(username, {});
}

auto UserDatabase::tryAdd(const el::String &username, const el::String &passwordHash) -> bool {
    return _users.tryInsert(username, passwordHash);
}

auto UserDatabase::trySetPassword(const el::String &username, const el::String &passwordHash) -> bool {
    return _users.tryReplace(username, passwordHash);
}

auto UserDatabase::tryRemove(const el::String &username) -> bool {
    if (!_users.contains(username)) {
        return false;
    }
    _users.remove(username);
    return true;
}

void UserDatabase::save(const el::Path &path, const el::PathCollisionMode collisionMode) const {
    auto output = el::StringEditor{"@version: \"1.0\"\n"_el};
    for (const auto &[username, passwordHash] : _users) {
        output.append("\n*[Users]*\n"_el);
        appendElclText(output, "Username"_el, username);
        appendElclText(output, "Password Hash"_el, passwordHash);
    }
    writeStorageFile(path, el::String{output}, collisionMode);
}

}
