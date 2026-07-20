// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Date.hpp"
#include "Duration.hpp"
#include "Time.hpp"
#include "TimeOccurrenceInFold.hpp"
#include "TimeZoneId.hpp"

#include "tz/impl/Database_fwd.hpp"
#include "tz/impl/LocalTimeZoneBackend_fwd.hpp"
#include "tz/TimeOffset.hpp"

#include "../text/String.hpp"
#include "../text/StringEditor.hpp"
#include "../text/StringList.hpp"
#include "../unit/Version.hpp"

#include <optional>
#include <variant>

namespace erbsland::time {

/// A UTC, fixed-offset, or named IANA time zone.
/// @seedoc{/reference/time/date_and_time}
/// @tested{TimeZoneTest}
class TimeZone final {
    struct FixedOffset {
        Seconds offset;
        friend auto operator==(const FixedOffset &, const FixedOffset &) noexcept -> bool = default;
    };
    struct NamedZone {
        TimeZoneId id;
        friend auto operator==(const NamedZone &, const NamedZone &) noexcept -> bool = default;
    };

public:
    /// Create UTC.
    TimeZone() noexcept = default;
    /// Create a fixed offset from parts.
    /// The resulting offset is clamped into the range `-23:59:59..+23:59:59`. A zero offset creates UTC.
    /// @param hours The offset hours. Clamped into the range `-23..+23`.
    /// @param minutes The offset minutes. Clamped into the range `-59..+59`.
    /// @param seconds The offset seconds. Clamped into the range `-59..+59`.
    explicit TimeZone(Hours hours, Minutes minutes = Minutes{}, Seconds seconds = Seconds{}) noexcept;
    /// Create a fixed offset.
    /// The offset is normalized into the range `-23:59:59..+23:59:59`. A zero offset creates UTC.
    /// @param offset The total offset duration.
    explicit TimeZone(Duration offset) noexcept;
    /// Create a named time zone from a transient id.
    /// The UTC and not-found identifiers create UTC.
    /// @param id The time zone identifier.
    explicit TimeZone(TimeZoneId id) noexcept;

    // defaults
    ~TimeZone() = default;
    TimeZone(const TimeZone &) noexcept = default;
    auto operator=(const TimeZone &) noexcept -> TimeZone & = default;
    TimeZone(TimeZone &&) noexcept = default;
    auto operator=(TimeZone &&) noexcept -> TimeZone & = default;

public: // operators
    /// Compare time zones.
    [[nodiscard]] auto operator==(const TimeZone &other) const noexcept -> bool = default;

public: // tests
    /// Test if this zone is UTC.
    /// @return `true` if this is the UTC time zone.
    [[nodiscard]] auto isUtc() const noexcept -> bool;
    /// Test if this zone is a fixed non-zero UTC offset.
    /// @return `true` if this is a fixed offset other than UTC.
    [[nodiscard]] auto isStaticOffset() const noexcept -> bool;
    /// Test if this zone is a named IANA time zone.
    /// @return `true` if this is a named zone.
    [[nodiscard]] auto isNamed() const noexcept -> bool;
    /// Test if this zone originated from the system-local setting.
    /// @return `true` if this is the system-local zone.
    [[nodiscard]] constexpr auto isLocalTime() const noexcept -> bool { return _isLocalTime; }

public: // accessors
    /// Return the fixed offset, or zero for UTC and named zones.
    /// @return The fixed offset duration.
    [[nodiscard]] auto staticOffset() const noexcept -> Duration;
    /// Return the primary IANA zone name, or an empty string for UTC and fixed offsets.
    /// @return The zone name (e.g. `America/New_York`).
    [[nodiscard]] auto name() const -> text::String;
    /// Return the transient time-zone identifier, or the UTC identifier for UTC and fixed offsets.
    /// @return The zone identifier.
    [[nodiscard]] auto id() const noexcept -> TimeZoneId;

public:
    /// Test if a name is known.
    /// @param name The zone name to check.
    /// @return `true` if the zone is supported.
    [[nodiscard]] static auto isValidName(const text::String &name) noexcept -> bool;
    /// Create a named zone from a name.
    /// Also accepts special UTC names such as `UTC`, `GMT`, `Z`, and fixed-offset names such as `UTC+02:00`.
    /// @param name The zone name.
    /// @return The time zone, or `std::nullopt` if unknown.
    [[nodiscard]] static auto fromName(const text::String &name) noexcept -> std::optional<TimeZone>;
    /// Create a named zone from a name or throw.
    /// Also accepts special UTC names such as `UTC`, `GMT`, `Z`, and fixed-offset names such as `UTC+02:00`.
    /// @param name The zone name.
    /// @return The time zone.
    /// @throws err::ParseError if the zone name is unknown.
    [[nodiscard]] static auto fromNameOrThrow(const text::String &name) -> TimeZone;
    /// Return all supported zone names.
    /// @return A list of all IANA zone names in the bundled database.
    [[nodiscard]] static auto names() -> text::StringList;
    /// Return the bundled IANA database version.
    /// @return The version of the bundled time zone database.
    [[nodiscard]] static auto databaseVersion() noexcept -> unit::Version;
    /// Return UTC.
    [[nodiscard]] static auto utc() noexcept -> TimeZone { return {}; }
    /// Return the process-cached system-local base time zone.
    /// Unknown or unavailable platform settings produce local-marked UTC.
    /// @return The system-local zone.
    [[nodiscard]] static auto local() noexcept -> TimeZone;

private:
    friend class DateTime;
    friend class tz::impl::LocalTimeZoneResolver;
    [[nodiscard]] static auto abbreviation(tz::TimeOffset offset) -> text::String;
    [[nodiscard]] static auto database() noexcept -> const tz::impl::Database &;
    [[nodiscard]] static auto normalizeOffset(Seconds seconds) noexcept -> Seconds;
    [[nodiscard]] static auto parseFixedOffsetText(const text::String &text, Seconds &offset) noexcept -> bool;
    [[nodiscard]] static auto specialTimeZoneFromName(const text::String &name) noexcept -> std::optional<TimeZone>;
    [[nodiscard]] auto timeOffsetAtLocal(Date date, Time time, TimeOccurrenceInFold occurrence) const noexcept
        -> tz::TimeOffset;
    [[nodiscard]] auto timeOffsetAtUtc(Date date, Time time) const noexcept -> tz::TimeOffset;

private:
    std::variant<std::monostate, FixedOffset, NamedZone> _storage;
    bool _isLocalTime{false}; ///< Whether this zone originated from the system setting.
};

}
