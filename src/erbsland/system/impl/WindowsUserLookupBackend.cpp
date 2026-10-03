// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsUserLookupBackend.hpp"

#include "WindowsErrorContext.hpp"

#include "../PlatformError.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../text/impl/PlatformU16StringAccess.hpp"
#include "../../text/impl/UnsafeU16StringBuffer.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/u16/U16StringEditor.hpp"
#include "../../unit/U16Count.hpp"

#include <sddl.h>

#include <memory>
#include <utility>
#include <vector>

namespace erbsland::system::impl {

using namespace text;
using text::impl::PlatformU16StringAccess;
using text::impl::UnsafeU16StringBuffer;
using namespace text::literals;

void WindowsUserLookupBackend::throwLookupError(
    String reason, const system::impl::WindowsErrorContext::ErrorCode errorCode) {
    throw system::PlatformError{std::move(reason), system::impl::WindowsErrorContext::fromErrorCode(errorCode)};
}

auto WindowsUserLookupBackend::userNameForId(const UserId &id) -> UserName {
    return UserName::fromString(accountNameForSidString(id.value()));
}

auto WindowsUserLookupBackend::groupNameForId(const GroupId &id) -> GroupName {
    return GroupName::fromString(accountNameForSidString(id.value()));
}

auto WindowsUserLookupBackend::userIdForName(const UserName &name) -> UserId {
    return UserId{sidStringForAccountName(name.toString())};
}

auto WindowsUserLookupBackend::groupIdForName(const GroupName &name) -> GroupId {
    return GroupId{sidStringForAccountName(name.toString())};
}

auto WindowsUserLookupBackend::accountNameForSidString(const String &sid) -> String {
    const auto sidText = StringConverter{sid}.toU16String();
    const auto sidTextAccess = PlatformU16StringAccess{sidText};
    auto *rawSid = static_cast<void *>(nullptr);
    if (::ConvertStringSidToSidW(sidTextAccess.nullTerminatedWideCharPtr(), &rawSid) == 0) {
        throwLookupError("Cannot parse Windows SID."_el, ::GetLastError());
    }
    auto sidPtr = std::unique_ptr<void, decltype(&::LocalFree)>{rawSid, &::LocalFree};

    auto nameLength = DWORD{0};
    auto domainLength = DWORD{0};
    auto sidNameUse = SID_NAME_USE{};
    ::LookupAccountSidW(nullptr, sidPtr.get(), nullptr, &nameLength, nullptr, &domainLength, &sidNameUse);
    const auto lengthError = ::GetLastError();
    if (lengthError != ERROR_INSUFFICIENT_BUFFER) {
        throwLookupError("Cannot resolve Windows SID."_el, lengthError);
    }

    auto nameBuffer = UnsafeU16StringBuffer{static_cast<std::size_t>(nameLength)};
    auto domainBuffer = UnsafeU16StringBuffer{static_cast<std::size_t>(domainLength)};
    if (::LookupAccountSidW(
            nullptr,
            sidPtr.get(),
            nameBuffer.dataAsWide(),
            &nameLength,
            domainBuffer.dataAsWide(),
            &domainLength,
            &sidNameUse) == 0) {
        throwLookupError("Cannot resolve Windows SID."_el, ::GetLastError());
    }

    auto result = StringEditor{};
    if (domainLength > 0U) {
        result.append(domainBuffer.takeAsUtf8(unit::U16Count::fromSizeT(domainLength)));
        result.append("\\"_el);
    }
    result.append(nameBuffer.takeAsUtf8(unit::U16Count::fromSizeT(nameLength)));
    return result;
}

auto WindowsUserLookupBackend::sidStringForAccountName(const String &name) -> String {
    const auto accountName = StringConverter{name}.toU16String();
    const auto accountNameAccess = PlatformU16StringAccess{accountName};
    auto sidLength = DWORD{0};
    auto domainLength = DWORD{0};
    auto sidNameUse = SID_NAME_USE{};
    ::LookupAccountNameW(
        nullptr,
        accountNameAccess.nullTerminatedWideCharPtr(),
        nullptr,
        &sidLength,
        nullptr,
        &domainLength,
        &sidNameUse);
    const auto lengthError = ::GetLastError();
    if (lengthError != ERROR_INSUFFICIENT_BUFFER) {
        throwLookupError("Cannot resolve Windows account name."_el, lengthError);
    }

    auto sid = std::vector<unsigned char>(sidLength);
    auto domainBuffer = UnsafeU16StringBuffer{static_cast<std::size_t>(domainLength)};
    if (::LookupAccountNameW(
            nullptr,
            accountNameAccess.nullTerminatedWideCharPtr(),
            sid.data(),
            &sidLength,
            domainBuffer.dataAsWide(),
            &domainLength,
            &sidNameUse) == 0) {
        throwLookupError("Cannot resolve Windows account name."_el, ::GetLastError());
    }

    auto *sidText = static_cast<wchar_t *>(nullptr);
    if (::ConvertSidToStringSidW(sid.data(), &sidText) == 0) {
        throwLookupError("Cannot convert Windows SID."_el, ::GetLastError());
    }
    const auto freeSidText = std::unique_ptr<void, decltype(&::LocalFree)>{sidText, &::LocalFree};
    return StringConverter{std::wstring_view{sidText, std::wcslen(sidText)}}.toString();
}

[[nodiscard]] auto createUserLookupBackend() -> UserLookupBackendPtr {
    return std::make_unique<WindowsUserLookupBackend>();
}

}
