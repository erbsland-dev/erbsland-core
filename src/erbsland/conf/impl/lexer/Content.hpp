// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock.hpp"
#include "../../../text/String.hpp"
#include "../../../time/CalendarDelta.hpp"
#include "../../../time/Date.hpp"
#include "../../../time/DateTime.hpp"
#include "../../../time/Time.hpp"
#include "../../../time/TimeWithZone.hpp"
#include "../../Float.hpp"
#include "../../Integer.hpp"

#include <variant>

namespace erbsland::conf::impl {

/// A placeholder type to signal that the token has no value.
struct NoContent {};

/// A variant used to store the contents of a value.
using Content = std::variant<
    NoContent,
    Integer,
    bool,
    Float,
    text::String,
    time::Date,
    time::Time,
    time::TimeWithZone,
    time::DateTime,
    mem::ByteBlock,
    time::CalendarDelta>;

}
