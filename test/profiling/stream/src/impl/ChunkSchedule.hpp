// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ChunkSchedule_fwd.hpp"

#include "../ProfileTypes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace app::stream::impl {

/// Produces transfer chunk sizes for a stream scenario.
/// @notest{Covered by stream profiler CTest entries.}
class ChunkSchedule final {
public:
    /// Create a chunk schedule for `scenario` using `seed`.
    ChunkSchedule(const Scenario &scenario, const std::uint64_t seed) :
        _scenario{scenario}, _random{seed}, _next{scenario.chunkSizeMinimum} {}

    /// Get the next transfer size, limited by the remaining data.
    [[nodiscard]] auto next(const std::uint64_t remaining = std::numeric_limits<std::uint64_t>::max())
        -> std::uint64_t {
        auto result = _scenario.chunkSizeMaximum;
        if (_scenario.chunkMode == ChunkMode::Fixed) {
            result = _scenario.chunkSizeMaximum;
        } else if (_scenario.chunkMode == ChunkMode::Incrementing) {
            result = _next;
            if (_next >= _scenario.chunkSizeMaximum) {
                _next = _scenario.chunkSizeMinimum;
            } else if (_next > _scenario.chunkSizeMaximum / 2U) {
                _next = _scenario.chunkSizeMaximum;
            } else {
                _next *= 2U;
            }
        } else {
            ++_randomIndex;
            if (_randomIndex % 4U == 0U) {
                constexpr auto boundaries = std::array<std::uint64_t, 12>{
                    1U, 7U, 63U, 64U, 65U, 4095U, 4096U, 4097U, 65535U, 65536U, 65537U, 1048576U};
                result = boundaries[_random.getUInt64(0U, boundaries.size() - 1U)];
                result = std::clamp(result, _scenario.chunkSizeMinimum, _scenario.chunkSizeMaximum);
            } else {
                const auto minimumLog = std::log2(static_cast<double>(_scenario.chunkSizeMinimum));
                const auto maximumLog = std::log2(static_cast<double>(_scenario.chunkSizeMaximum));
                result = static_cast<std::uint64_t>(std::exp2(_random.getDouble(minimumLog, maximumLog)));
                result = std::clamp(result, _scenario.chunkSizeMinimum, _scenario.chunkSizeMaximum);
            }
        }
        return std::max<std::uint64_t>(1U, std::min(result, remaining));
    }

private:
    const Scenario &_scenario;
    el::FastRandom _random;
    std::uint64_t _next;
    std::uint64_t _randomIndex{};
};

}
