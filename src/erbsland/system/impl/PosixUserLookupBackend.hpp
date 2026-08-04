// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UserLookupBackend.hpp"

namespace erbsland::system::impl {

/// POSIX implementation for user and group lookups.
class PosixUserLookupBackend final : public UserLookupBackend {
public:
    /// Create a POSIX user-lookup backend.
    PosixUserLookupBackend() = default;

public: // implement UserLookupBackend
    [[nodiscard]] auto userNameForId(const UserId &id) -> UserName override;
    [[nodiscard]] auto groupNameForId(const GroupId &id) -> GroupName override;
    [[nodiscard]] auto userIdForName(const UserName &name) -> UserId override;
    [[nodiscard]] auto groupIdForName(const GroupName &name) -> GroupId override;

private:
    /// Throw an error for a failed POSIX account lookup.
    [[noreturn]] static void throwLookupError(text::String reason, int status);
    /// Parse a POSIX user or group identifier.
    [[nodiscard]] static auto parseId(const text::String &id, const text::String &kind) -> unsigned long;
};

}
