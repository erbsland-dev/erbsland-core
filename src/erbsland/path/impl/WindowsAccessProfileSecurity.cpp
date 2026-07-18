// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsAccessProfileSecurity.hpp"

#include "../PathError.hpp"

#include "../../system/WindowsErrorContext.hpp"
#include "../../text/Literals.hpp"

#include <memory>
#include <utility>

namespace erbsland::path::impl {

using namespace text::literals;

WindowsAccessProfileSecurity::WindowsAccessProfileSecurity(const Path &path, const PathAccessProfile profile) {
    auto token = HANDLE{};
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) == 0) {
        throwProfileError("Cannot access the current process token."_el, path, GetLastError());
    }
    const auto closeToken = std::unique_ptr<void, decltype(&CloseHandle)>{token, &CloseHandle};

    _userInfo = tokenInformationOrThrow(token, TokenUser, path);
    auto *user = reinterpret_cast<TOKEN_USER *>(_userInfo.data());
    auto entries = std::vector<EXPLICIT_ACCESS_W>{};
    addEntry(entries, user->User.Sid, GENERIC_ALL, TRUSTEE_IS_USER);

    if (profile == PathAccessProfile::UserAndGroup) {
        _groupInfo = tokenInformationOrThrow(token, TokenPrimaryGroup, path);
        auto *group = reinterpret_cast<TOKEN_PRIMARY_GROUP *>(_groupInfo.data());
        addEntry(entries, group->PrimaryGroup, portableAccessMask(), TRUSTEE_IS_GROUP);
    } else if (profile == PathAccessProfile::Everyone) {
        _everyoneSid.resize(SECURITY_MAX_SID_SIZE);
        auto sidSize = DWORD{SECURITY_MAX_SID_SIZE};
        if (CreateWellKnownSid(WinWorldSid, nullptr, _everyoneSid.data(), &sidSize) == 0) {
            throwProfileError("Cannot create the Everyone SID."_el, path, GetLastError());
        }
        _everyoneSid.resize(sidSize);
        addEntry(entries, _everyoneSid.data(), portableAccessMask(), TRUSTEE_IS_GROUP);
    }

    const auto status = SetEntriesInAclW(static_cast<ULONG>(entries.size()), entries.data(), nullptr, &_acl);
    if (status != ERROR_SUCCESS) {
        throwProfileError("Cannot create a Windows access profile DACL."_el, path, status);
    }
    if (InitializeSecurityDescriptor(&_securityDescriptor, SECURITY_DESCRIPTOR_REVISION) == 0) {
        throwProfileError("Cannot initialize a Windows security descriptor."_el, path, GetLastError());
    }
    if (SetSecurityDescriptorDacl(&_securityDescriptor, TRUE, _acl, FALSE) == 0) {
        throwProfileError("Cannot assign a Windows access profile DACL."_el, path, GetLastError());
    }
    _securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    _securityAttributes.lpSecurityDescriptor = &_securityDescriptor;
    _securityAttributes.bInheritHandle = FALSE;
}

WindowsAccessProfileSecurity::~WindowsAccessProfileSecurity() {
    if (_acl != nullptr) {
        static_cast<void>(LocalFree(_acl));
    }
}

void WindowsAccessProfileSecurity::throwProfileError(
    const text::String &reason, const Path &path, const unsigned long errorCode) {
    throw PathError{PathErrorContext{"File permissions could not be changed"_el, reason}
            .setSourcePath(path.toString())
            .setPlatformContext(system::WindowsErrorContext::fromErrorCode(errorCode))};
}

auto WindowsAccessProfileSecurity::portableAccessMask() noexcept -> ACCESS_MASK {
    return GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE;
}

auto WindowsAccessProfileSecurity::tokenInformationOrThrow(
    void *token, const TOKEN_INFORMATION_CLASS informationClass, const Path &path) -> std::vector<BYTE> {
    auto size = DWORD{0};
    static_cast<void>(GetTokenInformation(token, informationClass, nullptr, 0, &size));
    if (size == 0U || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        throwProfileError("Cannot query the current process token."_el, path, GetLastError());
    }
    auto buffer = std::vector<BYTE>(size);
    if (GetTokenInformation(token, informationClass, buffer.data(), size, &size) == 0) {
        throwProfileError("Cannot read the current process token."_el, path, GetLastError());
    }
    return buffer;
}

void WindowsAccessProfileSecurity::addEntry(
    std::vector<EXPLICIT_ACCESS_W> &entries, void *sid, const ACCESS_MASK accessMask, const TRUSTEE_TYPE type) {
    auto entry = EXPLICIT_ACCESS_W{};
    entry.grfAccessPermissions = accessMask;
    entry.grfAccessMode = SET_ACCESS;
    entry.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    entry.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entry.Trustee.TrusteeType = type;
    entry.Trustee.ptstrName = static_cast<LPWSTR>(sid);
    entries.push_back(entry);
}

}
