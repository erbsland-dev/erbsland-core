// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RunMode_fwd.hpp"

#include <cstdint>

namespace erbsland::profiling {

/// The common profiling execution mode.
/// @tested{ConfigurationLoaderTest}
enum class RunMode : std::uint8_t {
    Profile,   ///< Repeat weighted scenarios until the deadline.
    Benchmark, ///< Collect calibrated timing samples.
};

}
