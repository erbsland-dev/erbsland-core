// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsEnvironmentVariableBackend.hpp"

#include "../WindowsErrorContext.hpp"

#include "../../../core/impl/WindowsApi.hpp"
#include "../../../text/impl/PlatformU16StringAccess.hpp"
#include "../../../text/impl/StringSplitter.hpp"
#include "../../../text/impl/UnsafeU16StringBuffer.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringConverter.hpp"
#include "../../../text/StringList.hpp"
#include "../../PlatformError.hpp"

#include <cstddef>
#include <memory>

namespace erbsland::system::impl {

using namespace text;
using text::impl::PlatformU16StringAccess;
using text::impl::StringSplitter;
using text::impl::UnsafeU16StringBuffer;
using namespace text::literals;

auto WindowsEnvironmentVariableBackend::getNative(const String &name) const -> std::optional<U16String> {
    const auto nativeName = StringConverter{name}.toU16String();
    const auto nameAccess = PlatformU16StringAccess{nativeName};
    ::SetLastError(ERROR_SUCCESS);
    auto bufferSize = ::GetEnvironmentVariableW(nameAccess.nullTerminatedWideCharPtr(), nullptr, 0U);
    if (bufferSize == 0U) {
        const auto errorCode = ::GetLastError();
        if (errorCode == ERROR_ENVVAR_NOT_FOUND) {
            return std::nullopt;
        }
        if (errorCode == ERROR_SUCCESS) {
            return U16String{};
        }
        throw PlatformError{
            "Failed to read an environment variable."_el, WindowsErrorContext::fromErrorCode(errorCode)};
    }

    while (true) {
        auto buffer = UnsafeU16StringBuffer{static_cast<std::size_t>(bufferSize)};
        ::SetLastError(ERROR_SUCCESS);
        const auto length = ::GetEnvironmentVariableW(
            nameAccess.nullTerminatedWideCharPtr(), buffer.dataAsWide(), static_cast<DWORD>(buffer.dataSize()));
        if (length == 0U) {
            const auto errorCode = ::GetLastError();
            if (errorCode == ERROR_ENVVAR_NOT_FOUND) {
                return std::nullopt;
            }
            if (errorCode == ERROR_SUCCESS) {
                return U16String{};
            }
            throw PlatformError{
                "Failed to read an environment variable."_el, WindowsErrorContext::fromErrorCode(errorCode)};
        }
        if (length < buffer.dataSize()) {
            return U16String{buffer.take(static_cast<std::size_t>(length))};
        }
        bufferSize = length;
    }
}

auto WindowsEnvironmentVariableBackend::get(const String &name) const -> std::optional<String> {
    if (const auto value = getNative(name)) {
        return StringConverter{*value}.toString();
    }
    return {};
}

auto WindowsEnvironmentVariableBackend::getPathVariable(const String &name) const -> std::optional<String> {
    const auto value = getNative(name);
    if (value && value->isEncodingValid()) {
        return StringConverter{*value}.toString();
    }
    return {};
}

auto WindowsEnvironmentVariableBackend::getPath() const -> std::optional<String> {
    const auto value = getNative(cPathVariable);
    if (!value) {
        return {};
    }
    auto entries = StringList{};
    auto splitter = StringSplitter<U16String>{*value, U';'};
    while (!splitter.isAtEnd()) {
        const auto entry = splitter.next();
        if (entry.isEncodingValid()) {
            entries.append(StringConverter{entry}.toString());
        }
    }
    // An all-malformed PATH must not become an empty entry denoting the current directory.
    if (entries.isEmpty()) {
        return {};
    }
    return entries.join(";"_el);
}

void WindowsEnvironmentVariableBackend::set(const String &name, const String &value) {
    const auto nativeName = StringConverter{name}.toU16String();
    const auto nativeValue = StringConverter{value}.toU16String();
    const auto nameAccess = PlatformU16StringAccess{nativeName};
    const auto valueAccess = PlatformU16StringAccess{nativeValue};
    const auto *valueData = value.isEmpty() ? L"" : valueAccess.nullTerminatedWideCharPtr();
    if (::SetEnvironmentVariableW(nameAccess.nullTerminatedWideCharPtr(), valueData) == 0) {
        const auto errorCode = ::GetLastError();
        throw PlatformError{"Failed to set an environment variable."_el, WindowsErrorContext::fromErrorCode(errorCode)};
    }
}

void WindowsEnvironmentVariableBackend::remove(const String &name) {
    const auto nativeName = StringConverter{name}.toU16String();
    const auto nameAccess = PlatformU16StringAccess{nativeName};
    if (::SetEnvironmentVariableW(nameAccess.nullTerminatedWideCharPtr(), nullptr) == 0) {
        const auto errorCode = ::GetLastError();
        if (errorCode == ERROR_ENVVAR_NOT_FOUND) {
            return;
        }
        throw PlatformError{
            "Failed to remove an environment variable."_el, WindowsErrorContext::fromErrorCode(errorCode)};
    }
}

auto createEnvironmentVariableBackend() -> EnvironmentVariableBackendPtr {
    return std::make_unique<WindowsEnvironmentVariableBackend>();
}

}
