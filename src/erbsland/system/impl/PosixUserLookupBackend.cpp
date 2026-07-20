// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixUserLookupBackend.hpp"

#include "../PlatformError.hpp"
#include "../PosixErrorContext.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"

#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <utility>

namespace erbsland::system::impl {

using namespace text::literals;

void PosixUserLookupBackend::throwLookupError(text::String reason, const int status) {
    throw system::PlatformError{std::move(reason), system::PosixErrorContext::fromErrorCode(status)};
}

auto PosixUserLookupBackend::userNameForId(const UserId &id) -> UserName {
    const auto uid = static_cast<uid_t>(parseId(id.value(), "user"_el));
    const auto configuredBufferSize = ::sysconf(_SC_GETPW_R_SIZE_MAX);
    auto bufferSize =
        configuredBufferSize > 1024L ? static_cast<std::size_t>(configuredBufferSize) : std::size_t{1024U};
    while (true) {
        auto buffer = std::make_unique<char[]>(bufferSize);
        auto password = passwd{};
        auto *result = static_cast<passwd *>(nullptr);
        const auto status = ::getpwuid_r(uid, &password, buffer.get(), bufferSize, &result);
        if (status == 0 && result != nullptr) {
            return UserName{text::String{std::string_view{result->pw_name}}};
        }
        if (status == ERANGE) {
            bufferSize *= 2U;
            continue;
        }
        if (status == 0) {
            throw system::PlatformError{"Cannot resolve POSIX user identifier."_el};
        }
        throwLookupError("Cannot resolve POSIX user identifier."_el, status);
    }
}

auto PosixUserLookupBackend::groupNameForId(const GroupId &id) -> GroupName {
    const auto gid = static_cast<gid_t>(parseId(id.value(), "group"_el));
    const auto configuredBufferSize = ::sysconf(_SC_GETGR_R_SIZE_MAX);
    auto bufferSize =
        configuredBufferSize > 1024L ? static_cast<std::size_t>(configuredBufferSize) : std::size_t{1024U};
    while (true) {
        auto buffer = std::make_unique<char[]>(bufferSize);
        auto group = ::group{};
        auto *result = static_cast<::group *>(nullptr);
        const auto status = ::getgrgid_r(gid, &group, buffer.get(), bufferSize, &result);
        if (status == 0 && result != nullptr) {
            return GroupName{text::String{std::string_view{result->gr_name}}};
        }
        if (status == ERANGE) {
            bufferSize *= 2U;
            continue;
        }
        if (status == 0) {
            throw system::PlatformError{"Cannot resolve POSIX group identifier."_el};
        }
        throwLookupError("Cannot resolve POSIX group identifier."_el, status);
    }
}

auto PosixUserLookupBackend::userIdForName(const UserName &name) -> UserId {
    const auto nameText = text::StringConverter{name.name()}.toStdString();
    const auto configuredBufferSize = ::sysconf(_SC_GETPW_R_SIZE_MAX);
    auto bufferSize =
        configuredBufferSize > 1024L ? static_cast<std::size_t>(configuredBufferSize) : std::size_t{1024U};
    while (true) {
        auto buffer = std::make_unique<char[]>(bufferSize);
        auto password = passwd{};
        auto *result = static_cast<passwd *>(nullptr);
        const auto status = ::getpwnam_r(nameText.c_str(), &password, buffer.get(), bufferSize, &result);
        if (status == 0 && result != nullptr) {
            return UserId{text::String::fromInteger(static_cast<unsigned long>(result->pw_uid))};
        }
        if (status == ERANGE) {
            bufferSize *= 2U;
            continue;
        }
        if (status == 0) {
            throw system::PlatformError{"Cannot resolve POSIX user name."_el};
        }
        throwLookupError("Cannot resolve POSIX user name."_el, status);
    }
}

auto PosixUserLookupBackend::groupIdForName(const GroupName &name) -> GroupId {
    const auto nameText = text::StringConverter{name.name()}.toStdString();
    const auto configuredBufferSize = ::sysconf(_SC_GETGR_R_SIZE_MAX);
    auto bufferSize =
        configuredBufferSize > 1024L ? static_cast<std::size_t>(configuredBufferSize) : std::size_t{1024U};
    while (true) {
        auto buffer = std::make_unique<char[]>(bufferSize);
        auto group = ::group{};
        auto *result = static_cast<::group *>(nullptr);
        const auto status = ::getgrnam_r(nameText.c_str(), &group, buffer.get(), bufferSize, &result);
        if (status == 0 && result != nullptr) {
            return GroupId{text::String::fromInteger(static_cast<unsigned long>(result->gr_gid))};
        }
        if (status == ERANGE) {
            bufferSize *= 2U;
            continue;
        }
        if (status == 0) {
            throw system::PlatformError{"Cannot resolve POSIX group name."_el};
        }
        throwLookupError("Cannot resolve POSIX group name."_el, status);
    }
}

auto PosixUserLookupBackend::parseId(const text::String &id, const text::String &kind) -> unsigned long {
    static_cast<void>(kind);
    const auto idText = text::StringConverter{id}.toStdString();
    auto result = 0UL;
    const auto *begin = idText.data();
    const auto *end = begin + idText.size();
    const auto [position, error] = std::from_chars(begin, end, result);
    if (error != std::errc{} || position != end || result > std::numeric_limits<unsigned int>::max()) {
        throw err::ParameterError{"Invalid POSIX user or group identifier."_el, kind};
    }
    return result;
}

[[nodiscard]] auto createUserLookupBackend() -> UserLookupBackendPtr {
    return std::make_unique<PosixUserLookupBackend>();
}

}
