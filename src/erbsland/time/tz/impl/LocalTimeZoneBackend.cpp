// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LocalTimeZoneBackend.hpp"

namespace erbsland::time::tz::impl {

LocalTimeZoneCache::LocalTimeZoneCache(LocalTimeZoneBackendPtr backend) noexcept :
    _value{
        backend != nullptr ? LocalTimeZoneResolver::fromBackend(*backend)
                           : LocalTimeZoneResolver::fromTimeZone(std::nullopt)} {
}

auto localTimeZoneFromBackend(LocalTimeZoneBackend &backend) noexcept -> TimeZone {
    return LocalTimeZoneResolver::fromBackend(backend);
}

auto localTimeZoneFromSystem() noexcept -> TimeZone {
    try {
        return LocalTimeZoneCache{createLocalTimeZoneBackend()}.value();
    } catch (...) {
        return LocalTimeZoneResolver::fromTimeZone(std::nullopt);
    }
}

auto LocalTimeZoneResolver::fromTimeZone(std::optional<TimeZone> timeZone) noexcept -> TimeZone {
    auto result = timeZone.value_or(TimeZone{});
    result._isLocalTime = true;
    return result;
}

auto LocalTimeZoneResolver::fromBackend(LocalTimeZoneBackend &backend) noexcept -> TimeZone {
    return fromTimeZone(backend.timeZone());
}

}
