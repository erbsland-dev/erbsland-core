// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InputBase.hpp"
#include "Match.hpp"

#include <memory>

namespace erbsland::re {

class Input;
/// A shared pointer to an input instance.
using InputPtr = std::shared_ptr<Input>;

/// An abstract input for regular expression matching.
/// @tested{InputBaseTest}
class Input : public InputBase {
protected:
    using InputBase::InputBase;

public:
    ~Input() override = default;

public:
    /// Create a match object for this input.
    /// @param regEx The regular expression object that created the match.
    /// @param captureGroupList The list of capture groups.
    /// @return An owning match result that holds a copy of the matched text.
    /// Implementations may throw encoding errors or other runtime errors; matching operations propagate them unchanged.
    [[nodiscard]] virtual auto createMatch(ConstRegExPtr regEx, CaptureGroupList captureGroupList) -> MatchPtr = 0;
};

}
