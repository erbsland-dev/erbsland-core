// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AuthenticationOperation.hpp"

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/Hmac.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

/// Prepared and validated Authentication sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class AuthenticationFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param algorithm Fixed-output hash algorithm.
    /// @param operation One fixed operation selected before repetitions.
    AuthenticationFixture(
        el::Random &random, bool population, el::HashAlgorithm algorithm, AuthenticationOperation operation);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the statically selected operation once.
    template <AuthenticationOperation tOperation>
    [[nodiscard]] auto sample() -> uint64_t;

private:
    el::HashAlgorithm _algorithm;       ///< Scenario configuration.
    AuthenticationOperation _operation; ///< Scenario configuration.
    el::ByteBuffer _key{};              ///< Sensitive material or expected tag.
    el::ByteBuffer _data{};             ///< Fixed public message.
    std::unique_ptr<el::Hmac> _hmac{};  ///< Prepared cached verifier.
};

}
