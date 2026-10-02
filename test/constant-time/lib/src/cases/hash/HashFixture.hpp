// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

/// Prepared and validated Hash sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class HashFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param algorithm Fixed-output hash algorithm.
    HashFixture(el::Random &random, bool population, el::HashAlgorithm algorithm);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the configured operation once.
    [[nodiscard]] auto sample() -> uint64_t;

private:
    el::ByteBuffer _data{}; ///< Message bytes.
    el::Hasher _hasher{};   ///< Worker prepared before measurement.
};

}
