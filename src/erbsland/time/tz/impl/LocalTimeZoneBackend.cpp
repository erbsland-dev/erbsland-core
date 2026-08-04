// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LocalTimeZoneBackend.hpp"

namespace erbsland::time::tz::impl {

auto LocalTimeZoneBackend::localTimeZone() noexcept -> TimeZone {
    auto result = detectedTimeZone().value_or(TimeZone{});
    result.markAsLocalTime();
    return result;
}

}
