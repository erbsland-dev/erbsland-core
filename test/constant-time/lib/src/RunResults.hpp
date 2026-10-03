// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Result.hpp"

#include <erbsland/Definitions.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/List.hpp>

namespace app::constant_time {

/// Ordered collected outcomes and their documented exit-code precedence.
/// @tested{ConstantTimeRunnerTest}
class RunResults final {
public:
    /// Retain the next completed experiment.
    /// @param result Completed measurement state.
    void append(Result result) { _results.append(std::move(result)); }
    /// Get the number of completed experiments.
    [[nodiscard]] auto count() const noexcept -> el::ItemCount { return _results.count(); }
    /// Aggregate outcomes: interruption, errors, leakage, insufficient evidence, success.
    [[nodiscard]] auto exitCode() const noexcept -> int;

private:
    el::List<Result> _results; ///< Stable execution-order results.
};

}
