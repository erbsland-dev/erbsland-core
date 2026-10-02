// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DateTime.hpp"
#include "TimeDelta.hpp"
#include "Timestamp_fwd.hpp"

#include "impl/TimestampTraits.hpp"

#include "../mem/ByteBlock.hpp"
#include "../text/String.hpp"

#include <compare>
#include <cstdint>
#include <ctime>
#include <optional>
#include <utility>

namespace erbsland::time {

/// A compact UTC instant with nanosecond precision and no display-zone metadata.
/// @seedoc{/reference/time/timestamp}
/// @tested{TimestampTest TimestampArithmeticTest TimestampConverterTest}
class Timestamp final {
    struct PrivateTag {};

public: // types
    /// The unbiased day count and nanoseconds since midnight.
    using RawValue = std::pair<int32_t, int64_t>;

public: // construction
    /// Create an invalid timestamp.
    constexpr Timestamp() noexcept = default;

    /// Create a UTC instant from calendar fields; an invalid date produces an invalid timestamp.
    /// @param date The UTC date.
    /// @param time The UTC time.
    Timestamp(Date date, Time time) noexcept;

    // defaults
    ~Timestamp() = default;
    Timestamp(const Timestamp &) noexcept = default;
    Timestamp(Timestamp &&) noexcept = default;
    auto operator=(const Timestamp &) noexcept -> Timestamp & = default;
    auto operator=(Timestamp &&) noexcept -> Timestamp & = default;

public: // operators
    /// Compare UTC instants; invalid timestamps sort first.
    [[nodiscard]] constexpr auto operator<=>(const Timestamp &) const noexcept -> std::strong_ordering = default;
    /// Test equality of UTC instants.
    [[nodiscard]] constexpr auto operator==(const Timestamp &) const noexcept -> bool = default;
    /// Return the precise signed delta from the other timestamp to this one.
    [[nodiscard]] auto operator-(Timestamp other) const noexcept -> TimeDelta { return other.timeDeltaTo(*this); }
    /// Return this timestamp with a fixed amount added.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    [[nodiscard]] auto operator+(T amount) const noexcept -> Timestamp {
        return added(amount);
    }
    /// Add a fixed amount in place.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    auto operator+=(T amount) noexcept -> Timestamp & {
        *this = added(amount);
        return *this;
    }
    /// Return this timestamp with a fixed amount subtracted.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    [[nodiscard]] auto operator-(T amount) const noexcept -> Timestamp {
        return subtracted(amount);
    }
    /// Subtract a fixed amount in place.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    auto operator-=(T amount) noexcept -> Timestamp & {
        *this = subtracted(amount);
        return *this;
    }

public: // tests
    /// Test whether this timestamp represents an instant.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _days >= 0; }
    /// Test whether this timestamp converts exactly to DateTime.
    [[nodiscard]] constexpr auto isValidDateTime() const noexcept -> bool { return isValid(); }

public: // accessors
    /// Return the UTC date, or an invalid date.
    [[nodiscard]] auto date() const noexcept -> Date;
    /// Return days since the Core epoch, or minus one for invalid timestamps.
    [[nodiscard]] constexpr auto dateAsDays() const noexcept -> Days { return Days{_days}; }
    /// Return the UTC time, or midnight for invalid timestamps.
    [[nodiscard]] auto time() const noexcept -> Time;
    /// Return nanoseconds since midnight, or zero for invalid timestamps.
    [[nodiscard]] constexpr auto timeAsNanoseconds() const noexcept -> Nanoseconds { return Nanoseconds{_nanoseconds}; }

