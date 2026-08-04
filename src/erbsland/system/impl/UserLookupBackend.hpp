// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UserLookupBackend_fwd.hpp"

#include "../GroupId.hpp"
#include "../GroupName.hpp"
#include "../UserId.hpp"
#include "../UserName.hpp"

#include "../../text/String.hpp"
#include "../../text/StringEditor.hpp"

namespace erbsland::system::impl {

/// Backend interface for platform user and group lookups.
/// @tested{UserLookupTest}
class UserLookupBackend {
public:
    // defaults
    virtual ~UserLookupBackend() = default;

public:
    /// Resolve an owner name from a platform owner identifier.
    [[nodiscard]] virtual auto userNameForId(const UserId &id) -> UserName = 0;
    /// Resolve a group name from a platform group identifier.
    [[nodiscard]] virtual auto groupNameForId(const GroupId &id) -> GroupName = 0;
    /// Resolve a platform owner identifier from an owner name.
    [[nodiscard]] virtual auto userIdForName(const UserName &name) -> UserId = 0;
    /// Resolve a platform group identifier from a group name.
    [[nodiscard]] virtual auto groupIdForName(const GroupName &name) -> GroupId = 0;
};

/// Create the platform backend.
[[nodiscard]] auto createUserLookupBackend() -> UserLookupBackendPtr;

}
