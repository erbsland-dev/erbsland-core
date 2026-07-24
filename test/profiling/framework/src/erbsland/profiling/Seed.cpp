// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Seed.hpp"

namespace erbsland::profiling {

auto deriveSeed(
    std::uint64_t globalSeed, const String &scenarioId, const std::uint32_t worker, const std::uint64_t sample) noexcept
    -> std::uint64_t {
    auto mix = [](std::uint64_t value) noexcept -> std::uint64_t {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31U);
    };
    auto result = mix(globalSeed);
    for (const auto character : scenarioId) {
        result = mix(result ^ static_cast<std::uint64_t>(character.toRawValue()));
    }
    return mix(result ^ mix(worker) ^ mix(sample));
}

}
