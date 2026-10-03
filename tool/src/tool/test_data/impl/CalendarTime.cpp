// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CalendarTime.hpp"

#include <erbsland/StringFormat.hpp>

namespace tool::test_data::impl {

using namespace el::text::literals;

auto CalendarTime::nanoseconds() const -> int64_t {
    return ((hour * 60LL + minute) * 60 + second) * 1000000000 + nanosecond;
}

auto CalendarTime::fields() const -> el::String {
    return el::StringFormat{"{} {} {} {}"_el}.build(hour, minute, second, nanosecond);
}

auto CalendarTime::fromNanoseconds(const int64_t value) -> CalendarTime {
    return {
        static_cast<int>(value / 3600000000000),
        static_cast<int>(value / 60000000000 % 60),
        static_cast<int>(value / 1000000000 % 60),
        static_cast<int>(value % 1000000000)};
}

auto CalendarTime::random(el::FastRandom &random) -> CalendarTime {
    return {random.getInt32(0, 23), random.getInt32(0, 59), random.getInt32(0, 59), random.getInt32(0, 999999999)};
}

}
