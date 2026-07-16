// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupTypes.hpp"
#include "CharClassData.hpp"
#include "Counter.hpp"
#include "Operation.hpp"
#include "Program.hpp"
#include "ProgramHelpers.hpp"
#include "SequenceData.hpp"

#include "../error/InternalError.hpp"
#include "../text/Category.hpp"
#include "../text/Character.hpp"
#include "../text/TextAnchor.hpp"

namespace erbsland::re::impl {

/// A program writer.
class ProgramWriter : public ProgramHelpers {
public:
    /// Create a new program writer that operates on the given program and counter.
    /// @param program The program to write.
    /// @param programCounter The program counter to use.
    explicit ProgramWriter(Program &program, ProgramCounter &programCounter) noexcept :
        _program{program}, _programCounter{programCounter} {}

public: // program counter
    [[nodiscard]] auto programCounter() const noexcept -> ProgramCounter { return _programCounter; }

    void setProgramCounter(const ProgramCounter programCounter) const noexcept { _programCounter = programCounter; }

public: // write code
    void writeNone() { _program.writeCode(Operation{Operation::None}.toCode(), _programCounter); }

    void writeMatch() { _program.writeCode(Operation{Operation::Match}.toCode(), _programCounter); }

    void writeNotMatch() { _program.writeCode(Operation{Operation::NotMatch}.toCode(), _programCounter); }

    void writeSuccess() { _program.writeCode(Operation{Operation::Success}.toCode(), _programCounter); }

    void writeFailure() { _program.writeCode(Operation{Operation::Failure}.toCode(), _programCounter); }

    void writeSplit(const ProgramCounter programCounterA, const ProgramCounter programCounterB) {
        const auto code = Operation{Operation::Split}.toCode() | codeProgramCounter(programCounterA);
        _program.writeCode(code, _programCounter);
        _program.writeCode(programCounterB, _programCounter);
    }

    void writeJump(const ProgramCounter programCounter) {
        const auto code = Operation{Operation::Jump}.toCode() | codeProgramCounter(programCounter);
        _program.writeCode(code, _programCounter);
    }

    void writeAnchor(const TextAnchor anchor) {
        const auto code = Operation{Operation::Anchor}.toCode() | codeLowerWord(anchor.raw());
        _program.writeCode(code, _programCounter);
    }

    /// Write a START_CAPTURE operation.
    /// @param captureIndex The capture index (0-based, representing the 1st capture group).
    void writeStartCapture(const std::size_t captureIndex) {
        const auto code =
            Operation{Operation::StartCapture}.toCode() | codeLowerWord(static_cast<uint16_t>(captureIndex));
        _program.writeCode(code, _programCounter);
    }

    /// Write a STOP_CAPTURE operation.
    /// @param captureIndex The capture index (0-based, representing the 1st capture group).
    void writeStopCapture(const std::size_t captureIndex) {
        const auto code =
            Operation{Operation::StopCapture}.toCode() | codeLowerWord(static_cast<uint16_t>(captureIndex));
        _program.writeCode(code, _programCounter);
    }

    void writeStartAtomic(const AtomicGroupId atomicGroupId) {
        const auto code = Operation{Operation::StartAtomic}.toCode() | codeLowerWord(atomicGroupId);
        _program.writeCode(code, _programCounter);
    }

    void writeStopAtomic(const AtomicGroupId atomicGroupId) {
        const auto code = Operation{Operation::StopAtomic}.toCode() | codeLowerWord(atomicGroupId);
        _program.writeCode(code, _programCounter);
    }

    void writeCounter(const CounterIndex counterIndex, const CounterType value) {
        const auto code = Operation{Operation::Counter}.toCode() | codeHigherByte(counterIndex) | codeLowerWord(value);
        _program.writeCode(code, _programCounter);
    }

    void writeAddCounter(const CounterIndex counterIndex, const CounterType value) {
        const auto code =
            Operation{Operation::AddCounter}.toCode() | codeHigherByte(counterIndex) | codeLowerWord(value);
        _program.writeCode(code, _programCounter);
    }

