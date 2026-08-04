// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Argument.hpp"

#include "../engine/ProgramReader.hpp"

namespace erbsland::re::impl {

/// An extension to the program reader with a generic interface for the disassembler.
class GenericProgramReader : public ProgramReader {
public:
    /// Create a generic reader for `program`.
    explicit GenericProgramReader(const Program &program) noexcept : ProgramReader{program} {}

public:
    /// Read the operation at a program counter and its decoded arguments.
    [[nodiscard]] auto readOperation(ProgramCounter &programCounter) const noexcept -> std::pair<Operation, Arguments> {
        auto operation = peekOperation(programCounter);
        switch (operation.raw()) {
        case Operation::None:
        case Operation::Match:
        case Operation::NotMatch:
        case Operation::Success:
        case Operation::Failure:
        case Operation::Any:
            skipOperation(programCounter);
            return {operation, Arguments{}};
        case Operation::StartAtomic:
        case Operation::StopAtomic: {
            const auto atomicGroupId = readAtomic(programCounter);
            return {operation, Arguments{atomicGroupId}};
        }
        case Operation::Split: {
            const auto [programCounterA, programCounterB] = readSplit(programCounter);
            return {operation, Arguments{programCounterA, programCounterB}};
        }
        case Operation::Jump: {
            const auto programCounterA = readJump(programCounter);
            return {operation, Arguments{programCounterA}};
        }
        case Operation::Anchor: {
            const auto value = readAnchorValue(programCounter);
            return {operation, Arguments{value}};
        }
        case Operation::StartCapture:
        case Operation::StopCapture: {
            const auto captureIndex = readCapture(programCounter);
            return {operation, Arguments{captureIndex}};
        }
        case Operation::Counter:
        case Operation::AddCounter:
        case Operation::Maximum:
        case Operation::SkipIfMaximum:
        case Operation::Minimum: {
            const auto [counterIndex, value] = readCounter(programCounter);
            return {operation, Arguments{counterIndex, value}};
        }
        case Operation::Char:
        case Operation::CiChar:
        case Operation::NotChar:
        case Operation::NotCiChar:
            return {operation, Arguments{readCharValue(programCounter)}};
        case Operation::Sequence:
        case Operation::CiSequence: {
            const auto [sequenceIndex, sequenceLength] = readSequence(programCounter);
            return {operation, Arguments{sequenceIndex, sequenceLength}};
        }
        case Operation::Category:
        case Operation::NotCategory:
        case Operation::AssertCategory:
        case Operation::NotAssertCategory:
            return {operation, Arguments{readCategoryValue(programCounter)}};
        case Operation::Class:
        case Operation::CiClass:
        case Operation::NotClass:
        case Operation::NotCiClass:
            return {operation, Arguments{readClass(programCounter)}};
        default:
            skipOperation(programCounter);
            return {Operation::Unknown, Arguments{}};
        }
    }
};

}
