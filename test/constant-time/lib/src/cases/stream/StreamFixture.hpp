// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamOperation.hpp"

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/ChaCha20BackendFactory.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

/// Prepared and validated Stream sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class StreamFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param size Public input length in bytes.
    /// @param operation One fixed operation selected before repetitions.
    /// @param automatic Allow factory selection of an accelerated backend.
    StreamFixture(el::Random &random, bool population, std::size_t size, StreamOperation operation, bool automatic);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the statically selected operation once.
    template <StreamOperation tOperation>
    [[nodiscard]] auto sample() -> uint64_t;

private:
    std::size_t _size;                              ///< Scenario configuration.
    StreamOperation _operation;                     ///< Scenario configuration.
    bool _automatic;                                ///< Scenario configuration.
    el::ByteBuffer _key{};                          ///< Fixed or random key.
    el::ByteBuffer _nonce{};                        ///< Fixed public nonce.
    el::ByteBuffer _data{};                         ///< Fixed public message.
    std::unique_ptr<ci::ChaCha20Backend> _chacha{}; ///< Prepared stream kernel.
};

}