public: // arithmetic
    /// Test whether applying a fixed amount would exceed the calendar range.
    /// @param amount The signed amount to add.
    /// @return Whether saturation would occur; false for invalid timestamps.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    [[nodiscard]] auto wouldAddSaturate(T amount) const noexcept -> bool;
    /// Return a shifted timestamp, saturating at the calendar boundaries.
    /// @param amount The signed amount to add.
    /// @return The shifted timestamp; invalid timestamps stay invalid.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    [[nodiscard]] auto added(T amount) const noexcept -> Timestamp;
    /// Return an exactly shifted timestamp; invalid timestamps stay invalid.
    /// @param amount The signed amount to add.
    /// @return The shifted timestamp.
    /// @throws err::OverflowError if the calendar range would be exceeded.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    [[nodiscard]] auto addedOrThrow(T amount) const -> Timestamp;
    /// Apply saturating arithmetic in place.
    /// @param amount The signed amount to add.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    void add(T amount) noexcept {
        *this = added(amount);
    }
    /// Apply exact arithmetic in place, leaving this timestamp unchanged on failure.
    /// @param amount The signed amount to add.
    /// @throws err::OverflowError if the calendar range would be exceeded.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    void addOrThrow(T amount) {
        *this = addedOrThrow(amount);
    }
    /// Test whether applying a fixed amount would exceed the calendar range.
    /// @param amount The signed amount to subtract.
    /// @return Whether saturation would occur; false for invalid timestamps.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    [[nodiscard]] auto wouldSubtractSaturate(T amount) const noexcept -> bool;
    /// Return a shifted timestamp, saturating at the calendar boundaries.
    /// @param amount The signed amount to subtract.
    /// @return The shifted timestamp; invalid timestamps stay invalid.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    [[nodiscard]] auto subtracted(T amount) const noexcept -> Timestamp;
    /// Return an exactly shifted timestamp; invalid timestamps stay invalid.
    /// @param amount The signed amount to subtract.
    /// @return The shifted timestamp.
    /// @throws err::OverflowError if the calendar range would be exceeded.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    [[nodiscard]] auto subtractedOrThrow(T amount) const -> Timestamp;
    /// Apply saturating arithmetic in place.
    /// @param amount The signed amount to subtract.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    void subtract(T amount) noexcept {
        *this = subtracted(amount);
    }
    /// Apply exact arithmetic in place, leaving this timestamp unchanged on failure.
    /// @param amount The signed amount to subtract.
    /// @throws err::OverflowError if the calendar range would be exceeded.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    void subtractOrThrow(T amount) {
        *this = subtractedOrThrow(amount);
    }

