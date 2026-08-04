// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RunConfiguration_fwd.hpp"
#include "RunMode.hpp"

#include <erbsland/all.hpp>

#include <cstdint>

namespace erbsland::profiling {

using namespace text::literals;

/// Settings shared by a complete profiling run.
/// @tested{ConfigurationLoaderTest WorkloadRunnerTest}
struct RunConfiguration {
    RunMode mode{RunMode::Benchmark};                         ///< Execution mode.
    String suite{"snapshot"_el};                              ///< Selected built-in suite.
    TimeDelta duration{TimeDelta::minutes(5)};                ///< Hard run deadline.
    std::uint32_t threadCount{4U};                            ///< Workload worker count.
    std::uint64_t seed{0x455242534c414e44ULL};                ///< Global deterministic seed.
    std::uint32_t warmupSamples{1U};                          ///< Warm-up samples per scenario.
    std::uint32_t samples{9U};                                ///< Measured benchmark samples.
    TimeDelta minimumSampleTime{TimeDelta::milliseconds(20)}; ///< Calibration target.
    ByteLength memoryLimit{256U * 1024U * 1024U};             ///< Aggregate fixture memory limit.
    TimeDelta progressInterval{TimeDelta::seconds(10)};       ///< Progress reporting interval.
};

}
