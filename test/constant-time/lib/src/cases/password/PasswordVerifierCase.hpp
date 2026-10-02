// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PasswordVerifierFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

/// Fixed-shape PasswordVerifier scenario and explicit variant factory.
/// @notest{Its fixtures validate the selected algorithm before timing.}
class PasswordVerifierCase final : public PreparedCase<PasswordVerifierFixture> {
public:
    /// Configure the scenario and identify its concrete backend.
    /// @param keyed Protect the prepared verifier using a key.
    explicit PasswordVerifierCase(bool keyed);

public: // implement TestCase
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected: // implement PreparedCase
    [[nodiscard]] auto createFixture(el::Random &random, bool population)
        -> std::unique_ptr<PasswordVerifierFixture> override;

private:
    bool _keyed; ///< Scenario configuration.
};

}
