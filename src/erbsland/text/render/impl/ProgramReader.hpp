// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Opcode.hpp"
#include "Program.hpp"
#include "ProgramInstruction.hpp"

#include "../../../mem/ByteReader.hpp"

namespace erbsland::text::render::impl {

/// Decode the private layout-program bytecode format.
/// @tested{RenderProgramTest}
class ProgramReader final {
public:
    /// Create a reader for a layout program.
    explicit ProgramReader(const Program &program) noexcept : _program{program}, _reader{program.data()} {}

public:
    /// Test if the reader reached the end of the bytecode.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool { return _reader.isAtEnd(); }
    /// Decode the next complete instruction.
    /// @throws ProgramError If the bytecode is unknown, malformed, or truncated.
    [[nodiscard]] auto read() -> ProgramInstruction;
    /// Jump to an absolute instruction position.
    /// @throws ProgramError If the target is not the beginning of an instruction.
    void jumpTo(uint64_t target);

private:
    /// Read an unsigned variable-length operand.
    [[nodiscard]] auto readOperand() -> uint64_t;
    /// Read a fixed-width operand.
    [[nodiscard]] auto readFixedOperand() -> uint64_t;

private:
    const Program &_program;           ///< Program providing bytecode and source mappings.
    mem::ByteReader _reader;           ///< Underlying byte reader.
    std::size_t _nextSourceMapEntry{}; ///< Hint for sequential source-location lookup.
};

}
