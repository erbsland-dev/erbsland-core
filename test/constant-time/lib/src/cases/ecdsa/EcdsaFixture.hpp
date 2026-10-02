// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EcdsaOperation.hpp"

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/impl/algorithm/ecdsa_signature/EcdsaSigner.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

/// Prepared and validated ECDSA sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class EcdsaFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param name NIST curve with a fixed scalar width.
    /// @param operation One fixed operation selected before repetitions.
    EcdsaFixture(el::Random &random, bool population, ci::NistPrimeCurve::Name name, EcdsaOperation operation);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the statically selected operation once.
    template <EcdsaOperation tOperation>
    [[nodiscard]] auto sample() -> uint64_t;

private:
    ci::NistPrimeCurve::Name _name;     ///< Scenario configuration.
    EcdsaOperation _operation;          ///< Scenario configuration.
    el::ByteBuffer _secret{};           ///< Fixed or random scalar bytes.
    el::ByteBuffer _message{};          ///< Fixed public message.
    ci::NistPrimeCurve::Number _nist{}; ///< Prepared valid NIST scalar.
};

}
