// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParameterType_fwd.hpp"

#include <cstdint>

namespace erbsland::profiling {

/// A supported declarative parameter type.
/// @tested{ProfilingDefinitionTest}
enum class ParameterType : std::uint8_t {
    Text,       ///< An Erbsland Core string.
    Integer,    ///< A signed integer.
    Boolean,    ///< A boolean flag.
    ByteLength, ///< A byte-count value.
    TimeDelta,  ///< A fixed time delta.
    Path,       ///< An Erbsland Core path.
};

}
