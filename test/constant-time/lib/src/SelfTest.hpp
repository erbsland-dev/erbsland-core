// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Runner.hpp"

namespace app::constant_time {

/// Deterministic harness checks and an explicitly requested real-time control.
/// @tested{ConstantTimeRunnerTest}
class SelfTest final {
public:
    /// Verify deterministic statistics and registry behavior.
    void checkDeterministic() const;
    /// Run the deliberately leaky fixture, requiring detection for success.
    /// @param runner Measurement and interruption handler.
    /// @return Control outcome.
    [[nodiscard]] auto runControl(Runner &runner) const -> Result;

private:
    /// Require a deterministic harness invariant.
    /// @param condition Required invariant.
    static void require(bool condition);
};

}
