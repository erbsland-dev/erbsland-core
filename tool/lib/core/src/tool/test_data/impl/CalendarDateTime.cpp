// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CalendarDateTime.hpp"

#include <erbsland/StringJoins.hpp>

namespace tool::test_data::impl {

using namespace el::text::literals;

auto CalendarDateTime::seconds() const -> int64_t {
    return date.days() * 86400 + time.nanoseconds() / 1000000000;
}

auto CalendarDateTime::fields() const -> el::String {
    return el::join(date.fields(), " "_el, time.fields());
}

auto CalendarDateTime::addedSeconds(const el::BigInteger &amount) const -> CalendarDateTime {
    const auto target = el::BigInteger{seconds()} + amount;
    if (target < el::BigInteger{0}) {
        return {};
    }
    if (target > el::BigInteger{315569519999LL}) {
        return {{9999, 12, 31}, {23, 59, 59, 999999999}};
    }
    const auto value = target.castOrThrow<int64_t>();
    return {
        CalendarDate::fromDays(value / 86400),
        CalendarTime::fromNanoseconds((value % 86400) * 1000000000 + time.nanosecond)};
}

}
