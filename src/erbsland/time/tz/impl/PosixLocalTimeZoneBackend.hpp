// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LocalTimeZoneBackend.hpp"

#include "../../../text/String.hpp"

namespace erbsland::time::tz::impl {

/// POSIX backend for identifying the configured system-local IANA zone.
/// @tested{PosixLocalTimeZoneTest}
class PosixLocalTimeZoneBackend final : public LocalTimeZoneBackend {
public:
    /// Detect the configured local IANA time zone.
    /// @return The detected time zone, or no value if detection fails.
    [[nodiscard]] auto detectedTimeZone() noexcept -> std::optional<TimeZone> override;

private:
    /// Extract an IANA zone name from a system file path.
    /// @param path The system file path.
    /// @return The extracted IANA zone name, or an empty string if unavailable.
    [[nodiscard]] static auto nameFromPath(const text::String &path) -> text::String;
    /// Read an IANA zone name from the environment.
    /// @return The configured IANA zone name, or an empty string if unavailable.
    [[nodiscard]] static auto nameFromEnvironment() -> text::String;
    /// Read an IANA zone name from `/etc/timezone`.
    /// @return The configured IANA zone name, or an empty string if unavailable.
    [[nodiscard]] static auto nameFromEtcTimezone() -> text::String;
};

}
