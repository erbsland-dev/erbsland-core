// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupTypes.hpp"
#include "Counter.hpp"
#include "Operation.hpp"
#include "Program.hpp"
#include "ProgramHelpers.hpp"
#include "SequenceData.hpp"

#include "../text/Category.hpp"
#include "../text/TextAnchor.hpp"

namespace erbsland::re::impl {

/// Read the program instructions.
class ProgramReader : public ProgramHelpers {
public:
    /// Bind the program reader to a program.
    explicit ProgramReader(const Program &program) noexcept : _program{program} {}

    // defaults
    ~ProgramReader() = default;
    ProgramReader(const ProgramReader &) = delete;
    ProgramReader(ProgramReader &&) = default;
    auto operator=(const ProgramReader &) -> ProgramReader & = delete;
    auto operator=(ProgramReader &&) -> ProgramReader & = delete;

public: // decode
    [[nodiscard]] auto peekOperation(const ProgramCounter programCounter) const -> Operation {
        if (programCounter >= _program.size()) {
            return Operation::Failure;
        }
        return Operation::fromCode(_program.peekCode(programCounter));
    }

    void skipOperation(ProgramCounter &programCounter) const { _program.skipCode(programCounter); }

    /// @return Programm counter A and B.
    [[nodiscard]] auto readSplit(ProgramCounter &programCounter) const -> std::pair<ProgramCounter, ProgramCounter> {
        auto programCounterA = extractProgramCounter(_program.readCode(programCounter));
        auto programCounterB = extractProgramCounter(_program.readCode(programCounter));
        return {programCounterA, programCounterB};
    }

    /// @return The program counter.
    [[nodiscard]] auto readJump(ProgramCounter &programCounter) const -> ProgramCounter {
        return extractProgramCounter(_program.readCode(programCounter));
    }

    /// @return The text anchor value.
    [[nodiscard]] auto readAnchorValue(ProgramCounter &programCounter) const -> uint8_t {
        return extractByte<0>(_program.readCode(programCounter));
    }

    /// @return The text anchor.
    [[nodiscard]] auto readAnchor(ProgramCounter &programCounter) const -> TextAnchor {
        return TextAnchor{static_cast<TextAnchor::Value>(readAnchorValue(programCounter))};
    }

    /// The `START CAPTURE` and `STOP CAPTURE` operations.
    /// @return The capture group index (0-based, representing the 1st capture group).
    [[nodiscard]] auto readCapture(ProgramCounter &programCounter) const -> uint16_t {
        return extractLowerWord(_program.readCode(programCounter));
    }

    /// The `START ATOMIC` and `STOP ATOMIC` operations.
    /// @return The atomic group id.
    [[nodiscard]] auto readAtomic(ProgramCounter &programCounter) const -> AtomicGroupId {
        return static_cast<AtomicGroupId>(extractLowerWord(_program.readCode(programCounter)));
    }

    /// Read `COUNTER`, `ADD COUNTER`, `MAXIMUM` and `SKIP MAXIMUM`, 'MINIMUM`
    /// @return The counter-index, the value to set.
    [[nodiscard]] auto readCounter(ProgramCounter &programCounter) const -> std::pair<CounterIndex, CounterType> {
        const auto code = _program.readCode(programCounter);
        const auto counterIndex = extractHigherByte(code);
        const auto value = extractLowerWord(code);
        return {counterIndex, value};
    }

    /// Read `CHAR`, `CI CHAR`, `NOT CHAR` and `NOT CI CHAR`
    /// @return The character value.
    [[nodiscard]] auto readCharValue(ProgramCounter &programCounter) const -> uint32_t {
        return extractLower24bits(_program.readCode(programCounter));
    }

    /// Read `CHAR`, `CI CHAR`, `NOT CHAR` and `NOT CI CHAR`
    /// @return The char.
    [[nodiscard]] auto readChar(ProgramCounter &programCounter) const -> text::Char {
        return text::Char{static_cast<char32_t>(readCharValue(programCounter))};
    }

    /// Read `SEQUENCE` and `CI SEQUENCE`
    /// @return The sequence index, the sequence length.
    [[nodiscard]] auto readSequence(ProgramCounter &programCounter) const -> std::pair<SequenceIndex, SequenceLength> {
        const auto code = _program.readCode(programCounter);
        const auto sequenceIndex = extractLowerWord(code);
        const auto sequenceLength = extractHigherByte(code);
        return {sequenceIndex, sequenceLength};
    }

    /// @return The category value
    [[nodiscard]] auto readCategoryValue(ProgramCounter &programCounter) const -> uint32_t {
        return extractLower24bits(_program.readCode(programCounter));
    }

    /// Read `CATEGORY` and `NOT CATEGORY`
    /// @return The category
    [[nodiscard]] auto readCategory(ProgramCounter &programCounter) const -> Category {
        return Category{static_cast<Category::Value>(readCategoryValue(programCounter))};
    }

    /// Read `CLASS`, `CI CLASS`, `NOT CLASS` and `NOT CI CLASS`
    /// @return The class-index
    [[nodiscard]] auto readClass(ProgramCounter &programCounter) const -> uint16_t {
        return extractLowerWord(_program.readCode(programCounter));
    }

private:
    const Program &_program;
};

}
