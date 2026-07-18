// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CharAndPosition_fwd.hpp"
#include "InputBase_fwd.hpp"

#include "../unit/CpLength.hpp"

#include <memory>

namespace erbsland::re {

/// The abstract base class for inputs for regular expression matching.
/// Exceptions raised by input operations propagate unchanged through the matching engine.
/// @tested{InputBaseTest}
class InputBase : public std::enable_shared_from_this<InputBase> {
public:
    InputBase() = default;
    virtual ~InputBase() = default;
    InputBase(const InputBase &) = delete;
    auto operator=(const InputBase &) -> InputBase & = delete;
    InputBase(InputBase &&) = delete;
    auto operator=(InputBase &&) -> InputBase & = delete;

public:
    /// Read the next character from the input and advance the position.
    /// Implementations must return a valid Unicode scalar value, or `text::Char::endOfData()` after exhaustion.
    /// Repeated reads after exhaustion must keep returning the end-of-data signal.
    /// Implementations may throw encoding errors or other runtime errors; matching operations propagate them unchanged.
    /// @return 1. The next character from the input, or the end-of-data signal after exhaustion.
    ///     2. The start position of the read character (the position of the first byte of the read character).
    [[nodiscard]] virtual auto read() -> CharAndPosition = 0;
    /// Peek at the next character from the input, do *not* advance the position.
    /// Implementations must follow the same valid-scalar/end-of-data contract as `read()`.
    /// Implementations may throw encoding errors or other runtime errors; matching operations propagate them unchanged.
    /// @return 1. The next character from the input, or the end-of-data signal after exhaustion.
    ///     2. The start position of the read character (the position of the first byte of the read character).
    [[nodiscard]] virtual auto peek() -> CharAndPosition = 0;
    /// Skip a number of characters.
    /// Skipping beyond the available input must leave the input exhausted.
    /// Implementations may throw encoding errors or other runtime errors; matching operations propagate them unchanged.
    virtual void skip(unit::CpLength characterCount) = 0;
};

}
