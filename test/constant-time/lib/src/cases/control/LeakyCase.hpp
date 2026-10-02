// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LeakyFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

/// Explicitly selected leaky control, separate from algorithm registrations.
/// @notest{Explicit optimized self-test checks its expected leakage.}
class LeakyCase final : public PreparedCase<LeakyFixture> {
public:
    /// Configure the deliberately variable-work control.
    LeakyCase();
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected:
    [[nodiscard]] auto createFixture(el::Random &random, bool population) -> std::unique_ptr<LeakyFixture> override;
};

}
