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
    virtual ~LocalTimeZoneBackend() = default;

public:
    /// Read the configured time zone.
    /// @return The zone, or no value if the platform setting cannot be identified.
    [[nodiscard]] virtual auto timeZone() noexcept -> std::optional<TimeZone> = 0;
};

/// Immutable cache for a platform-provided local time zone.
/// @tested{TimeOffsetTest}
class LocalTimeZoneCache final {
public:
    /// Read, mark and store the zone from the backend.
    explicit LocalTimeZoneCache(LocalTimeZoneBackendPtr backend) noexcept;

public:
    /// Return the cached local-marked zone.
    [[nodiscard]] auto value() const noexcept -> const TimeZone & { return _value; }

private:
    TimeZone _value;
};

/// Resolve platform zone names into local-marked time zones.
/// @tested{TimeOffsetTest}
class LocalTimeZoneResolver final {
public:
    /// Mark a platform zone as local, falling back to local-marked UTC.
    [[nodiscard]] static auto fromTimeZone(std::optional<TimeZone> timeZone) noexcept -> TimeZone;
    /// Read and resolve the result of a backend.
    [[nodiscard]] static auto fromBackend(LocalTimeZoneBackend &backend) noexcept -> TimeZone;
};

/// Read and mark a local zone from a backend.
/// @param backend The backend to query.
/// @return The local-marked zone, or local-marked UTC when unavailable.
[[nodiscard]] auto localTimeZoneFromBackend(LocalTimeZoneBackend &backend) noexcept -> TimeZone;

/// Read and mark the local zone from the platform backend.
/// @return The local-marked zone, or local-marked UTC when unavailable.
[[nodiscard]] auto localTimeZoneFromSystem() noexcept -> TimeZone;

/// Create the platform backend.
[[nodiscard]] auto createLocalTimeZoneBackend() -> LocalTimeZoneBackendPtr;

}
