// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Fixture.hpp"

namespace app::constant_time {

/// Deliberately variable-work fixture used to validate real timing detection.
/// @notest{Explicit optimized self-test checks its expected leakage.}
class LeakyFixture final : public Fixture {
public:
    /// Select a deliberately different secret workload.
    /// @param population Whether to execute 128 instead of 64 iterations.
    explicit LeakyFixture(bool population) : _population{population} {}
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute one deliberately variable-work operation.
    [[nodiscard]] auto sample() const -> uint64_t;
    bool _population; ///< Deliberately observable secret population.
};

}
