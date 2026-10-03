// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixLocalTimeZoneBackend.hpp"

#include "../../../err/RuntimeError.hpp"
#include "../../../system/EnvironmentVariables.hpp"
#include "../../../text/CharSet.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringList.hpp"
#include "../../../unit/CpIndex.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace erbsland::time::tz::impl {

using namespace text;
using namespace text::literals;

auto PosixLocalTimeZoneBackend::nameFromPath(const String &path) -> String {
    const auto parts = StringList::fromSplit(path, CharSet{U'/'});
    auto index = parts.findLast("zoneinfo"_el);
    if (index.isNoIndex()) {
        return {};
    }
    ++index;
    if (!index.isWithin(parts.count())) {
        return {};
    }
    auto result = StringEditor{};
    for (; index.isWithin(parts.count()); ++index) {
        if (!result.isEmpty()) {
            result.append("/"_el);
        }
        result.append(parts.get(index));
    }
    return result;
}

auto PosixLocalTimeZoneBackend::nameFromEnvironment() -> String {
    const auto configuredValue = system::EnvironmentVariables{}.get("TZ"_el);
    if (!configuredValue.has_value() || configuredValue->isEmpty()) {
        return {};
    }
    auto value = *configuredValue;
    if (value.charAt(StringSide::Front) == U':') {
        value = value.slice(StringSide::Back, unit::CpIndex{1U});
    }
    if (const auto pathName = nameFromPath(value); !pathName.isEmpty()) {
        return pathName;
    }
    if (!value.isEmpty() && value.charAt(StringSide::Front) != U'/') {
        return value;
    }
    return {};
}

auto PosixLocalTimeZoneBackend::nameFromEtcTimezone() -> String {
    auto stream = std::ifstream{"/etc/timezone"};
    auto nativeValue = std::string{};
    if (!stream || !std::getline(stream, nativeValue)) {
        return {};
    }
    return String{nativeValue}.trimmed(CharSet{U'\r', U'\n', U' '}, StringSide::Back);
}

auto PosixLocalTimeZoneBackend::detectedTimeZone() noexcept -> std::optional<TimeZone> {
    try {
        auto name = nameFromEnvironment();
        if (name.isEmpty()) {
            std::error_code error;
            const auto link = std::filesystem::read_symlink("/etc/localtime", error);
            if (!error) {
                name = nameFromPath(String{link.generic_string()});
            }
        }
        if (name.isEmpty()) {
            name = nameFromEtcTimezone();
        }
        if (name.isEmpty()) {
            return {};
        }
        return TimeZone::fromName(name);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto createLocalTimeZoneBackend() -> LocalTimeZoneBackendPtr {
    return std::make_unique<PosixLocalTimeZoneBackend>();
}

}
