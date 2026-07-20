// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeDeltaFormat.hpp"

#include "../text/Literals.hpp"

#include <algorithm>

namespace erbsland::time {

using namespace text::literals;

TimeDeltaFormat::TimeDeltaFormat() : _valueSeparator{" "_el}, _unitSeparator{" "_el} {
}

auto TimeDeltaFormat::setMaximumFractionDigits(const uint8_t value) noexcept -> TimeDeltaFormat & {
    _maximumFractionDigits = std::min<uint8_t>(value, 9U);
    return *this;
}

auto TimeDeltaFormat::shortUnits() -> TimeDeltaFormat {
    return {};
}

auto TimeDeltaFormat::longUnits() -> TimeDeltaFormat {
    return TimeDeltaFormat{}.setUnitStyle(UnitStyle::Long);
}

auto TimeDeltaFormat::elcl() -> TimeDeltaFormat {
    auto result = TimeDeltaFormat{};
    result.setValueSeparator({}).setUnitSeparator(", "_el);
    result._usesElclUnitNames = true;
    return result;
}

}
