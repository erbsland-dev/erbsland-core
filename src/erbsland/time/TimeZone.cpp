// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeZone.hpp"

#include "tz/impl/Database.hpp"
#include "tz/impl/LocalTimeZoneBackend.hpp"

#include "../err/ParseError.hpp"
#include "../text/Char.hpp"
#include "../text/IntegerBase.hpp"
#include "../text/IntegerParseFlag.hpp"
#include "../text/IntegerParseOptions.hpp"
#include "../text/Literals.hpp"
#include "../text/ReadNumberStatus.hpp"
#include "../text/StringCharReader.hpp"
#include "../text/StringSide.hpp"

#include <compare>

namespace erbsland::time {

using namespace text::literals;
using namespace text;
using unit::CpLength;
using unit::Version;

auto TimeZone::normalizeOffset(const Seconds seconds) noexcept -> Seconds {
    return seconds % 86400;
}

auto TimeZone::database() noexcept -> const tz::impl::Database & {
    static const auto instance = tz::impl::Database{};
    return instance;
}

auto TimeZone::parseFixedOffsetText(const String &text, Seconds &offset) noexcept -> bool {
    auto reader = StringCharReader{text};
    const auto readDecimal = [&reader](const CpLength minimum, const CpLength maximum, int &value) -> bool {
        auto options = IntegerParseOptions::parserDefault();
        options.setFixedBase(IntegerBase::Decimal)
            .setMinimumDigits(minimum)
            .setMaximumDigits(maximum)
            .addFlags(IntegerParseFlag::StopAtMaximum);
        const auto result = reader.parseInteger(options);
        if (result.status != ReadNumberStatus::Success) {
            return false;
        }
        value = static_cast<int>(result.value);
        return true;
    };

    auto sign = 1;
    if (reader.advanceIf(U'-')) {
        sign = -1;
    } else if (!reader.advanceIf(U'+')) {
        return false;
    }

    auto hour = 0;
    auto minute = 0;
    auto second = 0;
    if (!readDecimal(CpLength{1U}, CpLength{2U}, hour)) {
        return false;
    }
    const auto extended = reader.advanceIf(U':');
    if (extended) {
        if (!readDecimal(CpLength{2U}, CpLength{2U}, minute)) {
            return false;
        }
        if (reader.advanceIf(U':')) {
            if (!readDecimal(CpLength{2U}, CpLength{2U}, second)) {
                return false;
            }
        }
    } else if (reader.peek().isAsciiDigit()) {
        if (!readDecimal(CpLength{2U}, CpLength{2U}, minute)) {
            return false;
        }
    }
    if (!reader.isAtEnd() || hour > 23 || minute > 59 || second > 59) {
        return false;
    }
    offset = Seconds{sign * (hour * 3600 + minute * 60 + second)};
    return true;
}

auto TimeZone::specialTimeZoneFromName(const String &name) noexcept -> std::optional<TimeZone> {
    const auto isAsciiFoldedName = [&name](const auto &expected) -> bool {
        return name.compare(expected, Char::compareAsciiFolded) == std::strong_ordering::equal;
    };
    if (isAsciiFoldedName("Z"_el) || isAsciiFoldedName("Universal"_el) || isAsciiFoldedName("Zulu"_el) ||
        isAsciiFoldedName("Factory"_el)) {
        return TimeZone{};
    }

    auto offset = Seconds{};
    if (parseFixedOffsetText(name, offset)) {
        return TimeZone{Duration{offset}};
    }

    const auto prefixedOffset = [&name, &offset](const auto &prefix) -> std::optional<TimeZone> {
        if (!name.startsWith(prefix, Char::compareAsciiFolded)) {
            return std::nullopt;
        }
        const auto suffix = name.slice(StringSide::Back, name.length() - prefix.length());
        if (suffix.isEmpty()) {
            return TimeZone{};
        }
        if (parseFixedOffsetText(suffix, offset)) {
            return TimeZone{Duration{offset}};
        }
        return std::nullopt;
    };
    if (auto result = prefixedOffset("UTC"_el); result.has_value()) {
        return result;
    }
    if (auto result = prefixedOffset("GMT"_el); result.has_value()) {
        return result;
    }
    return std::nullopt;
}

TimeZone::TimeZone(const Hours hours, const Minutes minutes, const Seconds seconds) noexcept :
    TimeZone{Duration{
        hours.clamped(Hours{-23}, Hours{23}).converted<Seconds>() +
        minutes.clamped(Minutes{-59}, Minutes{59}).converted<Seconds>() + seconds.clamped(Seconds{-59}, Seconds{59})}} {
}

TimeZone::TimeZone(Duration offset) noexcept {
    const auto normalized = normalizeOffset(offset.toSeconds());
    if (!normalized.isZero()) {
        _storage = FixedOffset{normalized};
    }
}

TimeZone::TimeZone(TimeZoneId id) noexcept {
    if (!id.isUtc() && id.toRawValue() != tz::impl::cZoneIdNotFound) {
        _storage = NamedZone{id};
    }
}

auto TimeZone::isUtc() const noexcept -> bool {
    return std::holds_alternative<std::monostate>(_storage);
}

auto TimeZone::isStaticOffset() const noexcept -> bool {
    return std::holds_alternative<FixedOffset>(_storage);
}

auto TimeZone::isNamed() const noexcept -> bool {
    return std::holds_alternative<NamedZone>(_storage);
}

auto TimeZone::staticOffset() const noexcept -> Duration {
    if (const auto *offset = std::get_if<FixedOffset>(&_storage)) {
        return Duration{offset->offset};
    }
    return {};
}

auto TimeZone::name() const -> String {
    if (const auto *named = std::get_if<NamedZone>(&_storage)) {
        return database().nameFromZoneId(named->id.toRawValue());
    }
    return {};
}

auto TimeZone::id() const noexcept -> TimeZoneId {
    if (const auto *named = std::get_if<NamedZone>(&_storage)) {
        return named->id;
    }
    return {};
}

auto TimeZone::isValidName(const String &name) noexcept -> bool {
    return fromName(name).has_value();
}

auto TimeZone::fromName(const String &name) noexcept -> std::optional<TimeZone> {
    const auto id = database().zoneIdFromName(name);
    if (id != tz::impl::cZoneIdNotFound) {
        return TimeZone{TimeZoneId{id}};
    }
    return specialTimeZoneFromName(name);
}

auto TimeZone::fromNameOrThrow(const String &name) -> TimeZone {
    if (auto result = fromName(name); result.has_value()) {
        return result.value();
    }
    throw err::ParseError{"Unknown time zone name"};
}

auto TimeZone::local() noexcept -> TimeZone {
    static const auto instance = []() noexcept -> TimeZone {
        try {
            if (auto backend = tz::impl::createLocalTimeZoneBackend(); backend != nullptr) {
                return backend->localTimeZone();
            }
        } catch (...) {}
        auto fallback = TimeZone{};
        fallback.markAsLocalTime();
        return fallback;
    }();
    return instance;
}

auto TimeZone::names() -> StringList {
    return database().names();
}

auto TimeZone::databaseVersion() noexcept -> Version {
    return tz::impl::Database::version();
}

auto TimeZone::abbreviation(const tz::TimeOffset offset) -> String {
    if (!offset.isZone()) {
        return {};
    }
    return database().abbreviation(offset.zoneId().toRawValue(), offset.abbreviationId());
}

auto TimeZone::timeOffsetAtLocal(const Date date, const Time time, const TimeOccurrenceInFold occurrence) const noexcept
    -> tz::TimeOffset {
    if (const auto *fixed = std::get_if<FixedOffset>(&_storage)) {
        return tz::TimeOffset{fixed->offset, _isLocalTime};
    }
    if (const auto *named = std::get_if<NamedZone>(&_storage)) {
        if (const auto zoneInfo = database().info(named->id.toRawValue())) {
            const auto seconds = date.toDaysSinceEpoch().converted<Seconds>() + time.toSecondsSinceMidnight();
            const auto details = zoneInfo->detailsForLocal(seconds, occurrence);
            return tz::TimeOffset{details.total, !details.dst.isZero(), named->id, details.abbreviation, _isLocalTime};
        }
    }
    return tz::TimeOffset{Seconds{}, _isLocalTime};
}

auto TimeZone::timeOffsetAtUtc(Date date, Time time) const noexcept -> tz::TimeOffset {
    if (const auto *fixed = std::get_if<FixedOffset>(&_storage)) {
        return tz::TimeOffset{fixed->offset, _isLocalTime};
    }
    if (const auto *named = std::get_if<NamedZone>(&_storage)) {
        if (const auto zoneInfo = database().info(named->id.toRawValue())) {
            const auto seconds = date.toDaysSinceEpoch().converted<Seconds>() + time.toSecondsSinceMidnight();
            const auto details = zoneInfo->details(seconds, tz::impl::TimeReference::Utc);
            return tz::TimeOffset{details.total, !details.dst.isZero(), named->id, details.abbreviation, _isLocalTime};
        }
    }
    return tz::TimeOffset{Seconds{}, _isLocalTime};
}

}
