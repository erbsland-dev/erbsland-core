// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupTypes.hpp"
#include "CharClassData.hpp"
#include "Counter.hpp"
#include "Operation.hpp"
#include "Program.hpp"
#include "ProgramWriter_fwd.hpp"
#include "SequenceData.hpp"

#include "../error/InternalError.hpp"
#include "../Limits.hpp"
#include "../text/Category.hpp"
#include "../text/Character.hpp"
#include "../text/TextAnchor.hpp"

namespace erbsland::re::impl {

/// A program writer.
class ProgramWriter {
public:
    /// Create a new program writer that operates on the given program and counter.
    /// @param program The program to write.
    /// @param programCounter The program counter to use.
    explicit ProgramWriter(Program &program, ProgramCounter &programCounter) noexcept :
        _program{program}, _programCounter{programCounter} {}

public: // program counter
    /// Get the current position in the program.
    /// @return The current program counter.
    [[nodiscard]] auto programCounter() const noexcept -> ProgramCounter { return _programCounter; }

    /// Set the current position in the program.
    /// @param programCounter The new program counter.
    void setProgramCounter(const ProgramCounter programCounter) const noexcept { _programCounter = programCounter; }

public: // write code
    /// Write a NONE operation.
    void writeNone() { _program.writeCode(Operation{Operation::None}.toCode(), _programCounter); }

    /// Write a MATCH operation.
    void writeMatch() { _program.writeCode(Operation{Operation::Match}.toCode(), _programCounter); }

    /// Write a NOT_MATCH operation.
    void writeNotMatch() { _program.writeCode(Operation{Operation::NotMatch}.toCode(), _programCounter); }

    /// Write a SUCCESS operation.
    void writeSuccess() { _program.writeCode(Operation{Operation::Success}.toCode(), _programCounter); }

    /// Write a FAILURE operation.
    void writeFailure() { _program.writeCode(Operation{Operation::Failure}.toCode(), _programCounter); }

    /// Write a SPLIT operation.
    /// @param programCounterA The first branch program counter.
    /// @param programCounterB The second branch program counter.
    void writeSplit(const ProgramCounter programCounterA, const ProgramCounter programCounterB) {
        const auto code = Operation{Operation::Split}.toCode() | Program::codeProgramCounter(programCounterA);
        _program.writeCode(code, _programCounter);
        _program.writeCode(programCounterB, _programCounter);
    }

    /// Write a JUMP operation.
    /// @param programCounter The program counter to jump to.
    void writeJump(const ProgramCounter programCounter) {
        const auto code = Operation{Operation::Jump}.toCode() | Program::codeProgramCounter(programCounter);
        _program.writeCode(code, _programCounter);
    }

    /// Write an ANCHOR operation.
    /// @param anchor The anchor to match.
    void writeAnchor(const TextAnchor anchor) {
        const auto code = Operation{Operation::Anchor}.toCode() | Program::codeLowerWord(anchor.raw());
        _program.writeCode(code, _programCounter);
    }

    /// Write a START_CAPTURE operation.
    /// @param captureIndex The capture index (0-based, representing the 1st capture group).
    void writeStartCapture(const std::size_t captureIndex) {
        const auto code =
            Operation{Operation::StartCapture}.toCode() | Program::codeLowerWord(static_cast<uint16_t>(captureIndex));
        _program.writeCode(code, _programCounter);
    }

    /// Write a STOP_CAPTURE operation.
    /// @param captureIndex The capture index (0-based, representing the 1st capture group).
    void writeStopCapture(const std::size_t captureIndex) {
        const auto code =
            Operation{Operation::StopCapture}.toCode() | Program::codeLowerWord(static_cast<uint16_t>(captureIndex));
        _program.writeCode(code, _programCounter);
    }

    /// Write a START_ATOMIC operation.
    /// @param atomicGroupId The atomic group identifier.
    void writeStartAtomic(const AtomicGroupId atomicGroupId) {
        const auto code = Operation{Operation::StartAtomic}.toCode() | Program::codeLowerWord(atomicGroupId);
        _program.writeCode(code, _programCounter);
    }

    /// Write a STOP_ATOMIC operation.
    /// @param atomicGroupId The atomic group identifier.
    void writeStopAtomic(const AtomicGroupId atomicGroupId) {
        const auto code = Operation{Operation::StopAtomic}.toCode() | Program::codeLowerWord(atomicGroupId);
        _program.writeCode(code, _programCounter);
    }

