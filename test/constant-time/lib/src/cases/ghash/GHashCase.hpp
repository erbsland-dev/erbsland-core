// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GHashFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

/// Fixed-shape GHash scenario and explicit variant factory.
/// @notest{Its fixtures validate the selected algorithm before timing.}
class GHashCase final : public PreparedCase<GHashFixture> {
public:
    /// Configure the scenario and identify its concrete backend.
    /// @param accumulate Measure accumulation instead of one multiplication.
    /// @param automatic Allow factory selection of an accelerated backend.
    explicit GHashCase(bool accumulate, bool automatic);

public: // implement TestCase
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected: // implement PreparedCase
    [[nodiscard]] auto createFixture(el::Random &random, bool population) -> std::unique_ptr<GHashFixture> override;

private:
    bool _accumulate; ///< Scenario configuration.
    bool _automatic;  ///< Scenario configuration.
};

}
