// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParameterSelection_fwd.hpp"

#include <erbsland/all.hpp>

#include <cstdint>
#include <variant>

namespace erbsland::profiling {

/// One typed parameter stored in an expanded scenario.
/// @tested{ConfigurationLoaderTest}
struct ParameterSelection {
    using Value = std::variant<String, std::int64_t, bool, ByteLength, TimeDelta, Path>; ///< Supported values.

    String parameter;                                                                    ///< Parameter identifier.
    Value value{};                                                                       ///< Typed parameter value.
};

}