public: // distances
    /// Return the signed distance, truncated toward zero and saturated if necessary.
    /// @param other The target timestamp.
    /// @return The distance, or zero if either timestamp is invalid.
    [[nodiscard]] auto secondsTo(Timestamp other) const noexcept -> Seconds;
    /// Return the exact signed distance, truncated toward zero.
    /// @param other The target timestamp.
    /// @return The distance.
    /// @throws err::ParameterError if either timestamp is invalid.
    /// @throws err::OverflowError if the result does not fit.
    [[nodiscard]] auto secondsToOrThrow(Timestamp other) const -> Seconds;
    /// Test whether the distance would saturate; invalid operands return false.
    /// @param other The target timestamp.
    /// @return Whether the result exceeds its amount range.
    [[nodiscard]] auto wouldSecondsToSaturate(Timestamp other) const noexcept -> bool;
    /// Return the signed distance, truncated toward zero and saturated if necessary.
    /// @param other The target timestamp.
    /// @return The distance, or zero if either timestamp is invalid.
    [[nodiscard]] auto millisecondsTo(Timestamp other) const noexcept -> Milliseconds;
    /// Return the exact signed distance, truncated toward zero.
    /// @param other The target timestamp.
    /// @return The distance.
    /// @throws err::ParameterError if either timestamp is invalid.
    /// @throws err::OverflowError if the result does not fit.
    [[nodiscard]] auto millisecondsToOrThrow(Timestamp other) const -> Milliseconds;
    /// Test whether the distance would saturate; invalid operands return false.
    /// @param other The target timestamp.
    /// @return Whether the result exceeds its amount range.
    [[nodiscard]] auto wouldMillisecondsToSaturate(Timestamp other) const noexcept -> bool;
    /// Return the signed distance, truncated toward zero and saturated if necessary.
    /// @param other The target timestamp.
    /// @return The distance, or zero if either timestamp is invalid.
    [[nodiscard]] auto microsecondsTo(Timestamp other) const noexcept -> Microseconds;
    /// Return the exact signed distance, truncated toward zero.
    /// @param other The target timestamp.
    /// @return The distance.
    /// @throws err::ParameterError if either timestamp is invalid.
    /// @throws err::OverflowError if the result does not fit.
    [[nodiscard]] auto microsecondsToOrThrow(Timestamp other) const -> Microseconds;
    /// Test whether the distance would saturate; invalid operands return false.
    /// @param other The target timestamp.
    /// @return Whether the result exceeds its amount range.
    [[nodiscard]] auto wouldMicrosecondsToSaturate(Timestamp other) const noexcept -> bool;
    /// Return the signed distance, truncated toward zero and saturated if necessary.
    /// @param other The target timestamp.
    /// @return The distance, or zero if either timestamp is invalid.
    [[nodiscard]] auto nanosecondsTo(Timestamp other) const noexcept -> Nanoseconds;
    /// Return the exact signed distance, truncated toward zero.
    /// @param other The target timestamp.
    /// @return The distance.
    /// @throws err::ParameterError if either timestamp is invalid.
    /// @throws err::OverflowError if the result does not fit.
    [[nodiscard]] auto nanosecondsToOrThrow(Timestamp other) const -> Nanoseconds;
    /// Test whether the distance would saturate; invalid operands return false.
    /// @param other The target timestamp.
    /// @return Whether the result exceeds its amount range.
    [[nodiscard]] auto wouldNanosecondsToSaturate(Timestamp other) const noexcept -> bool;
    /// Return the signed distance, truncated toward zero and saturated if necessary.
    /// @param other The target timestamp.
    /// @return The distance, or zero if either timestamp is invalid.
    [[nodiscard]] auto durationTo(Timestamp other) const noexcept -> Duration;
    /// Return the exact signed distance, truncated toward zero.
    /// @param other The target timestamp.
    /// @return The distance.
    /// @throws err::ParameterError if either timestamp is invalid.
    /// @throws err::OverflowError if the result does not fit.
    [[nodiscard]] auto durationToOrThrow(Timestamp other) const -> Duration;
    /// Test whether the distance would saturate; invalid operands return false.
    /// @param other The target timestamp.
    /// @return Whether the result exceeds its amount range.
    [[nodiscard]] auto wouldDurationToSaturate(Timestamp other) const noexcept -> bool;
    /// Return the signed distance, truncated toward zero and saturated if necessary.
    /// @param other The target timestamp.
    /// @return The distance, or zero if either timestamp is invalid.
    [[nodiscard]] auto timeDeltaTo(Timestamp other) const noexcept -> TimeDelta;
    /// Return the exact signed distance, truncated toward zero.
    /// @param other The target timestamp.
    /// @return The distance.
    /// @throws err::ParameterError if either timestamp is invalid.
    /// @throws err::OverflowError if the result does not fit.
    [[nodiscard]] auto timeDeltaToOrThrow(Timestamp other) const -> TimeDelta;
    /// Test whether the distance would saturate; invalid operands return false.
    /// @param other The target timestamp.
    /// @return Whether the result exceeds its amount range.
    [[nodiscard]] auto wouldTimeDeltaToSaturate(Timestamp other) const noexcept -> bool;