    /// Write a COUNTER operation.
    /// @param counterIndex The counter index.
    /// @param value The value to assign.
    void writeCounter(const CounterIndex counterIndex, const CounterType value) {
        const auto code = Operation{Operation::Counter}.toCode() | Program::codeHigherByte(counterIndex) |
            Program::codeLowerWord(value);
        _program.writeCode(code, _programCounter);
    }

    /// Write an ADD_COUNTER operation.
    /// @param counterIndex The counter index.
    /// @param value The value to add.
    void writeAddCounter(const CounterIndex counterIndex, const CounterType value) {
        const auto code = Operation{Operation::AddCounter}.toCode() | Program::codeHigherByte(counterIndex) |
            Program::codeLowerWord(value);
        _program.writeCode(code, _programCounter);
    }

    /// Write a MAXIMUM operation.
    /// @param counterIndex The counter index.
    /// @param maxValue The maximum permitted value.
    void writeMaximum(const CounterIndex counterIndex, const CounterType maxValue) {
        const auto code = Operation{Operation::Maximum}.toCode() | Program::codeHigherByte(counterIndex) |
            Program::codeLowerWord(maxValue);
        _program.writeCode(code, _programCounter);
    }

    /// Write a SKIP_IF_MAXIMUM operation.
    /// @param counterIndex The counter index.
    /// @param maxValue The maximum value that causes the skip.
    void writeSkipIfMaximum(const CounterIndex counterIndex, const CounterType maxValue) {
        const auto code = Operation{Operation::SkipIfMaximum}.toCode() | Program::codeHigherByte(counterIndex) |
            Program::codeLowerWord(maxValue);
        _program.writeCode(code, _programCounter);
    }

    /// Write a MINIMUM operation.
    /// @param counterIndex The counter index.
    /// @param minValue The minimum permitted value.
    void writeMinimum(const CounterIndex counterIndex, const CounterType minValue) {
        const auto code = Operation{Operation::Minimum}.toCode() | Program::codeHigherByte(counterIndex) |
            Program::codeLowerWord(minValue);
        _program.writeCode(code, _programCounter);
    }

    /// Write a CHAR operation.
    /// @param character The character to match.
    void writeChar(const text::Char character) { writeCharacterOperation(Operation::Char, character); }

    /// Write a CI_CHAR operation.
    /// @param character The character to match.
    void writeCiChar(const text::Char character) { writeCharacterOperation(Operation::CiChar, character); }

    /// Write a NOT_CHAR operation.
    /// @param character The character to match.
    void writeNotChar(const text::Char character) { writeCharacterOperation(Operation::NotChar, character); }

    /// Write a NOT_CI_CHAR operation.
    /// @param character The character to match.
    void writeNotCiChar(const text::Char character) { writeCharacterOperation(Operation::NotCiChar, character); }

    /// Write a SEQUENCE operation.
    /// @param offset The sequence offset.
    /// @param length The sequence length.
    void writeSequence(const SequenceIndex offset, const SequenceLength length) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(length <= 0xFF, "Sequence length out of bounds"_el);
        const auto code = Operation{Operation::Sequence}.toCode() | Program::codeLowerWord(offset) |
            Program::codeHigherByte(static_cast<uint8_t>(length));
        _program.writeCode(code, _programCounter);
    }

