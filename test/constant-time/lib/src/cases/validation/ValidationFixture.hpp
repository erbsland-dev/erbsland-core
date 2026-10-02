// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValidationOperation.hpp"

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/symmetric/SymmetricTag.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

/// Prepared and validated Validation sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class ValidationFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param operation One fixed operation selected before repetitions.
    ValidationFixture(el::Random &random, bool population, ValidationOperation operation);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the statically selected operation once.
    template <ValidationOperation tOperation>
    [[nodiscard]] auto sample() -> uint64_t;
    /// Reject the prepared input while preserving its observable outcome.
    template <ValidationOperation tOperation>
    [[nodiscard]] auto reject() -> uint64_t;

private:
    ValidationOperation _operation; ///< Scenario configuration.
    el::ByteBuffer _key{};          ///< Fixed key.
    el::ByteBuffer _nonce{};        ///< Fixed nonce/IV.
    el::ByteBlock _ciphertext{};    ///< Prepared valid or deliberately invalid ciphertext.
    el::SymmetricTag _tag{};        ///< Invalid complete tag.
};

}
