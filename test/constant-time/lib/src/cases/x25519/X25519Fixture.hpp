// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X25519Operation.hpp"

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/impl/algorithm/x25519/FieldElement.hpp>
#include <erbsland/cryptology/impl/algorithm/x25519/X25519.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

/// Prepared and validated X25519 sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class X25519Fixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param operation One fixed operation selected before repetitions.
    X25519Fixture(el::Random &random, bool population, X25519Operation operation);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the statically selected operation once.
    template <X25519Operation tOperation>
    [[nodiscard]] auto sample() -> uint64_t;

private:
    X25519Operation _operation;        ///< Scenario configuration.
    el::ByteBuffer _secret{};          ///< Fixed or random scalar bytes.
    el::ByteBuffer _message{};         ///< Fixed public message.
    ci::x25519::FieldElement _field{}; ///< Prepared Curve25519 field element.
};

}
