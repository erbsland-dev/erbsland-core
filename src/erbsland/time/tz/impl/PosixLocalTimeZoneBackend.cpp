// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixLocalTimeZoneBackend.hpp"

#include "../../../text/CharSet.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringList.hpp"
#include "../../../unit/CpIndex.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace erbsland::time::tz::impl {

using namespace text::literals;

auto PosixLocalTimeZoneBackend::nameFromPath(const text::String &path) -> text::String {
    const auto parts = text::StringList::fromSplit(path, text::CharSet{U'/'});
    auto index = parts.findLast("zoneinfo"_el);
    if (index.isNoIndex()) {
        return {};
    }
    ++index;
    if (!index.isWithin(parts.count())) {
        return {};
    }
    auto result = text::StringEditor{};
    for (; index.isWithin(parts.count()); ++index) {
        if (!result.isEmpty()) {
            result.append("/"_el);
        }
        result.append(parts.get(index));
    }
    return result;
}

auto PosixLocalTimeZoneBackend::nameFromEnvironment() -> text::String {
    const auto *rawValue = std::getenv("TZ");
    if (rawValue == nullptr || *rawValue == '\0') {
        return {};
    }
    auto value = text::String{std::string_view{rawValue}};
    if (value.charAt(text::StringSide::Front) == U':') {
        value = value.slice(text::StringSide::Back, unit::CpIndex{1U});
    }
    if (const auto pathName = nameFromPath(value); !pathName.isEmpty()) {
        return pathName;
    }
    if (!value.isEmpty() && value.charAt(text::StringSide::Front) != U'/') {
        return value;
    }
    return {};
}

auto PosixLocalTimeZoneBackend::nameFromEtcTimezone() -> text::String {
    auto stream = std::ifstream{"/etc/timezone"};
    auto nativeValue = std::string{};
    if (!stream || !std::getline(stream, nativeValue)) {
        return {};
    }
    return text::String{nativeValue}.trimmed(text::CharSet{U'\r', U'\n', U' '}, text::StringSide::Back);
}

auto PosixLocalTimeZoneBackend::timeZone() noexcept -> std::optional<TimeZone> {
    try {
        auto name = nameFromEnvironment();
        if (name.isEmpty()) {
            std::error_code error;
            const auto link = std::filesystem::read_symlink("/etc/localtime", error);
            if (!error) {
                name = nameFromPath(text::String{link.generic_string()});
            }
        }
        if (name.isEmpty()) {
            name = nameFromEtcTimezone();
        }
        if (name.isEmpty()) {
            return {};
        }
        return TimeZone::fromName(name);
    } catch (...) {
        return {};
    }
}

auto createLocalTimeZoneBackend() -> LocalTimeZoneBackendPtr {
    return std::make_unique<PosixLocalTimeZoneBackend>();
}

}