public: // conversion
    /// Return the stored signed fields, including the canonical invalid state.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> RawValue { return {_days, _nanoseconds}; }
    /// Serialize the signed day and nanoseconds as 12 big-endian bytes.
    /// @return The canonical binary representation, excluding object padding.
    [[nodiscard]] auto toByteBlock() const -> mem::ByteBlock;
    /// Convert to a UTC DateTime.
    /// @return The UTC instant, or no value for an invalid timestamp.
    [[nodiscard]] auto toDateTime() const noexcept -> std::optional<DateTime>;
    /// Convert to a UTC DateTime.
    /// @return The UTC instant.
    /// @throws err::OutOfRangeError if this timestamp is invalid.
    [[nodiscard]] auto toDateTimeOrThrow() const -> DateTime;
    /// Format the instant as YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ.
    /// @return The ISO text, or no value for an invalid timestamp.
    [[nodiscard]] auto toIsoString() const -> std::optional<text::String>;
    /// Format the instant with exactly nine fractional digits in UTC.
    /// @return The ISO text.
    /// @throws err::OutOfRangeError if this timestamp is invalid.
    [[nodiscard]] auto toIsoStringOrThrow() const -> text::String;
    /// Return canonical ISO text, or an empty string for an invalid timestamp.
    [[nodiscard]] auto toString() const -> text::String;
    /// Convert to signed ticks relative to a named epoch, truncating toward zero.
    /// @tparam T A seconds, milliseconds, microseconds or nanoseconds amount.
    /// @param epoch The reference epoch.
    /// @return The ticks, or no value if invalid or unrepresentable.
    template <impl::DateTimeTickUnit T>
    [[nodiscard]] auto toTicks(TimeEpoch epoch = TimeEpoch::Core) const noexcept -> std::optional<T>;
    /// Convert to signed ticks relative to a named epoch, truncating toward zero.
    /// @tparam T A supported fixed tick amount.
    /// @param epoch The reference epoch.
    /// @return The ticks.
    /// @throws err::OutOfRangeError if invalid or unrepresentable.
    template <impl::DateTimeTickUnit T>
    [[nodiscard]] auto toTicksOrThrow(TimeEpoch epoch = TimeEpoch::Core) const -> T;
    /// Split epoch-relative time into floor seconds and a nonnegative fraction.
    /// @param epoch The reference epoch.
    /// @return Seconds and nanoseconds, or no value for an invalid timestamp.
    [[nodiscard]] auto toSecondsAndFractions(TimeEpoch epoch = TimeEpoch::Core) const noexcept
        -> std::optional<std::pair<Seconds, Nanoseconds>>;
    /// Split epoch-relative time into floor seconds and a nonnegative fraction.
    /// @param epoch The reference epoch.
    /// @return Seconds and nanoseconds.
    /// @throws err::OutOfRangeError if this timestamp is invalid.
    [[nodiscard]] auto toSecondsAndFractionsOrThrow(TimeEpoch epoch = TimeEpoch::Core) const
        -> std::pair<Seconds, Nanoseconds>;
    /// Convert to floor POSIX seconds, or minus one if invalid or outside the native range.
    [[nodiscard]] auto toTimeT() const noexcept -> std::time_t;

