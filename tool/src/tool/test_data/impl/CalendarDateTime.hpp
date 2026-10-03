// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CalendarDate.hpp"
#include "CalendarTime.hpp"

#include <erbsland/BigInteger.hpp>

namespace tool::test_data::impl {

/// Independent combined calendar model with exact duration arithmetic.
struct CalendarDateTime {
    CalendarDate date; ///< Civil date.
    CalendarTime time; ///< Wall-clock time.
    auto operator<=>(const CalendarDateTime &) const = default;
    /// Get seconds since year zero.
    [[nodiscard]] auto seconds() const -> int64_t;
    /// Format the seven calendar fields.
    [[nodiscard]] auto fields() const -> el::String;
    /// Add seconds with saturation at the supported endpoints.
    /// @param amount Exact signed duration, including negated INT64_MIN.
    [[nodiscard]] auto addedSeconds(const el::BigInteger &amount) const -> CalendarDateTime;
};

}
