// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/impl/tls/Tls13KeySchedule.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

/// Prepared and validated TlsFinished sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class TlsFinishedFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param algorithm Fixed-output hash algorithm.
    TlsFinishedFixture(el::Random &random, bool population, el::HashAlgorithm algorithm);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the configured operation once.
    [[nodiscard]] auto sample() -> uint64_t;

private:
    std::unique_ptr<ci::Tls13KeySchedule> _schedule{}; ///< Initialized handshake schedule.
    el::ByteBuffer _transcript{};                      ///< Fixed public transcript hash.
    el::ByteBuffer _expected{};                        ///< Tag differing at one position.
};

}
