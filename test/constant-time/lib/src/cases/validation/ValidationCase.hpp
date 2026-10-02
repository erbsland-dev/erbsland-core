// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValidationFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>

namespace app::constant_time {

/// Fixed-shape Validation scenario and explicit variant factory.
/// @notest{Its fixtures validate the selected algorithm before timing.}
class ValidationCase final : public PreparedCase<ValidationFixture> {
public:
    /// Configure the scenario and identify its concrete backend.
    /// @param operation One fixed operation selected before repetitions.
    explicit ValidationCase(ValidationOperation operation);

public: // implement TestCase
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected: // implement PreparedCase
    [[nodiscard]] auto createFixture(el::Random &random, bool population)
        -> std::unique_ptr<ValidationFixture> override;

private:
    /// Get the stable configured operation identifier.
    [[nodiscard]] auto identifier() const -> el::String;

private:
    ValidationOperation _operation; ///< Scenario configuration.
};

}
