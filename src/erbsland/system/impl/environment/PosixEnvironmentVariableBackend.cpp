// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixEnvironmentVariableBackend.hpp"

#include "../PosixErrorContext.hpp"

#include "../../../text/impl/PlatformU8StringAccess.hpp"
#include "../../../text/Literals.hpp"
#include "../../PlatformError.hpp"

#include <cerrno>
#include <cstdlib>
#include <memory>
#include <string_view>

namespace erbsland::system::impl {

using namespace text;
using text::impl::PlatformU8StringAccess;
using namespace text::literals;

auto PosixEnvironmentVariableBackend::get(const String &name) const -> std::optional<String> {
    const auto nameAccess = PlatformU8StringAccess{name};
    const auto *value = std::getenv(nameAccess.nullTerminatedCharPtr());
    if (value == nullptr) {
        return std::nullopt;
    }
    return String{std::string_view{value}};
}

void PosixEnvironmentVariableBackend::set(const String &name, const String &value) {
    const auto nameAccess = PlatformU8StringAccess{name};
    const auto valueAccess = PlatformU8StringAccess{value};
    const auto *valueData = value.isEmpty() ? "" : valueAccess.nullTerminatedCharPtr();
    if (::setenv(nameAccess.nullTerminatedCharPtr(), valueData, 1) != 0) {
        const auto errorCode = errno;
        throw PlatformError{"Failed to set an environment variable."_el, PosixErrorContext::fromErrorCode(errorCode)};
    }
}

void PosixEnvironmentVariableBackend::remove(const String &name) {
    const auto nameAccess = PlatformU8StringAccess{name};
    if (::unsetenv(nameAccess.nullTerminatedCharPtr()) != 0) {
        const auto errorCode = errno;
        throw PlatformError{
            "Failed to remove an environment variable."_el, PosixErrorContext::fromErrorCode(errorCode)};
    }
}

auto createEnvironmentVariableBackend() -> EnvironmentVariableBackendPtr {
    return std::make_unique<PosixEnvironmentVariableBackend>();
}

}