public: // factories
    /// Read the system wall clock directly; repeated or backwards values are possible.
    /// @return The current UTC instant, or invalid if outside the supported calendar range.
    [[nodiscard]] static auto now() noexcept -> Timestamp;
    /// Return a named epoch.
    /// @param epoch The epoch to return.
    /// @return The UTC epoch timestamp.
    [[nodiscard]] static auto epoch(TimeEpoch epoch = TimeEpoch::Core) noexcept -> Timestamp;
    /// Return 0000-01-01 midnight UTC.
    [[nodiscard]] static constexpr auto first() noexcept -> Timestamp { return Timestamp{0, 0, PrivateTag{}}; }
    /// Return 9999-12-31 at the final nanosecond UTC.
    [[nodiscard]] static constexpr auto last() noexcept -> Timestamp {
        return Timestamp{cLastDay, cNanosecondsPerDay - 1, PrivateTag{}};
    }
    /// Create from unbiased fields; negative fields produce canonical invalid state.
    /// @param days The input days.
    /// @param nanoseconds The input nanoseconds.
    /// @return The timestamp, or no value on failure.
    [[nodiscard]] static auto fromRawValue(int32_t days, int64_t nanoseconds) noexcept -> std::optional<Timestamp>;
    /// Create from unbiased fields; negative fields produce canonical invalid state.
    /// @param days The input days.
    /// @param nanoseconds The input nanoseconds.
    /// @return The timestamp.
    /// @throws err::ParameterError if the input cannot be converted.
    [[nodiscard]] static auto fromRawValueOrThrow(int32_t days, int64_t nanoseconds) -> Timestamp;
    /// Create from unbiased day and nanosecond amounts.
    /// @param days The input days.
    /// @param nanoseconds The input nanoseconds.
    /// @return The timestamp, or no value on failure.
    [[nodiscard]] static auto fromDaysAndNanoseconds(Days days, Nanoseconds nanoseconds) noexcept
        -> std::optional<Timestamp>;
    /// Create from unbiased day and nanosecond amounts.
    /// @param days The input days.
    /// @param nanoseconds The input nanoseconds.
    /// @return The timestamp.
    /// @throws err::ParameterError if the input cannot be converted.
    [[nodiscard]] static auto fromDaysAndNanosecondsOrThrow(Days days, Nanoseconds nanoseconds) -> Timestamp;
    /// Decode exactly 12 big-endian bytes.
    /// @param value The input value.
    /// @return The timestamp, or no value on failure.
    [[nodiscard]] static auto fromByteBlock(const mem::ByteBlock &value) noexcept -> std::optional<Timestamp>;
    /// Decode exactly 12 big-endian bytes.
    /// @param value The input value.
    /// @return The timestamp.
    /// @throws err::ParameterError if the input cannot be converted.
    [[nodiscard]] static auto fromByteBlockOrThrow(const mem::ByteBlock &value) -> Timestamp;
    /// Copy the UTC instant and discard display metadata.
    /// @param value The input value.
    /// @return The timestamp, or no value on failure.
    [[nodiscard]] static auto fromDateTime(const DateTime &value) noexcept -> std::optional<Timestamp>;
    /// Copy the UTC instant and discard display metadata.
    /// @param value The input value.
    /// @return The timestamp.
    /// @throws err::OutOfRangeError if the input cannot be converted.
    [[nodiscard]] static auto fromDateTimeOrThrow(const DateTime &value) -> Timestamp;
    /// Parse an ISO instant with seconds and an explicit zone offset.
    /// @param value The input value.
    /// @return The timestamp, or no value on failure.
    [[nodiscard]] static auto fromIsoString(const text::String &value) -> std::optional<Timestamp>;
    /// Parse an ISO instant with seconds and an explicit zone offset.
    /// @param value The input value.
    /// @return The timestamp.
    /// @throws err::ParseError if the input cannot be converted.
    [[nodiscard]] static auto fromIsoStringOrThrow(const text::String &value) -> Timestamp;
    /// Create from a signed raw field pair.
    /// @param value The input fields.
    /// @return The timestamp, or no value on failure.
    [[nodiscard]] static auto fromRawValue(RawValue value) noexcept -> std::optional<Timestamp> {
        return fromRawValue(value.first, value.second);
    }
    /// Create from a signed raw field pair.
    /// @param value The input fields.
    /// @return The timestamp.
    /// @throws err::ParameterError if nonnegative fields exceed their bounds.
    [[nodiscard]] static auto fromRawValueOrThrow(RawValue value) -> Timestamp {
        return fromRawValueOrThrow(value.first, value.second);
    }
    /// Create from signed ticks relative to a named epoch.
    /// @tparam T A supported fixed tick amount.
    /// @param ticks The signed tick count.
    /// @param epoch The reference epoch.
    /// @return The timestamp, or no value outside the calendar range.
    template <impl::DateTimeTickUnit T>
    [[nodiscard]] static auto fromTicks(T ticks, TimeEpoch epoch = TimeEpoch::Core) noexcept
        -> std::optional<Timestamp>;
    /// Create from signed ticks relative to a named epoch.
    /// @tparam T A supported fixed tick amount.
    /// @param ticks The signed tick count.
    /// @param epoch The reference epoch.
    /// @return The timestamp.
    /// @throws err::OutOfRangeError if the result exceeds the calendar range.
    template <impl::DateTimeTickUnit T>
    [[nodiscard]] static auto fromTicksOrThrow(T ticks, TimeEpoch epoch = TimeEpoch::Core) -> Timestamp;
    /// Create from floor seconds and a nonnegative nanosecond fraction.
    /// @param seconds The signed seconds relative to the epoch.
    /// @param fractions The fraction in 0..999999999.
    /// @param epoch The reference epoch.
    /// @return The timestamp, or no value on invalid input or range overflow.
    [[nodiscard]] static auto fromTicks(
        Seconds seconds, Nanoseconds fractions, TimeEpoch epoch = TimeEpoch::Core) noexcept -> std::optional<Timestamp>;
    /// Create from floor seconds and a nonnegative nanosecond fraction.
    /// @param seconds The signed seconds relative to the epoch.
    /// @param fractions The fraction in 0..999999999.
    /// @param epoch The reference epoch.
    /// @return The timestamp.
    /// @throws err::ParameterError if the fraction is invalid.
    /// @throws err::OutOfRangeError if the result exceeds the calendar range.
    [[nodiscard]] static auto fromTicksOrThrow(
        Seconds seconds, Nanoseconds fractions, TimeEpoch epoch = TimeEpoch::Core) -> Timestamp;
    /// Create from signed native POSIX seconds.
    /// @param value The native seconds.
    /// @return The timestamp, or invalid outside the calendar range.
    [[nodiscard]] static auto fromTimeT(std::time_t value) noexcept -> Timestamp;

