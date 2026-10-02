// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EcdsaFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>

namespace app::constant_time {

/// Fixed-shape Ecdsa scenario and explicit variant factory.
/// @notest{Its fixtures validate the selected algorithm before timing.}
class EcdsaCase final : public PreparedCase<EcdsaFixture> {
public:
    /// Configure the scenario and identify its concrete backend.
    /// @param name NIST curve with a fixed scalar width.
    /// @param operation One fixed operation selected before repetitions.
    explicit EcdsaCase(ci::NistPrimeCurve::Name name, EcdsaOperation operation);

public: // implement TestCase
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected: // implement PreparedCase
    [[nodiscard]] auto createFixture(el::Random &random, bool population) -> std::unique_ptr<EcdsaFixture> override;

private:
    /// Get the stable configured operation name.
    [[nodiscard]] auto operationName() const -> el::String;

private:
    ci::NistPrimeCurve::Name _name; ///< Scenario configuration.
    EcdsaOperation _operation;      ///< Scenario configuration.
};

}
