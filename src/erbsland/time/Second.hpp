// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/TimePartBases.hpp"

namespace erbsland::time {

/// A second within a minute, range `0..59`.
///
/// Provides clamped arithmetic.
/// @tested{TimePartsTest}
class Second final : public impl::SecondBase {
    using Base = impl::SecondBase;

public:
    using Base::Base;
};

}
