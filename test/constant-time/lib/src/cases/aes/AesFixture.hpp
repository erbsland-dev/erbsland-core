// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AesOperation.hpp"

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipherFactory.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

/// Prepared and validated AES sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class AesFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param keySize AES key length in bytes.
    /// @param operation One fixed operation selected before repetitions.
    /// @param automatic Allow factory selection of an accelerated backend.
    AesFixture(el::Random &random, bool population, std::size_t keySize, AesOperation operation, bool automatic);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the statically selected operation once.
    template <AesOperation tOperation>
    [[nodiscard]] auto sample() -> uint64_t;

private:
    AesOperation _operation;                       ///< Scenario configuration.
    el::ByteBuffer _key{};                         ///< Prepared secret key.
    std::unique_ptr<ci::AesBlockCipher> _cipher{}; ///< Backend prepared outside measurement.
    ci::AesBlockCipher::Block _plaintext{};        ///< Fixed input block.
    ci::AesBlockCipher::Block _ciphertext{};       ///< Valid ciphertext for decryption.
};

}
