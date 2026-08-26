// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BuiltInFilter.hpp"
#include "Opcode.hpp"
#include "Program.hpp"
#include "ValueTest.hpp"

#include "../../../mem/ByteWriter.hpp"
#include "../../EscapeFormat.hpp"

#include <cstddef>
#include <vector>

namespace erbsland::text::render::impl {

/// Encode the private layout-program bytecode format.
/// @tested{RenderProgramTest RenderCompilerTest}
class ProgramWriter final {
public:
    /// A bytecode position used as a validated backward-jump target.
    class Label final {
        friend class ProgramWriter;

    public:
        /// Test if this label identifies a bytecode position.
        [[nodiscard]] auto isValid() const noexcept -> bool { return !_position.isNoIndex(); }

    private:
        /// Create a label at a bytecode position.
        explicit Label(unit::ByteIndex position) noexcept : _position{position} {}

    private:
        unit::ByteIndex _position{unit::ByteIndex::noIndex()}; ///< Target bytecode position.
    };

public:
    /// A forward jump operand awaiting its target.
    class JumpHandle final {
        friend class ProgramWriter;

    public:
        /// Test if this handle identifies a jump operand.
        [[nodiscard]] auto isValid() const noexcept -> bool { return !_operandPosition.isNoIndex(); }

    private:
        /// Create a handle for one fixed-width jump operand.
        explicit JumpHandle(unit::ByteIndex operandPosition) noexcept : _operandPosition{operandPosition} {}

    private:
        unit::ByteIndex _operandPosition{unit::ByteIndex::noIndex()}; ///< Jump operand byte position.
    };

public:
    /// Write an `EmitText` instruction.
    void writeEmitText(std::size_t constantIndex, unit::CodeLocation location);
    /// Write a `LoadName` instruction.
    void writeLoadName(std::size_t constantIndex, unit::CodeLocation location);
    /// Write a `GetMember` instruction.
    void writeGetMember(std::size_t constantIndex, unit::CodeLocation location);
    /// Write a `PushTrue` instruction.
    void writePushTrue(unit::CodeLocation location);
    /// Write a `PushFalse` instruction.
    void writePushFalse(unit::CodeLocation location);
    /// Write a `PushNull` instruction.
    void writePushNull(unit::CodeLocation location);
    /// Write a `PushInteger` instruction.
    void writePushInteger(int64_t value, unit::CodeLocation location);
    /// Write a `PushFloat` instruction.
    void writePushFloat(double value, unit::CodeLocation location);
    /// Write a `PushText` instruction.
    void writePushText(std::size_t constantIndex, unit::CodeLocation location);
    /// Write a `LogicalNot` instruction.
    void writeLogicalNot(unit::CodeLocation location);
    /// Write a comparison instruction.
    void writeComparison(Opcode opcode, unit::CodeLocation location);
    /// Write an application-filter instruction.
    void writeApplicationFilter(std::size_t constantIndex, std::size_t argumentCount, unit::CodeLocation location);
    /// Write a built-in-filter instruction.
    void writeBuiltInFilter(BuiltInFilter filter, std::size_t argumentCount, unit::CodeLocation location);
    /// Write a unary arithmetic instruction.
    void writeUnary(Opcode opcode, unit::CodeLocation location);
    /// Write a binary arithmetic, concatenation, or membership instruction.
    void writeBinary(Opcode opcode, unit::CodeLocation location);
    /// Write a value-test instruction.
    void writeTest(Opcode opcode, ValueTest test, unit::CodeLocation location);
    /// Write a `BuildList` instruction.
    void writeBuildList(std::size_t count, unit::CodeLocation location);
    /// Write a `BuildMap` instruction.
    void writeBuildMap(std::size_t count, unit::CodeLocation location);
    /// Write a `StoreName` instruction.
    void writeStoreName(std::size_t constantIndex, unit::CodeLocation location);
    /// Write a forward `Jump` instruction.
    [[nodiscard]] auto writeJump(unit::CodeLocation location) -> JumpHandle;
    /// Write a forward `JumpIfFalse` instruction.
    [[nodiscard]] auto writeJumpIfFalse(unit::CodeLocation location) -> JumpHandle;
    /// Write a forward `JumpIfFalseOrPop` instruction.
    [[nodiscard]] auto writeJumpIfFalseOrPop(unit::CodeLocation location) -> JumpHandle;
    /// Write a forward `JumpIfTrueOrPop` instruction.
    [[nodiscard]] auto writeJumpIfTrueOrPop(unit::CodeLocation location) -> JumpHandle;
    /// Mark the current bytecode position for a later backward jump.
    [[nodiscard]] auto markLabel() const noexcept -> Label;
    /// Write a `Jump` instruction to an existing instruction label.
    void writeJumpTo(Label label, unit::CodeLocation location);
    /// Patch a forward jump to the current program position.
    void patchJump(JumpHandle handle);
    /// Write a `BeginListIteration` instruction.
    void writeBeginListIteration(unit::CodeLocation location);
    /// Write a `BeginMapIteration` instruction.
    void writeBeginMapIteration(unit::CodeLocation location);
    /// Write a forward `NextIteration` instruction.
    [[nodiscard]] auto writeNextIteration(unit::CodeLocation location) -> JumpHandle;
    /// Write a `StoreScopedName` instruction.
    void writeStoreScopedName(std::size_t constantIndex, unit::CodeLocation location);
    /// Write an `EndIteration` instruction.
    void writeEndIteration(unit::CodeLocation location);
    /// Close an iteration and skip its else branch if it was non-empty.
    [[nodiscard]] auto writeEndIterationIfNotEmpty(unit::CodeLocation location) -> JumpHandle;
    /// Write an `Include` instruction.
    void writeInclude(std::size_t includeIndex, unit::CodeLocation location);
    /// Write a `RenderBlock` instruction.
    void writeRenderBlock(std::size_t nameConstantIndex, unit::CodeLocation location);
    /// Write a `LoadSuper` instruction.
    void writeLoadSuper(std::size_t depth, unit::CodeLocation location);
    /// Write an `EmitValue` instruction.
    void writeEmitValue(EscapeFormat format, unit::CodeLocation location);
    /// Write an unescaped `EmitValue` instruction.
    void writeEmitValue(unit::CodeLocation location) { writeEmitValue(EscapeFormat::None, location); }
    /// Write an `End` instruction.
    void writeEnd(unit::CodeLocation location);
    /// Take the complete program and reset this writer.
    [[nodiscard]] auto takeProgram() -> Program;

private:
    /// Write an opcode without an operand.
    void writeOpcode(Opcode opcode, unit::CodeLocation location);
    /// Write an opcode followed by an unsigned variable-length operand.
    void writeOpcode(Opcode opcode, std::size_t operand, unit::CodeLocation location);
    /// Write an opcode followed by a fixed-width operand.
    void writeFixedOpcode(Opcode opcode, uint64_t operand, unit::CodeLocation location);
    /// Write one forward jump opcode.
    [[nodiscard]] auto writeJumpOpcode(Opcode opcode, unit::CodeLocation location) -> JumpHandle;
    /// Write one filter opcode with its identifier and argument count.
    void writeFilterOpcode(
        Opcode opcode, std::size_t identifier, std::size_t argumentCount, unit::CodeLocation location);

private:
    mem::ByteWriter _writer;                         ///< Underlying byte writer.
    std::vector<Program::SourceMapEntry> _sourceMap; ///< Bytecode source map.
    std::vector<unit::ByteIndex> _openJumps;         ///< Unpatched jump operand positions.
};

}