    /// Write a CI_SEQUENCE operation.
    /// @param offset The sequence offset.
    /// @param length The sequence length.
    void writeCiSequence(const SequenceIndex offset, const SequenceLength length) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(length <= 0xFF, "Sequence length out of bounds"_el);
        const auto code = Operation{Operation::CiSequence}.toCode() | Program::codeLowerWord(offset) |
            Program::codeHigherByte(static_cast<uint8_t>(length));
        _program.writeCode(code, _programCounter);
    }

    /// Write a CATEGORY operation.
    /// @param category The character category to match.
    void writeCategory(const Category category) {
        const auto code = Operation{Operation::Category}.toCode() | Program::codeLower24bits(category.mask());
        _program.writeCode(code, _programCounter);
    }

    /// Write a NOT_CATEGORY operation.
    /// @param category The character category to exclude.
    void writeNotCategory(const Category category) {
        const auto code = Operation{Operation::NotCategory}.toCode() | Program::codeLower24bits(category.mask());
        _program.writeCode(code, _programCounter);
    }

    /// Write an ASSERT_CATEGORY operation.
    /// @param category The character category to assert.
    void writeAssertCategory(const Category category) {
        const auto code = Operation{Operation::AssertCategory}.toCode() | Program::codeLower24bits(category.mask());
        _program.writeCode(code, _programCounter);
    }

    /// Write a NOT_ASSERT_CATEGORY operation.
    /// @param category The character category to exclude.
    void writeNotAssertCategory(const Category category) {
        const auto code = Operation{Operation::NotAssertCategory}.toCode() | Program::codeLower24bits(category.mask());
        _program.writeCode(code, _programCounter);
    }

    /// Write a CLASS operation.
    /// @param classIndex The character class index.
    void writeClass(const CharClassIndex classIndex) {
        const auto code = Operation{Operation::Class}.toCode() | Program::codeLowerWord(classIndex);
        _program.writeCode(code, _programCounter);
    }

    /// Write a CI_CLASS operation.
    /// @param classIndex The character class index.
    void writeCiClass(const CharClassIndex classIndex) {
        const auto code = Operation{Operation::CiClass}.toCode() | Program::codeLowerWord(classIndex);
        _program.writeCode(code, _programCounter);
    }

    /// Write a NOT_CLASS operation.
    /// @param classIndex The character class index.
    void writeNotClass(const CharClassIndex classIndex) {
        const auto code = Operation{Operation::NotClass}.toCode() | Program::codeLowerWord(classIndex);
        _program.writeCode(code, _programCounter);
    }

    /// Write a NOT_CI_CLASS operation.
    /// @param classIndex The character class index.
    void writeNotCiClass(const CharClassIndex classIndex) {
        const auto code = Operation{Operation::NotCiClass}.toCode() | Program::codeLowerWord(classIndex);
        _program.writeCode(code, _programCounter);
    }

    /// Write an ANY operation.
    void writeAny() { _program.writeCode(Operation{Operation::Any}.toCode(), _programCounter); }

    /// Append another program to the program being written.
    /// @param otherProgram The program to append.
    void writeProgram(const Program &otherProgram) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            _programCounter == _program.size(), "Programs can only be appended at the end"_el);
        if (_program.size() + otherProgram.size() > limits::maximumProgramLength) {
            throw RegExError{
                ErrorCategory::Limit,
                "Failed to compile regular expression"_el,
                "The generated program exceeds the maximum program length."_el};
        }
        _program.append(otherProgram);
        _programCounter += static_cast<ProgramCounter>(otherProgram.size());
    }

public:
    /// Patch an offset for the operation at the current location.
    /// @param newOffset The new offset to write.
    /// @param argumentIndex The argument index to patch. Usually zero, but for operations like `Split` it
    ///   chooses which of the arguments to patch.
    void patchOffset(const uint32_t newOffset, const uint8_t argumentIndex) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(argumentIndex < 2, "Argument index out of bounds"_el);
        auto code = _program.peekCode(_programCounter);
        auto operation = Operation::fromCode(code);
        switch (operation.raw()) {
        case Operation::Split:
            if (argumentIndex == 0) {
                code = (static_cast<Program::Code>(newOffset) & 0x00FFFFFFU) | (code & 0xFF000000U);
                _program.writeCode(code, _programCounter);
            } else {
                _programCounter += 1;
                _program.writeCode(static_cast<Program::Code>(newOffset), _programCounter);
            }
            break;
        case Operation::Jump:
            code = (static_cast<Program::Code>(newOffset) & 0x00FFFFFFU) | (code & 0xFF000000U);
            _program.writeCode(code, _programCounter);
            break;
        case Operation::Sequence:
        case Operation::CiSequence:
        case Operation::Class:
        case Operation::CiClass:
            code = (static_cast<Program::Code>(newOffset) & 0x0000FFFFU) | (code & 0xFFFF0000U);
            _program.writeCode(code, _programCounter);
            break;
        default:
            throwInternalError("Unexpected operation for patching"_el);
        }
    }

private:
    /// Write a character-matching operation.
    /// @param operation The operation to write.
    /// @param character The character to match.
    void writeCharacterOperation(const Operation::Value operation, const text::Char character) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(character.isValidUnicode(), "Cannot write an invalid Unicode character"_el);
        const auto code = Operation{operation}.toCode() | Program::codeLower24bits(character.toRawValue());
        _program.writeCode(code, _programCounter);
    }

private:
    Program &_program;
    ProgramCounter &_programCounter;
};

}
