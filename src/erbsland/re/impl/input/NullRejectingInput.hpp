// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../InputBase.hpp"

namespace erbsland::re::impl {

/// An input decorator that rejects null characters when they are read.
/// @tested{RegExNullCharacterTest RegExStreamInputTest}
class NullRejectingInput final : public InputBase {
public:
    /// Create a null-rejecting decorator for an input.
    [[nodiscard]] static auto create(InputBasePtr input) -> InputBasePtr;

    explicit NullRejectingInput(InputBasePtr input);

public: // implement InputBase
    [[nodiscard]] auto read() -> CharAndPosition override;
    [[nodiscard]] auto peek() -> CharAndPosition override;
    void skip(unit::CpLength characterCount) override;

private:
    [[nodiscard]] static auto validate(CharAndPosition result) -> CharAndPosition;

private:
    InputBasePtr _input; ///< The decorated input.
};

}
