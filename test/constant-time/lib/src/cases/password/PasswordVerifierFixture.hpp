// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/impl/PasswordHashData.hpp>
#include <erbsland/cryptology/PasswordHashKey.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

/// Prepared and validated PasswordVerifier sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class PasswordVerifierFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param keyed Protect the prepared verifier using a key.
    PasswordVerifierFixture(el::Random &random, bool population, bool keyed);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the configured operation once.
    [[nodiscard]] auto sample() -> uint64_t;

private:
    el::ByteBuffer _raw{};                       ///< Prepared derivation output.
    std::unique_ptr<el::PasswordHashKey> _key{}; ///< Optional verifier-protection key.
    ci::PasswordHashDataPtr _record{};           ///< Parsed immutable verifier record.
};

}
