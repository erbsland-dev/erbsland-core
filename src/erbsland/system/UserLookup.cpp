// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UserLookup.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::system {

using namespace text::literals;

UserLookup::UserLookup() : UserLookup{impl::createUserLookupBackend()} {
}

UserLookup::UserLookup(impl::UserLookupBackendPtr backend) : _backend{std::move(backend)} {
    if (_backend == nullptr) {
        throw err::ParameterError{"The user lookup backend is missing."_el, "backend"_el};
    }
}

auto UserLookup::userNameForId(const UserId &id) -> UserName {
    if (id.isEmpty()) {
        throw err::ParameterError{"Cannot resolve an empty user identifier."_el, "id"_el};
    }
    return cachedUserNameForId(id);
}

auto UserLookup::groupNameForId(const GroupId &id) -> GroupName {
    if (id.isEmpty()) {
        throw err::ParameterError{"Cannot resolve an empty group identifier."_el, "id"_el};
    }
    return cachedGroupNameForId(id);
}

auto UserLookup::userIdForName(const UserName &name) -> UserId {
    if (name.isEmpty()) {
        throw err::ParameterError{"Cannot resolve an empty user name."_el, "name"_el};
    }
    return cachedUserIdForName(name);
}

auto UserLookup::groupIdForName(const GroupName &name) -> GroupId {
    if (name.isEmpty()) {
        throw err::ParameterError{"Cannot resolve an empty group name."_el, "name"_el};
    }
    return cachedGroupIdForName(name);
}

void UserLookup::clearCache() noexcept {
    auto lock = std::scoped_lock{_mutex};
    _userNames.clear();
    _groupNames.clear();
    _userIds.clear();
    _groupIds.clear();
}

auto UserLookup::cachedUserNameForId(const UserId &id) -> UserName {
    auto lock = std::scoped_lock{_mutex};
    if (const auto cached = _userNames.get(id.value()); cached.has_value()) {
        return *cached;
    }
    auto result = _backend->userNameForId(id);
    _userNames.set(id.value(), result);
    return result;
}

auto UserLookup::cachedGroupNameForId(const GroupId &id) -> GroupName {
    auto lock = std::scoped_lock{_mutex};
    if (const auto cached = _groupNames.get(id.value()); cached.has_value()) {
        return *cached;
    }
    auto result = _backend->groupNameForId(id);
    _groupNames.set(id.value(), result);
    return result;
}

auto UserLookup::cachedUserIdForName(const UserName &name) -> UserId {
    auto lock = std::scoped_lock{_mutex};
    const auto key = name.toString();
    if (const auto cached = _userIds.get(key); cached.has_value()) {
        return *cached;
    }
    auto result = _backend->userIdForName(name);
    _userIds.set(key, result);
    return result;
}

auto UserLookup::cachedGroupIdForName(const GroupName &name) -> GroupId {
    auto lock = std::scoped_lock{_mutex};
    const auto key = name.toString();
    if (const auto cached = _groupIds.get(key); cached.has_value()) {
        return *cached;
    }
    auto result = _backend->groupIdForName(name);
    _groupIds.set(key, result);
    return result;
}

}
