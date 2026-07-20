// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixLocalTimeZoneBackend.hpp"

#include "../../../text/StringConverter.hpp"

#include <cstdlib>
#include <fstream>

namespace erbsland::time::tz::impl {

auto PosixLocalTimeZoneBackend::nameFromPath(const std::filesystem::path &path) -> std::string {
    const auto value = path.generic_string();
    constexpr auto marker = std::string_view{"zoneinfo/"};
    const auto markerPosition = value.rfind(marker);
    if (markerPosition == std::string::npos) {
        return {};
    }
    return value.substr(markerPosition + marker.size());
}

auto PosixLocalTimeZoneBackend::nameFromEnvironment() -> std::string {
    const auto *rawValue = std::getenv("TZ");
    if (rawValue == nullptr || *rawValue == '\0') {
        return {};
    }
    auto value = std::string{rawValue};
    if (value.front() == ':') {
        value.erase(value.begin());
    }
    if (const auto pathName = nameFromPath(value); !pathName.empty()) {
        return pathName;
    }
    if (!value.empty() && value.front() != '/') {
        return value;
    }
    return {};
}

auto PosixLocalTimeZoneBackend::nameFromEtcTimezone() -> std::string {
    auto stream = std::ifstream{"/etc/timezone"};
    auto value = std::string{};
    if (stream && std::getline(stream, value)) {
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == ' ')) {
            value.pop_back();
        }
    }
    return value;
}

auto PosixLocalTimeZoneBackend::timeZone() noexcept -> std::optional<TimeZone> {
    try {
        auto name = nameFromEnvironment();
        if (name.empty()) {
            std::error_code error;
            const auto link = std::filesystem::read_symlink("/etc/localtime", error);
            if (!error) {
                name = nameFromPath(link);
            }
        }
        if (name.empty()) {
            name = nameFromEtcTimezone();
        }
        if (name.empty()) {
            return {};
        }
        return TimeZone::fromName(text::String{name});
    } catch (...) {
        return {};
    }
}

auto createLocalTimeZoneBackend() -> LocalTimeZoneBackendPtr {
    return std::make_unique<PosixLocalTimeZoneBackend>();
}

}
