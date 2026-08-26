// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Opcode.hpp"

#include "../../../unit/CodeLocation.hpp"

#include <cstdint>

namespace erbsland::text::render::impl {

/// One decoded layout-program instruction.
/// @tested{RenderProgramTest}
class ProgramInstruction final {
public:
    /// Create a decoded instruction.
    ProgramInstruction(Opcode opcode, uint64_t operand, unit::CodeLocation location) noexcept :
        _opcode{opcode}, _operand{operand}, _location{location} {}

public: // accessors
    /// Get the decoded opcode.
    [[nodiscard]] auto opcode() const noexcept -> Opcode { return _opcode; }
    /// Get the decoded operand, or zero for an instruction without one.
    [[nodiscard]] auto operand() const noexcept -> uint64_t { return _operand; }
    /// Get the source location associated with the instruction.
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation { return _location; }

private:
    Opcode _opcode;               ///< The decoded opcode.
    uint64_t _operand;            ///< The decoded operand, or zero.
    unit::CodeLocation _location; ///< The corresponding layout-source location.
};

}
