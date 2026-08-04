// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Path.hpp"
#include "../PathAccessProfile.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../text/String.hpp"

#include <aclapi.h>

#include <vector>

namespace erbsland::path::impl {

/// Builds native Windows security attributes for a portable path access profile.
/// @tested{WindowsPathInfoTest}
class WindowsAccessProfileSecurity final {
public:
    /// Build native security attributes for a path access profile.
    /// @param path The path used for diagnostic context.
    /// @param profile The portable access profile to apply.
    WindowsAccessProfileSecurity(const Path &path, PathAccessProfile profile);
    /// Release the native access-control list.
    ~WindowsAccessProfileSecurity();

    // defaults/deletions
    WindowsAccessProfileSecurity(const WindowsAccessProfileSecurity &) = delete;
    auto operator=(const WindowsAccessProfileSecurity &) -> WindowsAccessProfileSecurity & = delete;

public:
    /// Get the native security attributes for a path operation.
    [[nodiscard]] auto securityAttributes() noexcept -> SECURITY_ATTRIBUTES * { return &_securityAttributes; }
    /// Get the native access-control list.
    [[nodiscard]] auto acl() const noexcept -> ACL * { return _acl; }

private:
    /// Throw an error while constructing a native access profile.
    [[noreturn]] static void throwProfileError(text::String reason, const Path &path, unsigned long errorCode);
    /// Get the native access mask for portable profile permissions.
    [[nodiscard]] static auto portableAccessMask() noexcept -> ACCESS_MASK;
    /// Query security token information or throw on failure.
    [[nodiscard]] static auto tokenInformationOrThrow(
        void *token, TOKEN_INFORMATION_CLASS informationClass, const Path &path) -> std::vector<BYTE>;
    /// Append one access-control entry to a native ACL.
    static void addEntry(std::vector<EXPLICIT_ACCESS_W> &entries, void *sid, ACCESS_MASK accessMask, TRUSTEE_TYPE type);

private:
    SECURITY_DESCRIPTOR _securityDescriptor{};
    SECURITY_ATTRIBUTES _securityAttributes{};
    ACL *_acl = nullptr;
    std::vector<BYTE> _userInfo;
    std::vector<BYTE> _groupInfo;
    std::vector<BYTE> _everyoneSid;
};

}
