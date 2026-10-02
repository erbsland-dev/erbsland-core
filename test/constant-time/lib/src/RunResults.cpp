// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RunResults.hpp"

namespace app::constant_time {

auto RunResults::exitCode() const noexcept -> int {
    auto interrupted = false;
    auto errors = false;
    auto leakage = false;
    auto insufficient = false;
    for (const auto &result : _results) {
        interrupted |= result.outcome() == Outcome::Interrupted;
        errors |= result.outcome() == Outcome::Error;
        leakage |= result.outcome() == Outcome::Leakage;
        insufficient |= result.outcome() == Outcome::Insufficient || result.outcome() == Outcome::Unavailable;
    }
    if (interrupted) {
        return 130;
    }
    if (errors) {
        return 2;
    }
    if (leakage) {
        return 1;
    }
    if (insufficient) {
        return 3;
    }
    return 0;
}

}
