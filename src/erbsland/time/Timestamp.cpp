// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Timestamp.hpp"

#include "impl/IsoDateTimeParser.hpp"

#include "../err/OutOfRangeError.hpp"
#include "../err/ParseError.hpp"
#include "../math/SaturatingMath.hpp"
#include "../mem/ByteWriter.hpp"
#include "../text/Literals.hpp"
#include "../unit/ByteCount.hpp"
#include "../unit/ByteIndex.hpp"

#include <chrono>
#include <limits>

namespace erbsland::time {

using namespace err;
using namespace mem;
using namespace text;
using namespace text::literals;

Timestamp::Timestamp(Date date, Time time) noexcept :
    _days{static_cast<int32_t>(date.toDaysSinceEpoch().toRawValue())},
    _nanoseconds{date.isValid() ? time.toNanosecondsSinceMidnight().toRawValue() : 0} {
}

auto Timestamp::date() const noexcept -> Date {
    return Date::fromDaysSinceEpoch(dateAsDays());
}

auto Timestamp::time() const noexcept -> Time {
    return Time::fromDurationSinceMidnight(TimeDelta{timeAsNanoseconds()});
}

auto Timestamp::toByteBlock() const -> ByteBlock {
    auto writer = ByteWriter{};
    writer.setEndianness(Endianness::Big);
    writer.writeInt32(_days).writeInt64(_nanoseconds);
    return writer.toByteBlock();
}

auto Timestamp::toDateTime() const noexcept -> std::optional<DateTime> {
    if (!isValid()) {
        return std::nullopt;
    }
    return DateTime{date(), time()};
}

auto Timestamp::toDateTimeOrThrow() const -> DateTime {
    const auto result = toDateTime();
    if (!result) {
        throw OutOfRangeError{"An invalid timestamp has no DateTime representation"_el};
    }
    return *result;
}

auto Timestamp::toIsoString() const -> std::optional<String> {
    const auto value = toDateTime();
    if (!value) {
        return std::nullopt;
    }
    return value->toIsoString(
        IsoTimeFormatFlags{
            IsoTimeFormat::Extended,
            IsoTimeFormat::TimePrefix,
            IsoTimeFormat::TimeShift,
            IsoTimeFormat::UseDotFraction},
        DateTimePrecision::Nanosecond);
}

auto Timestamp::toIsoStringOrThrow() const -> String {
    const auto result = toIsoString();
    if (!result) {
        throw OutOfRangeError{"An invalid timestamp has no ISO representation"_el};
    }
    return *result;
}

auto Timestamp::toString() const -> String {
    return toIsoString().value_or(String{});
}

auto Timestamp::toSecondsAndFractions(TimeEpoch epochValue) const noexcept
    -> std::optional<std::pair<Seconds, Nanoseconds>> {
    if (!isValid()) {
        return std::nullopt;
    }
    const auto seconds =
        (static_cast<int64_t>(_days) - epochDays(epochValue)) * cSecondsPerDay + _nanoseconds / cNanosecondsPerSecond;
    return std::pair{Seconds{seconds}, Nanoseconds{_nanoseconds % cNanosecondsPerSecond}};
}

auto Timestamp::toSecondsAndFractionsOrThrow(TimeEpoch epochValue) const -> std::pair<Seconds, Nanoseconds> {
    const auto result = toSecondsAndFractions(epochValue);
    if (!result) {
        throw OutOfRangeError{"An invalid timestamp has no epoch representation"_el};
    }
    return *result;
}

auto Timestamp::toTimeT() const noexcept -> std::time_t {
    const auto parts = toSecondsAndFractions(TimeEpoch::Posix);
    if (!parts || math::willCastOverflow<std::time_t>(parts->first.toRawValue())) {
        return std::time_t{-1};
    }
    return static_cast<std::time_t>(parts->first.toRawValue());
}

auto Timestamp::now() noexcept -> Timestamp {
    const auto sinceEpoch = std::chrono::system_clock::now().time_since_epoch();
    const auto seconds = std::chrono::floor<std::chrono::seconds>(sinceEpoch);
    const auto fraction = std::chrono::duration_cast<std::chrono::nanoseconds>(sinceEpoch - seconds).count();
    return fromTicks(Seconds{seconds.count()}, Nanoseconds{fraction}, TimeEpoch::Posix).value_or(Timestamp{});
}

auto Timestamp::epoch(TimeEpoch epochValue) noexcept -> Timestamp {
    return Timestamp{epochDays(epochValue), 0, PrivateTag{}};
}

auto Timestamp::epochDays(TimeEpoch epochValue) noexcept -> int32_t {
    switch (epochValue) {
    case TimeEpoch::Core:
        return 0;
    case TimeEpoch::Posix:
        return 719'528;
    case TimeEpoch::Windows:
        return 584'754;
    case TimeEpoch::Rfc868:
        return 693'961;
    }
    return 0;
}

auto Timestamp::fromRawValue(int32_t days, int64_t nanoseconds) noexcept -> std::optional<Timestamp> {
    if (days < 0 || nanoseconds < 0) {
        return Timestamp{};
    }
    if (days > cLastDay || nanoseconds >= cNanosecondsPerDay) {
        return std::nullopt;
    }
    return Timestamp{days, nanoseconds, PrivateTag{}};
}

auto Timestamp::fromRawValueOrThrow(int32_t days, int64_t nanoseconds) -> Timestamp {
    const auto result = fromRawValue(days, nanoseconds);
    if (!result) {
        throw OutOfRangeError{"Timestamp fields exceed the calendar or time-of-day bounds"_el};
    }
    return *result;
}

auto Timestamp::fromDaysAndNanoseconds(Days days, Nanoseconds nanoseconds) noexcept -> std::optional<Timestamp> {
    if (days.isNegative() || nanoseconds.isNegative()) {
        return Timestamp{};
    }
    if (days.toRawValue() > cLastDay) {
        return std::nullopt;
    }
    return fromRawValue(static_cast<int32_t>(days.toRawValue()), nanoseconds.toRawValue());
}

auto Timestamp::fromDaysAndNanosecondsOrThrow(Days days, Nanoseconds nanoseconds) -> Timestamp {
    const auto result = fromDaysAndNanoseconds(days, nanoseconds);
    if (!result) {
        throw OutOfRangeError{"Timestamp fields exceed their bounds"_el};
    }
    return *result;
}

auto Timestamp::fromByteBlock(const ByteBlock &value) noexcept -> std::optional<Timestamp> {
    if (value.length() != unit::ByteCount{12U}) {
        return std::nullopt;
    }
    return fromRawValue(
        value.getInteger<int32_t>(unit::ByteIndex{0U}, Endianness::Big),
        value.getInteger<int64_t>(unit::ByteIndex{4U}, Endianness::Big));
}

auto Timestamp::fromByteBlockOrThrow(const ByteBlock &value) -> Timestamp {
    const auto result = fromByteBlock(value);
    if (!result) {
        throw ParseError{"A timestamp requires 12 bytes containing canonical bounded fields"_el};
    }
    return *result;
}

auto Timestamp::fromDateTime(const DateTime &value) noexcept -> std::optional<Timestamp> {
    if (!value.isValid()) {
        return std::nullopt;
    }
    return Timestamp{
        static_cast<int32_t>(value.utcDate().toDaysSinceEpoch().toRawValue()),
        value.utcTime().toNanosecondsSinceMidnight().toRawValue(),
        PrivateTag{}};
}

auto Timestamp::fromDateTimeOrThrow(const DateTime &value) -> Timestamp {
    const auto result = fromDateTime(value);
    if (!result) {
        throw OutOfRangeError{"An invalid DateTime has no timestamp representation"_el};
    }
    return *result;
}

auto Timestamp::fromIsoString(const String &value) -> std::optional<Timestamp> {
    const auto parsed = impl::IsoDateTimeParser::parse(value, true);
    if (!parsed || !parsed->hasOffset || parsed->precision < DateTimePrecision::Second) {
        return std::nullopt;
    }
    const auto seconds = parsed->date.toDaysSinceEpoch().toRawValue() * cSecondsPerDay +
        parsed->time.toSecondsSinceMidnight().toRawValue() - parsed->offset.toRawValue();
    return fromTicks(Seconds{seconds}, parsed->time.nanosecondFraction());
}

auto Timestamp::fromIsoStringOrThrow(const String &value) -> Timestamp {
    const auto result = fromIsoString(value);
    if (!result) {
        throw ParseError{"Invalid ISO timestamp; seconds and an explicit UTC offset are required"_el};
    }
    return *result;
}

auto Timestamp::fromTicks(Seconds seconds, Nanoseconds fractions, TimeEpoch epochValue) noexcept
    -> std::optional<Timestamp> {
    const auto fraction = fractions.toRawValue();
    if (fraction < 0 || fraction >= cNanosecondsPerSecond) {
        return std::nullopt;
    }
    const auto [days, nanoseconds] = splitAmount(seconds);
    auto overflow = false;
    const auto result = epoch(epochValue).shifted(days, nanoseconds + fraction, false, overflow);
    if (overflow) {
        return std::nullopt;
    }
    return result;
}

auto Timestamp::fromTicksOrThrow(Seconds seconds, Nanoseconds fractions, TimeEpoch epochValue) -> Timestamp {
    if (fractions.isNegative() || fractions.toRawValue() >= cNanosecondsPerSecond) {
        throw OutOfRangeError{"Nanosecond fractions must be within one second"_el};
    }
    const auto result = fromTicks(seconds, fractions, epochValue);
    if (!result) {
        throw OutOfRangeError{"Epoch ticks exceed the timestamp calendar range"_el};
    }
    return *result;
}

auto Timestamp::fromTimeT(std::time_t value) noexcept -> Timestamp {
    if (math::willCastOverflow<int64_t>(value)) {
        return {};
    }
    return fromTicks(Seconds{static_cast<int64_t>(value)}, TimeEpoch::Posix).value_or(Timestamp{});
}

}
