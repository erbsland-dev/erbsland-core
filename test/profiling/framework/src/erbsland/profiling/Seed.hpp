// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>

#include <cstdint>

namespace erbsland::profiling {

/// Derive a stable workload seed.
/// @param globalSeed Global run seed.
/// @param scenarioId Stable expanded scenario ID.
/// @param worker Worker index.
/// @param sample Sample index.
/// @return Deterministically mixed seed.
/// @tested{SeedTest}
[[nodiscard]] auto deriveSeed(
    std::uint64_t globalSeed, const String &scenarioId, std::uint32_t worker, std::uint64_t sample) noexcept
    -> std::uint64_t;

}
