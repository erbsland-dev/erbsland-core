// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsLocalTimeZoneBackend.hpp"

#include "WindowsTimeZoneMap.hpp"

#include "../../../core/impl/WindowsApi.hpp"

#include <string_view>

namespace erbsland::time::tz::impl {

auto WindowsLocalTimeZoneBackend::timeZone() noexcept -> std::optional<TimeZone> {
    auto information = DYNAMIC_TIME_ZONE_INFORMATION{};
    if (GetDynamicTimeZoneInformation(&information) == TIME_ZONE_ID_INVALID) {
        return {};
    }
    auto length = std::size_t{};
    while (length < std::size(information.TimeZoneKeyName) && information.TimeZoneKeyName[length] != L'\0') {
        ++length;
    }
    if (length == 0) {
        return {};
    }
    const auto timeZoneId = timeZoneIdFromWindowsName(std::wstring_view{information.TimeZoneKeyName, length});
    if (!timeZoneId.has_value()) {
        return {};
    }
    return TimeZone{timeZoneId.value()};
}

auto createLocalTimeZoneBackend() -> LocalTimeZoneBackendPtr {
    return std::make_unique<WindowsLocalTimeZoneBackend>();
}

}