    void writeMaximum(const CounterIndex counterIndex, const CounterType maxValue) {
        const auto code =
            Operation{Operation::Maximum}.toCode() | codeHigherByte(counterIndex) | codeLowerWord(maxValue);
        _program.writeCode(code, _programCounter);
    }

    void writeSkipIfMaximum(const CounterIndex counterIndex, const CounterType maxValue) {
        const auto code =
            Operation{Operation::SkipIfMaximum}.toCode() | codeHigherByte(counterIndex) | codeLowerWord(maxValue);
        _program.writeCode(code, _programCounter);
    }

    void writeMinimum(const CounterIndex counterIndex, const CounterType minValue) {
        const auto code =
            Operation{Operation::Minimum}.toCode() | codeHigherByte(counterIndex) | codeLowerWord(minValue);
        _program.writeCode(code, _programCounter);
    }

    void writeChar(const text::Char character) { writeCharacterOperation(Operation::Char, character); }

    void writeCiChar(const text::Char character) { writeCharacterOperation(Operation::CiChar, character); }

    void writeNotChar(const text::Char character) { writeCharacterOperation(Operation::NotChar, character); }

    void writeNotCiChar(const text::Char character) { writeCharacterOperation(Operation::NotCiChar, character); }

    void writeSequence(const SequenceIndex offset, const SequenceLength length) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(length <= 0xFF, "Sequence length out of bounds"_el);
        const auto code = Operation{Operation::Sequence}.toCode() | codeLowerWord(offset) |
            codeHigherByte(static_cast<uint8_t>(length));
        _program.writeCode(code, _programCounter);
    }

    void writeCiSequence(const SequenceIndex offset, const SequenceLength length) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(length <= 0xFF, "Sequence length out of bounds"_el);
        const auto code = Operation{Operation::CiSequence}.toCode() | codeLowerWord(offset) |
            codeHigherByte(static_cast<uint8_t>(length));
        _program.writeCode(code, _programCounter);
    }

    void writeCategory(const Category category) {
        const auto code = Operation{Operation::Category}.toCode() | codeLower24bits(category.mask());
        _program.writeCode(code, _programCounter);
    }

    void writeNotCategory(const Category category) {
        const auto code = Operation{Operation::NotCategory}.toCode() | codeLower24bits(category.mask());
        _program.writeCode(code, _programCounter);
    }

    void writeAssertCategory(const Category category) {
        const auto code = Operation{Operation::AssertCategory}.toCode() | codeLower24bits(category.mask());
        _program.writeCode(code, _programCounter);
    }

    void writeNotAssertCategory(const Category category) {
        const auto code = Operation{Operation::NotAssertCategory}.toCode() | codeLower24bits(category.mask());
        _program.writeCode(code, _programCounter);
    }

    void writeClass(const CharClassIndex classIndex) {
        const auto code = Operation{Operation::Class}.toCode() | codeLowerWord(classIndex);
        _program.writeCode(code, _programCounter);
    }

    void writeCiClass(const CharClassIndex classIndex) {
        const auto code = Operation{Operation::CiClass}.toCode() | codeLowerWord(classIndex);
        _program.writeCode(code, _programCounter);
    }

    void writeNotClass(const CharClassIndex classIndex) {
        const auto code = Operation{Operation::NotClass}.toCode() | codeLowerWord(classIndex);
        _program.writeCode(code, _programCounter);
    }

    void writeNotCiClass(const CharClassIndex classIndex) {
        const auto code = Operation{Operation::NotCiClass}.toCode() | codeLowerWord(classIndex);
        _program.writeCode(code, _programCounter);
    }

    void writeAny() { _program.writeCode(Operation{Operation::Any}.toCode(), _programCounter); }

    void writeProgram(const Program &otherProgram) {
        for (const auto code : otherProgram.data()) {
            _program.writeCode(code, _programCounter);
        }
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
    void writeCharacterOperation(const Operation::Value operation, const text::Char character) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(character.isValidUnicode(), "Cannot write an invalid Unicode character"_el);
        const auto code = Operation{operation}.toCode() | codeLower24bits(character.toRawValue());
        _program.writeCode(code, _programCounter);
    }

private:
    Program &_program;
    ProgramCounter &_programCounter;
};

}
