// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InputBase.hpp"
#include "Match32.hpp"

#include <memory>

namespace erbsland::re {

class Input32;
/// A shared pointer to a UTF-32 input instance.
using Input32Ptr = std::shared_ptr<Input32>;

/// An abstract UTF-32 input for regular expression matching.
/// @tested{RegExUtf16Utf32Test}
class Input32 : public InputBase {
protected:
    using InputBase::InputBase;

public:
    ~Input32() override = default;

public:
    /// Create a match object for this input.
    /// @param regEx The regular expression object that created the match.
    /// @param captureGroupList The list of capture groups.
    /// @return An owning match result that holds a copy of the matched text.
    /// Implementations may throw encoding errors or other runtime errors; matching operations propagate them unchanged.
    [[nodiscard]] virtual auto createMatch(ConstRegExPtr regEx, CaptureGroupList captureGroupList) -> Match32Ptr = 0;
};

}
