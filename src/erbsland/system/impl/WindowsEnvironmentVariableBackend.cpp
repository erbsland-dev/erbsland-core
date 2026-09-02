// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsEnvironmentVariableBackend.hpp"

#include "WindowsErrorContext.hpp"

#include "../PlatformError.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../text/impl/PlatformU16StringAccess.hpp"
#include "../../text/impl/UnsafeU16StringBuffer.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"

#include <cstddef>
#include <memory>

namespace erbsland::system::impl {

using namespace text::literals;

auto WindowsEnvironmentVariableBackend::get(const text::String &name) const -> std::optional<text::String> {
    const auto nativeName = text::StringConverter{name}.toU16String();
    const auto nameAccess = text::impl::PlatformU16StringAccess{nativeName};
    ::SetLastError(ERROR_SUCCESS);
    auto bufferSize = ::GetEnvironmentVariableW(nameAccess.nullTerminatedWideCharPtr(), nullptr, 0U);
    if (bufferSize == 0U) {
        const auto errorCode = ::GetLastError();
        if (errorCode == ERROR_ENVVAR_NOT_FOUND) {
            return std::nullopt;
        }
        if (errorCode == ERROR_SUCCESS) {
            return text::String{};
        }
        throw PlatformError{
            "Failed to read an environment variable."_el, WindowsErrorContext::fromErrorCode(errorCode)};
    }

    while (true) {
        auto buffer = text::impl::UnsafeU16StringBuffer{static_cast<std::size_t>(bufferSize)};
        ::SetLastError(ERROR_SUCCESS);
        const auto length = ::GetEnvironmentVariableW(
            nameAccess.nullTerminatedWideCharPtr(), buffer.dataAsWide(), static_cast<DWORD>(buffer.dataSize()));
        if (length == 0U) {
            const auto errorCode = ::GetLastError();
            if (errorCode == ERROR_ENVVAR_NOT_FOUND) {
                return std::nullopt;
            }
            if (errorCode == ERROR_SUCCESS) {
                return text::String{};
            }
            throw PlatformError{
                "Failed to read an environment variable."_el, WindowsErrorContext::fromErrorCode(errorCode)};
        }
        if (length < buffer.dataSize()) {
            return buffer.takeAsUtf8(static_cast<std::size_t>(length));
        }
        bufferSize = length;
    }
}

void WindowsEnvironmentVariableBackend::set(const text::String &name, const text::String &value) {
    const auto nativeName = text::StringConverter{name}.toU16String();
    const auto nativeValue = text::StringConverter{value}.toU16String();
    const auto nameAccess = text::impl::PlatformU16StringAccess{nativeName};
    const auto valueAccess = text::impl::PlatformU16StringAccess{nativeValue};
    const auto *valueData = value.isEmpty() ? L"" : valueAccess.nullTerminatedWideCharPtr();
    if (::SetEnvironmentVariableW(nameAccess.nullTerminatedWideCharPtr(), valueData) == 0) {
        const auto errorCode = ::GetLastError();
        throw PlatformError{"Failed to set an environment variable."_el, WindowsErrorContext::fromErrorCode(errorCode)};
    }
}

void WindowsEnvironmentVariableBackend::remove(const text::String &name) {
    const auto nativeName = text::StringConverter{name}.toU16String();
    const auto nameAccess = text::impl::PlatformU16StringAccess{nativeName};
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
