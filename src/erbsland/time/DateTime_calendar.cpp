// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DateTime.hpp"

#include "../err/OverflowError.hpp"

namespace erbsland::time {

auto DateTime::wouldAddSaturate(const CalendarDelta &delta) const noexcept -> bool {
    if (!isValid()) {
        return false;
    }
    try {
        static_cast<void>(addedOrThrow(delta));
        return false;
    } catch (const err::OverflowError &) {
        return true;
    }
}

auto DateTime::wouldSubtractSaturate(const CalendarDelta &delta) const noexcept -> bool {
    return wouldAddSaturate(-delta);
}

auto DateTime::added(const CalendarDelta &delta) const noexcept -> DateTime {
    if (!isValid()) {
        return {};
    }
    auto date = _date;
    auto time = _time;
    const auto saturate = [&date, &time](const bool positive) -> void {
        date = positive ? Date::last() : Date::first();
        time = positive ? Time::last() : Time::first();
    };
    const auto applyTimeDelta = [&date, &time, &saturate](const TimeDelta amount) -> void {
        const auto days = time.addWithWrap(amount);
        if (date.wouldAddSaturate(days)) {
            saturate(days.isPositive());
        } else {
            date.add(days);
        }
    };
    const auto applySeconds = [&date, &time, &saturate](const Seconds amount) -> void {
        const auto days = time.addWithWrap(Duration{amount});
        if (date.wouldAddSaturate(days)) {
            saturate(days.isPositive());
        } else {
            date.add(days);
        }
    };
    const auto applySubseconds =
        [&applySeconds, &applyTimeDelta](
            const int64_t value, const int64_t partsPerSecond, const int64_t nsPerPart) -> void {
        applySeconds(Seconds{value / partsPerSecond});
        applyTimeDelta(TimeDelta{Nanoseconds{(value % partsPerSecond) * nsPerPart}});
    };
    const auto applySecondsAmount = [&applySeconds, &saturate](const auto amount) -> void {
        if (amount.template wouldConvertSaturate<Seconds>()) {
            saturate(amount.isPositive());
        } else {
            applySeconds(amount.template converted<Seconds>());
        }
    };
    const auto applyDateAmount = [&date, &saturate](const auto amount) -> void {
        if (date.wouldAddSaturate(amount)) {
            saturate(amount.isPositive());
        } else {
            date.add(amount);
        }
    };

    applySubseconds(delta.nanoseconds().toRawValue(), 1000000000LL, 1LL);
    applySubseconds(delta.microseconds().toRawValue(), 1000000LL, 1000LL);
    applySubseconds(delta.milliseconds().toRawValue(), 1000LL, 1000000LL);
    applySeconds(delta.seconds());
    applySecondsAmount(delta.minutes());
    applySecondsAmount(delta.hours());
    applyDateAmount(delta.days());
    if (delta.weeks().wouldConvertSaturate<Days>()) {
        saturate(delta.weeks().isPositive());
    } else {
        applyDateAmount(delta.weeks().converted<Days>());
    }
    applyDateAmount(delta.months());
    applyDateAmount(delta.years());

    auto result = DateTime{date, time};
    if (_offset.isZone()) {
        return result.toTimeZone(timeZone());
    }
    result._offset = _offset;
    return result;
}

auto DateTime::addedOrThrow(const CalendarDelta &delta) const -> DateTime {
    if (!isValid()) {
        return {};
    }
    auto date = _date;
    auto time = _time;
    const auto applyTimeDelta = [&date, &time](const TimeDelta amount) -> void {
        const auto days = time.addWithWrap(amount);
        date.addOrThrow(days);
    };
    const auto applySeconds = [&date, &time](const Seconds amount) -> void {
        const auto days = time.addWithWrap(Duration{amount});
        date.addOrThrow(days);
    };
    const auto applySubseconds =
        [&applySeconds, &applyTimeDelta](
            const int64_t value, const int64_t partsPerSecond, const int64_t nsPerPart) -> void {
        applySeconds(Seconds{value / partsPerSecond});
        applyTimeDelta(TimeDelta{Nanoseconds{(value % partsPerSecond) * nsPerPart}});
    };
    const auto applySecondsAmount = [&applySeconds](const auto amount) -> void {
        if (amount.template wouldConvertSaturate<Seconds>()) {
            throw err::OverflowError{"CalendarDelta application would exceed DateTime bounds"};
        }
        applySeconds(amount.template converted<Seconds>());
    };

    applySubseconds(delta.nanoseconds().toRawValue(), 1000000000LL, 1LL);
    applySubseconds(delta.microseconds().toRawValue(), 1000000LL, 1000LL);
    applySubseconds(delta.milliseconds().toRawValue(), 1000LL, 1000000LL);
    applySeconds(delta.seconds());
    applySecondsAmount(delta.minutes());
    applySecondsAmount(delta.hours());
    date.addOrThrow(delta.days());
    if (delta.weeks().wouldConvertSaturate<Days>()) {
        throw err::OverflowError{"CalendarDelta application would exceed DateTime bounds"};
    }
    date.addOrThrow(delta.weeks().converted<Days>());
    date.addOrThrow(delta.months());
    date.addOrThrow(delta.years());

    auto result = DateTime{date, time};
    if (_offset.isZone()) {
        return result.toTimeZone(timeZone());
    }
    result._offset = _offset;
    return result;
}

}
