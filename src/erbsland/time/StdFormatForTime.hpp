// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CalendarDelta.hpp"
#include "Date.hpp"
#include "DateTime.hpp"
#include "Time.hpp"
#include "TimeDelta.hpp"
#include "TimeWithZone.hpp"

#include "../text/StdFormatForText.hpp"

#include <format>

template <>
struct std::formatter<erbsland::time::CalendarDelta> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::time::CalendarDelta value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::time::Date> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::time::Date value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::time::DateTime> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::time::DateTime value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::time::Time> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::time::Time value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::time::TimeDelta> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::time::TimeDelta value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::time::TimeWithZone> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::time::TimeWithZone value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};
