// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Ed25519Operation.hpp"

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/impl/algorithm/ed25519_signature/Ed25519Signer.hpp>
#include <erbsland/cryptology/impl/algorithm/ed25519_signature/FieldElement.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

/// Prepared and validated Ed25519 sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class Ed25519Fixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param operation One fixed operation selected before repetitions.
    Ed25519Fixture(el::Random &random, bool population, Ed25519Operation operation);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the statically selected operation once.
    template <Ed25519Operation tOperation>
    [[nodiscard]] auto sample() -> uint64_t;

private:
    Ed25519Operation _operation;                    ///< Scenario configuration.
    el::ByteBuffer _secret{};                       ///< Fixed or random scalar bytes.
    el::ByteBuffer _message{};                      ///< Fixed public message.
    ci::ed25519_signer::Scalar _ed{};               ///< Reduced Ed25519 scalar.
    ci::ed25519_signature::FieldElement _edField{}; ///< Prepared Edwards field element.
};

}
