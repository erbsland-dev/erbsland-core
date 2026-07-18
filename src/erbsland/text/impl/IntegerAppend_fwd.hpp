// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IntegerFormat_fwd.hpp"

#include "../../math/IntegerTraits.hpp"

namespace erbsland::text::impl {

/// Append an integer to a decoded-character sink.
template <typename tSink, math::AnyIntegerType T>
void appendInteger(tSink &sink, T value, const IntegerFormat &format);

}
