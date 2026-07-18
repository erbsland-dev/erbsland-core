// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UserLookupBackend.hpp"

#include "../WindowsErrorContext.hpp"

namespace erbsland::system::impl {

/// Windows implementation for user and group lookups.
class WindowsUserLookupBackend final : public UserLookupBackend {
public:
    WindowsUserLookupBackend() = default;

public: // implement UserLookupBackend
    [[nodiscard]] auto userNameForId(const UserId &id) -> UserName override;
    [[nodiscard]] auto groupNameForId(const GroupId &id) -> GroupName override;
    [[nodiscard]] auto userIdForName(const UserName &name) -> UserId override;
    [[nodiscard]] auto groupIdForName(const GroupName &name) -> GroupId override;

private:
    [[noreturn]] static void throwLookupError(
        const text::String &reason, system::WindowsErrorContext::ErrorCode errorCode);
    [[nodiscard]] static auto accountNameForSidString(const text::String &sid) -> text::String;
    [[nodiscard]] static auto sidStringForAccountName(const text::String &name) -> text::String;
};

}
