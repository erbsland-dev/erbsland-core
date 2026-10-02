// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AuthenticationFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>

namespace app::constant_time {

/// Fixed-shape Authentication scenario and explicit variant factory.
/// @notest{Its fixtures validate the selected algorithm before timing.}
class AuthenticationCase final : public PreparedCase<AuthenticationFixture> {
public:
    /// Configure the scenario and identify its concrete backend.
    /// @param algorithm Fixed-output hash algorithm.
    /// @param operation One fixed operation selected before repetitions.
    explicit AuthenticationCase(el::HashAlgorithm algorithm, AuthenticationOperation operation);

public: // implement TestCase
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected: // implement PreparedCase
    [[nodiscard]] auto createFixture(el::Random &random, bool population)
        -> std::unique_ptr<AuthenticationFixture> override;

private:
    /// Get the stable configured operation name.
    [[nodiscard]] auto operationName() const -> el::String;

private:
    el::HashAlgorithm _algorithm;       ///< Scenario configuration.
    AuthenticationOperation _operation; ///< Scenario configuration.
};

}
