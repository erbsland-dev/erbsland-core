// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LocalTimeZoneBackend.hpp"

namespace erbsland::time::tz::impl {

/// Windows backend for the configured local time zone.
/// @tested{TimeOffsetTest}
class WindowsLocalTimeZoneBackend final : public LocalTimeZoneBackend {
public:
    [[nodiscard]] auto timeZone() noexcept -> std::optional<TimeZone> override;
};

}
