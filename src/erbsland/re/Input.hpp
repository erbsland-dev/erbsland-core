// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroup_fwd.hpp"
#include "Input_fwd.hpp"
#include "InputBase.hpp"
#include "Match_fwd.hpp"

namespace erbsland::re {

/// An abstract input for regular expression matching.
/// @tested{InputBaseTest}
class Input : public InputBase {
protected:
    using InputBase::InputBase;

public:
    ~Input() override = default;

public:
    /// Create a match object for this input.
    /// @param captureGroupList The list of capture groups.
    /// @return An owning match result that holds a copy of the matched text.
    /// Implementations may throw encoding errors or other runtime errors; matching operations propagate them unchanged.
    [[nodiscard]] virtual auto createMatch(CaptureGroupList captureGroupList) -> MatchPtr = 0;
};

}
