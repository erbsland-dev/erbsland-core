// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Path.hpp"
#include "../PathAccessProfile.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../text/StringView.hpp"

#include <aclapi.h>

#include <vector>

namespace erbsland::path::impl {

/// Builds native Windows security attributes for a portable path access profile.
/// @tested{WindowsPathInfoTest}
class WindowsAccessProfileSecurity final {
public:
    WindowsAccessProfileSecurity(const Path &path, PathAccessProfile profile);
    ~WindowsAccessProfileSecurity();

    WindowsAccessProfileSecurity(const WindowsAccessProfileSecurity &) = delete;
    auto operator=(const WindowsAccessProfileSecurity &) -> WindowsAccessProfileSecurity & = delete;

public:
    [[nodiscard]] auto securityAttributes() noexcept -> SECURITY_ATTRIBUTES * { return &_securityAttributes; }
    [[nodiscard]] auto acl() const noexcept -> ACL * { return _acl; }

private:
    [[noreturn]] static void throwProfileError(
        const text::StringView &reason, const Path &path, unsigned long errorCode);
    [[nodiscard]] static auto portableAccessMask() noexcept -> ACCESS_MASK;
    [[nodiscard]] static auto tokenInformationOrThrow(
        void *token, TOKEN_INFORMATION_CLASS informationClass, const Path &path) -> std::vector<BYTE>;
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
