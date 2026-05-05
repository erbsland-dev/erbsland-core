// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/TimePartBases.hpp"

namespace erbsland::time {

/// A minute within an hour, range `0..59`.
///
/// Provides clamped arithmetic.
/// @tested{TimePartsTest}
class Minute final : public impl::MinuteBase {
    using Base = impl::MinuteBase;

public:
    using Base::Base;
};

}
