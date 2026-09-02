// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UserLookupBackend.hpp"
#include "WindowsErrorContext.hpp"

namespace erbsland::system::impl {

/// Windows implementation for user and group lookups.
class WindowsUserLookupBackend final : public UserLookupBackend {
public:
    /// Create a Windows user-lookup backend.
    WindowsUserLookupBackend() = default;

public: // implement UserLookupBackend
    [[nodiscard]] auto userNameForId(const UserId &id) -> UserName override;
    [[nodiscard]] auto groupNameForId(const GroupId &id) -> GroupName override;
    [[nodiscard]] auto userIdForName(const UserName &name) -> UserId override;
    [[nodiscard]] auto groupIdForName(const GroupName &name) -> GroupId override;

private:
    /// Throw an error for a failed native account lookup.
    [[noreturn]] static void throwLookupError(
        text::String reason, system::impl::WindowsErrorContext::ErrorCode errorCode);
    /// Resolve an account name from a textual Windows SID.
    [[nodiscard]] static auto accountNameForSidString(const text::String &sid) -> text::String;
    /// Resolve a textual Windows SID from an account name.
    [[nodiscard]] static auto sidStringForAccountName(const text::String &name) -> text::String;
};

}
