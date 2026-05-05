// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../TimeAmounts.hpp"
#include "../TimeZoneId.hpp"

#include <compare>
#include <cstdint>

namespace erbsland::time::tz {

/// Offset details stored with a date/time value.
///
/// Holds the UTC offset, zone identifier, and daylight saving information
/// for a specific point in time.
/// @tested{TimeOffsetTest}
class TimeOffset final {
public:
    /// Create UTC offset.
    TimeOffset() noexcept = default;
    /// Create a fixed offset.
    /// @param offset The UTC offset.
    explicit TimeOffset(Seconds offset) noexcept : _offset{offset} {}
    /// Create a named-zone offset.
    /// @param offset The UTC offset.
    /// @param isDst Whether daylight saving time is active.
    /// @param zoneId The time zone identifier.
    /// @param abbreviationId The abbreviation index.
    TimeOffset(Seconds offset, bool isDst, TimeZoneId zoneId, uint8_t abbreviationId) noexcept :
        _offset{offset}, _zoneId{zoneId}, _abbreviationId{abbreviationId}, _isDst{isDst} {}

    // defaults
    ~TimeOffset() = default;
    TimeOffset(const TimeOffset &) noexcept = default;
    auto operator=(const TimeOffset &) noexcept -> TimeOffset & = default;

public: // operators
    [[nodiscard]] auto operator==(const TimeOffset &other) const noexcept -> bool = default;

public: // tests
    /// Test if this offset represents UTC without a named zone.
    [[nodiscard]] constexpr auto isUtc() const noexcept -> bool { return _zoneId.isUtc() && _offset.isZero(); }
    /// Test if this offset is a fixed non-zero UTC offset.
    [[nodiscard]] constexpr auto isStaticOffset() const noexcept -> bool {
        return _zoneId.isUtc() && !_offset.isZero();
    }
    /// Test if this offset is associated with a named time zone.
    [[nodiscard]] constexpr auto isZone() const noexcept -> bool { return !_zoneId.isUtc(); }
    /// Test if daylight saving time is active for this offset.
    [[nodiscard]] constexpr auto isDst() const noexcept -> bool { return _isDst; }

public: // accessors
    /// Return the UTC offset in seconds.
    [[nodiscard]] constexpr auto offset() const noexcept -> Seconds { return _offset; }
    /// Return the time zone identifier.
    [[nodiscard]] constexpr auto zoneId() const noexcept -> TimeZoneId { return _zoneId; }
    /// Return the time zone abbreviation table index.
    [[nodiscard]] constexpr auto abbreviationId() const noexcept -> uint8_t { return _abbreviationId; }

private:
    Seconds _offset;
    TimeZoneId _zoneId;
    uint8_t _abbreviationId{0};
    bool _isDst{false};
};

}
