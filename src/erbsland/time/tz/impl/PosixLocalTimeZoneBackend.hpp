// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LocalTimeZoneBackend.hpp"

#include "../../../text/String.hpp"

namespace erbsland::time::tz::impl {

/// POSIX backend for identifying the configured system-local IANA zone.
/// @tested{TimeZoneLocalTest}
class PosixLocalTimeZoneBackend final : public LocalTimeZoneBackend {
public:
    [[nodiscard]] auto timeZone() noexcept -> std::optional<TimeZone> override;

private:
    [[nodiscard]] static auto nameFromPath(const text::String &path) -> text::String;
    [[nodiscard]] static auto nameFromEnvironment() -> text::String;
    [[nodiscard]] static auto nameFromEtcTimezone() -> text::String;
};

}