private: // implementation
    /// Construct canonical fields after validation.
    constexpr Timestamp(int32_t days, int64_t nanoseconds, PrivateTag) noexcept :
        _days{days}, _nanoseconds{nanoseconds} {}
    /// Split a fixed amount into bounded day and sub-day components.
    /// @tparam T A fixed-unit amount, Duration or TimeDelta.
    template <impl::TimestampAmount T>
    [[nodiscard]] static auto splitAmount(T amount) noexcept -> std::pair<int64_t, int64_t>;
    /// Apply decomposed arithmetic and report calendar saturation.
    [[nodiscard]] auto shifted(int64_t days, int64_t nanoseconds, bool subtract, bool &overflow) const noexcept
        -> Timestamp;
    /// Calculate and bound a signed distance at the requested resolution.
    [[nodiscard]] auto distanceTicks(Timestamp other, int64_t nanosecondsPerTick, bool &overflow) const noexcept
        -> int64_t;
    /// Validate operands and calculate an exact bounded distance.
    [[nodiscard]] auto checkedDistanceTicks(Timestamp other, int64_t nanosecondsPerTick) const -> int64_t;
    /// Return the constant unbiased day count of a named epoch.
    [[nodiscard]] static auto epochDays(TimeEpoch epoch) noexcept -> int32_t;

private: // constants
    static constexpr int32_t cLastDay = 3'652'424;
    static constexpr int64_t cNanosecondsPerSecond = 1'000'000'000;
    static constexpr int64_t cSecondsPerDay = 86'400;
    static constexpr int64_t cNanosecondsPerDay = cSecondsPerDay * cNanosecondsPerSecond;

private:                     // storage
    int32_t _days{-1};       ///< Days since the Core epoch; minus one marks invalid.
    int64_t _nanoseconds{0}; ///< Nanoseconds since midnight; zero for invalid timestamps.
};

}

#include "Timestamp.tpp"
