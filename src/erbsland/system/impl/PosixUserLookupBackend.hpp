// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UserLookupBackend.hpp"

namespace erbsland::system::impl {

/// POSIX implementation for user and group lookups.
class PosixUserLookupBackend final : public UserLookupBackend {
public:
    PosixUserLookupBackend() = default;

public: // implement UserLookupBackend
    [[nodiscard]] auto userNameForId(const UserId &id) -> UserName override;
    [[nodiscard]] auto groupNameForId(const GroupId &id) -> GroupName override;
    [[nodiscard]] auto userIdForName(const UserName &name) -> UserId override;
    [[nodiscard]] auto groupIdForName(const GroupName &name) -> GroupId override;

private:
    [[noreturn]] static void throwLookupError(const text::StringView &reason, int status);
    [[nodiscard]] static auto parseId(const text::StringView &id, const text::StringView &kind) -> unsigned long;
};

}
