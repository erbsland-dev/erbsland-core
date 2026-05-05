// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeZone.hpp"

#include "tz/impl/Database.hpp"

#include "../err/ThrowHelper.hpp"
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

auto TimeZone::normalizeOffset(Seconds seconds) noexcept -> Seconds {
    auto secondsValue = seconds % 86400;
    if (secondsValue > Seconds{43200}) {
        secondsValue -= Seconds{86400};
    } else if (secondsValue < Seconds{-43199}) {
        secondsValue += Seconds{86400};
    }
    return Seconds{secondsValue};
}

auto TimeZone::database() noexcept -> const tz::impl::Database & {
    static const auto instance = tz::impl::Database{};
    return instance;
}

auto TimeZone::parseFixedOffsetText(const text::StringView &text, Seconds &offset) noexcept -> bool {
    auto reader = text::StringCharReader{text};
    const auto readDecimal = [&reader](const unit::CpLength minimum, const unit::CpLength maximum, int &value) -> bool {
        auto options = text::IntegerParseOptions::parserDefault();
        options.setFixedBase(text::IntegerBase::Decimal)
            .setMinimumDigits(minimum)
            .setMaximumDigits(maximum)
            .addFlags(text::IntegerParseFlag::StopAtMaximum);
        const auto result = reader.parseInteger(options);
        if (result.status != text::ReadNumberStatus::Success) {
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
    if (!readDecimal(unit::CpLength{1U}, unit::CpLength{2U}, hour)) {
        return false;
    }
    const auto extended = reader.advanceIf(U':');
    if (extended) {
        if (!readDecimal(unit::CpLength{2U}, unit::CpLength{2U}, minute)) {
            return false;
        }
        if (reader.advanceIf(U':')) {
            if (!readDecimal(unit::CpLength{2U}, unit::CpLength{2U}, second)) {
                return false;
            }
        }
    } else if (reader.peek().isAsciiDigit()) {
        if (!readDecimal(unit::CpLength{2U}, unit::CpLength{2U}, minute)) {
            return false;
        }
    }
    if (!reader.isAtEnd() || hour > 23 || minute > 59 || second > 59) {
        return false;
    }
    offset = Seconds{sign * (hour * 3600 + minute * 60 + second)};
    return true;
}

auto TimeZone::specialTimeZoneFromName(text::StringView name) noexcept -> std::optional<TimeZone> {
    const auto isAsciiFoldedName = [&name](const auto &expected) -> bool {
        return name.compare(expected, text::Char::compareAsciiFolded) == std::strong_ordering::equal;
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
        if (!name.startsWith(prefix, text::Char::compareAsciiFolded)) {
            return std::nullopt;
        }
        const auto suffix = name.slice(text::StringSide::Back, name.length() - prefix.length());
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

TimeZone::TimeZone(Hours hours, Minutes minutes, Seconds seconds) noexcept :
    TimeZone{Duration{hours.converted<Seconds>() + minutes.converted<Seconds>() + seconds}} {
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

auto TimeZone::name() const -> text::String {
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

auto TimeZone::isValidName(const text::StringView &name) noexcept -> bool {
    return fromName(name).has_value();
}

auto TimeZone::fromName(const text::StringView &name) noexcept -> std::optional<TimeZone> {
    const auto id = database().zoneIdFromName(name);
    if (id != tz::impl::cZoneIdNotFound) {
        return TimeZone{TimeZoneId{id}};
    }
    return specialTimeZoneFromName(name);
}

auto TimeZone::fromNameOrThrow(const text::StringView &name) -> TimeZone {
    if (auto result = fromName(name); result.has_value()) {
        return result.value();
    }
    err::throwParseError("Unknown time zone name");
}

auto TimeZone::names() -> text::StringList {
    return database().names();
}

auto TimeZone::databaseVersion() noexcept -> unit::Version {
    return tz::impl::Database::version();
}

auto TimeZone::abbreviation(const tz::TimeOffset offset) -> text::String {
    if (!offset.isZone()) {
        return {};
    }
    return database().abbreviation(offset.zoneId().toRawValue(), offset.abbreviationId());
}

auto TimeZone::timeOffsetAtLocal(const Date date, const Time time, const TimeOccurrenceInFold occurrence) const noexcept
    -> tz::TimeOffset {
    if (const auto *fixed = std::get_if<FixedOffset>(&_storage)) {
        return tz::TimeOffset{fixed->offset};
    }
    if (const auto *named = std::get_if<NamedZone>(&_storage)) {
        if (const auto zoneInfo = database().info(named->id.toRawValue())) {
            const auto seconds = date.toDaysSinceEpoch().converted<Seconds>() + time.toSecondsSinceMidnight();
            const auto details = zoneInfo->detailsForLocal(seconds, occurrence);
            return tz::TimeOffset{details.total, !details.dst.isZero(), named->id, details.abbreviation};
        }
    }
    return {};
}

auto TimeZone::timeOffsetAtUtc(Date date, Time time) const noexcept -> tz::TimeOffset {
    if (const auto *fixed = std::get_if<FixedOffset>(&_storage)) {
        return tz::TimeOffset{fixed->offset};
    }
    if (const auto *named = std::get_if<NamedZone>(&_storage)) {
        if (const auto zoneInfo = database().info(named->id.toRawValue())) {
            const auto seconds = date.toDaysSinceEpoch().converted<Seconds>() + time.toSecondsSinceMidnight();
            const auto details = zoneInfo->details(seconds, tz::impl::TimeReference::Utc);
            return tz::TimeOffset{details.total, !details.dst.isZero(), named->id, details.abbreviation};
        }
    }
    return {};
}

}
