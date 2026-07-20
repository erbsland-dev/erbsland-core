// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IntegerFormat_fwd.hpp"

#include "../../math/IntegerTraits.hpp"
#include "../../unit/CpLength_fwd.hpp"

namespace erbsland::text::impl {

class StringAppendTools;

/// Append an integer to a decoded-character sink.
template <math::AnyIntegerType T>
auto appendInteger(StringAppendTools &sink, T value, const IntegerFormat &format) -> unit::CpLength;

}
