// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LocalTimeZoneBackend_fwd.hpp"

#include "../../TimeZone.hpp"

#include <optional>

namespace erbsland::time::tz::impl {

/// Platform backend for identifying the configured system-local time zone.
/// @tested{TimeOffsetTest}
class LocalTimeZoneBackend {
public:
    // defaults
    virtual ~LocalTimeZoneBackend() = default;

public:
    /// Detect the configured time zone.
    /// @return The zone, or no value if the platform setting cannot be identified.
    [[nodiscard]] virtual auto detectedTimeZone() noexcept -> std::optional<TimeZone> = 0;
    /// Return the detected zone marked as local, falling back to local-marked UTC.
    [[nodiscard]] auto localTimeZone() noexcept -> TimeZone;
};

/// Create the platform backend.
/// @return The platform-specific backend.
/// @tested{TimeOffsetTest}
[[nodiscard]] auto createLocalTimeZoneBackend() -> LocalTimeZoneBackendPtr;

}
