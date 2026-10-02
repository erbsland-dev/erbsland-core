// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RsaContext.hpp"
#include "RsaOperation.hpp"

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

/// Prepared and validated RSA sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class RsaFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param context Shared immutable pinned key and arithmetic components.
    /// @param operation One fixed operation selected before repetitions.
    RsaFixture(el::Random &random, bool population, std::shared_ptr<RsaContext> context, RsaOperation operation);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the statically selected operation once.
    template <RsaOperation tOperation>
    [[nodiscard]] auto sample() -> uint64_t;

private:
    std::shared_ptr<RsaContext> _context;  ///< Scenario configuration.
    RsaOperation _operation;               ///< Scenario configuration.
    ci::rsa_signature::Number _value{};    ///< Fixed or random secret operand.
    ci::rsa_signature::Number _exponent{}; ///< Fixed or random exponent of fixed represented width.
    el::ByteBuffer _message{};             ///< Fixed-size sensitive signing message.
    el::ByteBuffer _salt{};                ///< Prepared fixed-length PSS salt.
    el::ByteBuffer _blinding{};            ///< Explicit valid blinding factor.
};

}
