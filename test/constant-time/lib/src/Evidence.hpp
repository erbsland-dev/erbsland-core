// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/text/String.hpp>

#include <cstdint>

namespace app::constant_time {

/// Largest eligible dudect statistic and its source.
/// @tested{ConstantTimeStatisticsTest}
struct Evidence final {
    double statistic{};    ///< Signed Welch statistic.
    el::String channel;    ///< Uncropped, cropped percentile, or second-order channel.
    uint64_t samples[2]{}; ///< Accepted counts in the reported analysis channel.
    bool eligible{};       ///< Whether an eligible statistical channel exists.
};

}
