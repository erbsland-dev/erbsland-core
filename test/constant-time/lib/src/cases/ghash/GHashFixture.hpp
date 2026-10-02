// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/GaloisMultiplierFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/GHash.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

/// Prepared and validated GHASH sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class GHashFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param accumulate Measure accumulation instead of one multiplication.
    /// @param automatic Allow factory selection of an accelerated backend.
    GHashFixture(el::Random &random, bool population, bool accumulate, bool automatic);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the configured operation once.
    template <bool tAccumulate>
    [[nodiscard]] auto sample() -> uint64_t;

private:
    bool _accumulate;                                    ///< Scenario configuration.
    bool _automatic;                                     ///< Scenario configuration.
    ci::GaloisMultiplier::Block _value{};                ///< Fixed public ciphertext block.
    ci::GaloisMultiplier::Block _subkey{};               ///< Fixed or random sensitive hash subkey.
    std::unique_ptr<ci::GaloisMultiplier> _multiplier{}; ///< Selected kernel.
};

}
