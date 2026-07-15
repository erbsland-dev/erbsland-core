// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GroupId.hpp"
#include "GroupName.hpp"
#include "UserId.hpp"
#include "UserLookup_fwd.hpp"
#include "UserName.hpp"

#include "impl/UserLookupBackend.hpp"

#include "../text/String.hpp"
#include "../text/StringHashMap.hpp"
#include "../text/StringView.hpp"

#include <mutex>

namespace erbsland::system {

/// Cached lookup service for platform user and group identities.
/// @tested{UserLookupTest}
class UserLookup final {
public:
    /// Create a lookup service with the default platform backend.
    UserLookup();
    /// Create a lookup service with a custom backend.
    explicit UserLookup(impl::UserLookupBackendPtr backend);

    // defaults
    ~UserLookup() = default;
    UserLookup(const UserLookup &) = delete;
    auto operator=(const UserLookup &) -> UserLookup & = delete;

public:
    /// Resolve an owner name from a platform owner identifier.
    /// @throws path::PathError If the identifier cannot be resolved.
    [[nodiscard]] auto userNameForId(const UserId &id) -> UserName;
    /// Resolve a group name from a platform group identifier.
    /// @throws path::PathError If the identifier cannot be resolved.
    [[nodiscard]] auto groupNameForId(const GroupId &id) -> GroupName;
    /// Resolve a platform owner identifier from an owner name.
    /// @throws path::PathError If the name cannot be resolved.
    [[nodiscard]] auto userIdForName(const UserName &name) -> UserId;
    /// Resolve a platform group identifier from a group name.
    /// @throws path::PathError If the name cannot be resolved.
    [[nodiscard]] auto groupIdForName(const GroupName &name) -> GroupId;
    /// Clear all cached lookups.
    void clearCache() noexcept;

private:
    [[nodiscard]] auto cachedUserNameForId(const UserId &id) -> UserName;
    [[nodiscard]] auto cachedGroupNameForId(const GroupId &id) -> GroupName;
    [[nodiscard]] auto cachedUserIdForName(const UserName &name) -> UserId;
    [[nodiscard]] auto cachedGroupIdForName(const GroupName &name) -> GroupId;

private:
    impl::UserLookupBackendPtr _backend;
    std::mutex _mutex;
    text::StringHashMap<UserName> _userNames;
    text::StringHashMap<GroupName> _groupNames;
    text::StringHashMap<UserId> _userIds;
    text::StringHashMap<GroupId> _groupIds;
};

}
