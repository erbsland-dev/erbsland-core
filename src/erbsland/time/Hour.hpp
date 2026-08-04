// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Hour_fwd.hpp"

#include "impl/TimePartBases.hpp"

namespace erbsland::time {

/// An hour within a day, range `0..23`.
///
/// Provides clamped arithmetic.
/// @tested{TimePartsTest}
class Hour final : public impl::HourBase {
    using Base = impl::HourBase;

public:
    using Base::Base;
};

}
