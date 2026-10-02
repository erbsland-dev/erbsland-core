// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AesFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>

namespace app::constant_time {

/// Fixed-shape Aes scenario and explicit variant factory.
/// @notest{Its fixtures validate the selected algorithm before timing.}
class AesCase final : public PreparedCase<AesFixture> {
public:
    /// Configure the scenario and identify its concrete backend.
    /// @param keySize AES key length in bytes.
    /// @param operation One fixed operation selected before repetitions.
    /// @param automatic Allow factory selection of an accelerated backend.
    explicit AesCase(std::size_t keySize, AesOperation operation, bool automatic);

public: // implement TestCase
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected: // implement PreparedCase
    [[nodiscard]] auto createFixture(el::Random &random, bool population) -> std::unique_ptr<AesFixture> override;

private:
    /// Get the stable configured operation name.
    [[nodiscard]] auto operationName() const -> el::String;

private:
    std::size_t _keySize;    ///< Scenario configuration.
    AesOperation _operation; ///< Scenario configuration.
    bool _automatic;         ///< Scenario configuration.
};

}
