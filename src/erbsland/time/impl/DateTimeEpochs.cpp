// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DateTimeEpochs.hpp"

#include "../Date.hpp"

namespace erbsland::time::impl {

auto secondsSinceCoreEpoch(const TimeEpoch epoch) noexcept -> Seconds {
    const auto secondsFromYear = [](const auto year) -> Seconds {
        return Date::fromYearMonthDay(year, 1, 1).toDaysSinceEpoch().template converted<Seconds>();
    };
    switch (epoch) {
    case TimeEpoch::Core:
        return {};
    case TimeEpoch::Posix: {
        static auto posixEpoch = secondsFromYear(1970);
        return posixEpoch;
    }
    case TimeEpoch::Windows: {
        static auto windowsEpoch = secondsFromYear(1601);
        return windowsEpoch;
    }
    case TimeEpoch::Rfc868: {
        static auto rfc868Epoch = secondsFromYear(1900);
        return rfc868Epoch;
    }
    }
    return {};
}

}
