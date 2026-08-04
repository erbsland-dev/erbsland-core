// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringHelper.hpp"

#include <erbsland/re/CaptureGroup.hpp>
#include <erbsland/re/impl/diagnostics/Assembler.hpp>
#include <erbsland/re/impl/diagnostics/Disassembler.hpp>
#include <erbsland/re/impl/engine/ProgramReader.hpp>
#include <erbsland/re/impl/Limits.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using impl::Assembler;
using impl::AtomicGroupId;
using impl::Category;
using impl::CounterIndex;
using impl::CounterType;
using impl::Disassembler;
using impl::EngineDataPtr;
using impl::Operation;
using impl::ProgramCounter;
using impl::ProgramPtr;
using impl::ProgramReader;
using impl::SequenceIndex;
using impl::SequenceLength;
using impl::TextAnchor;

/// Shared fixture helpers for regular-expression assembler diagnostics tests.
/// @notest{Test fixture base class.}
class AssemblerBase : public el::UnitTest {
public:
    EngineDataPtr engineData;
    std::unique_ptr<ProgramReader> reader;
    ProgramCounter programCounter;

    void setUp() override {
        engineData = {};
        reader = nullptr;
        programCounter = 0;
    }

    auto additionalErrorMessages() -> std::string override {
        try {
            Disassembler disassembler{engineData};
            std::string msg;
            for (const auto &line : disassembler.disassemble()) {
                msg += re_test::string_helper::toStdString(line);
                msg += '\n';
            }
            return msg;
        } catch (const std::exception &exc) {
            return std::format("Unexpected exception while providing additional error messages: {}", exc.what());
        }
    }

    /// Compile source lines and require the expected compiler error.
    void requireCompilerError(const el::text::StringList &lines, const std::string_view expectedError = {}) {

        Assembler assembler;
        std::string errorMessage;
        try {
            engineData = assembler.compile(lines);
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.title(), "Failed to assemble regular expression"_el);
            errorMessage = re_test::string_helper::toStdString(e.description());
        } catch (const std::exception &e) {
            consoleWriteLine(std::format("Unexpected std exception: {}", e.what()));
            REQUIRE(false);
        }
        if (!expectedError.empty()) {
            if (errorMessage.find(expectedError) == std::string::npos) {
                consoleWriteLine(
                    std::format("Expected error message to contain '{}', but got:\n{}", expectedError, errorMessage));
            }
            REQUIRE(errorMessage.find(expectedError) != std::string::npos);
        }
    }

    /// Compile source lines and require the expected compiler error.
    void requireCompilerError(
        const std::initializer_list<std::string_view> lines, const std::string_view expectedError = {}) {
        requireCompilerError(re_test::string_helper::toStringList(lines), expectedError);
    }

    /// Compile source lines and require successful assembly.
    void requireCompile(const std::initializer_list<std::string_view> lines) {
        Assembler assembler;
        REQUIRE_NOTHROW(engineData = assembler.compile(re_test::string_helper::toStringList(lines)));
        reader = std::make_unique<ProgramReader>(engineData->program);
        programCounter = 0;
    }

    // Methods to validate a given operation and arguments.
    // Use with `WITH_CONTEXT(require...)`

    /// Require an operation without any arguments.
    /// The program counter is moved to the next operation after this call.
    void requireOperation(const Operation expectedOperation) {
        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, expectedOperation);
        reader->skipOperation(programCounter);
    }

    /// Require a NONE operation.
    void requireNone() { requireOperation(Operation::None); }

    /// Require a MATCH operation.
    void requireMatch() { requireOperation(Operation::Match); }

    /// Require a NOT MATCH operation.
    void requireNotMatch() { requireOperation(Operation::NotMatch); }

    /// Require a SUCCESS operation.
    void requireSuccess() { requireOperation(Operation::Success); }

    /// Require a FAILURE operation.
    void requireFailure() { requireOperation(Operation::Failure); }

    /// Require an ANY operation.
    void requireAny() { requireOperation(Operation::Any); }

    /// Require a JUMP with the given address.
    void requireJump(const ProgramCounter expectedAddress) {
        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, Operation::Jump);
        const auto actualAddress = reader->readJump(programCounter);
        REQUIRE_EQUAL(actualAddress, expectedAddress);
    }

    /// Require a SPLIT with the given two addresses.
    void requireSplit(const ProgramCounter expectedA, const ProgramCounter expectedB) {
        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, Operation::Split);
        const auto [actualA, actualB] = reader->readSplit(programCounter);
        REQUIRE_EQUAL(actualA, expectedA);
        REQUIRE_EQUAL(actualB, expectedB);
    }

    /// Require a class operation with the expected index.
    void requireClassBase(const uint16_t expectedIndex, const Operation expectedOperation) {
        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, expectedOperation);
        const auto actualIndex = reader->readClass(programCounter);
        REQUIRE_EQUAL(actualIndex, expectedIndex);
    }

    /// Require a CLASS with the given index.
    void requireClass(const uint16_t expectedIndex) { requireClassBase(expectedIndex, Operation::Class); }

    /// Require a NOT CLASS with the given index.
    void requireNotClass(const uint16_t expectedIndex) { requireClassBase(expectedIndex, Operation::NotClass); }

    /// Require a CI CLASS with the given index.
    void requireCiClass(const uint16_t expectedIndex) { requireClassBase(expectedIndex, Operation::CiClass); }

    /// Require a NOT CI CLASS with the given index.
    void requireNotCiClass(const uint16_t expectedIndex) { requireClassBase(expectedIndex, Operation::NotCiClass); }

    /// Require ANCHOR with the given value.
    void requireAnchor(const TextAnchor expectedAnchor) {
        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, Operation::Anchor);
        const auto actualAnchor = reader->readAnchor(programCounter);
        REQUIRE_EQUAL(actualAnchor, expectedAnchor);
    }

    /// Require START CAPTURE with the given group index
    void requireStartCapture(const CaptureGroupIndex expectedIndex) {
        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, Operation::StartCapture);
        const auto actualIndex = reader->readCapture(programCounter);
        REQUIRE_EQUAL(actualIndex, expectedIndex);
    }

    /// Require STOP CAPTURE with the given group index
    void requireStopCapture(const CaptureGroupIndex expectedIndex) {
        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, Operation::StopCapture);
        const auto actualIndex = reader->readCapture(programCounter);
        REQUIRE_EQUAL(actualIndex, expectedIndex);
    }

    /// Require a character operation with the expected character.
    void requireCharBase(const char32_t expectedChar, const Operation expectedOperation) {
        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, expectedOperation);
        const auto actualChar = reader->readChar(programCounter);
        REQUIRE_EQUAL(actualChar, expectedChar);
    }

    /// Require CHAR with the given character.
    void requireChar(const char32_t expectedChar) { requireCharBase(expectedChar, Operation::Char); }
    /// Require CI CHAR with the given character.
    void requireCiChar(const char32_t expectedChar) { requireCharBase(expectedChar, Operation::CiChar); }
    /// Require NOT CHAR with the given character.
    void requireNotChar(const char32_t expectedChar) { requireCharBase(expectedChar, Operation::NotChar); }
    /// Require NOT CI CHAR with the given character.
    void requireNotCiChar(const char32_t expectedChar) { requireCharBase(expectedChar, Operation::NotCiChar); }

    /// Require a sequence operation with the expected offset and length.
    void requireSequenceBase(
        const SequenceIndex expectedOffset, const SequenceLength expectedLength, const Operation expectedOperation) {

        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, expectedOperation);
        const auto [actualOffset, actualLength] = reader->readSequence(programCounter);
        REQUIRE_EQUAL(actualOffset, expectedOffset);
        REQUIRE_EQUAL(actualLength, expectedLength);
    }

    /// Require SEQUENCE with the given parameters.
    void requireSequence(const SequenceIndex expectedOffset, const SequenceLength expectedLength) {
        requireSequenceBase(expectedOffset, expectedLength, Operation::Sequence);
    }

    /// Require CI SEQUENCE with the given parameters.
    void requireCiSequence(const SequenceIndex expectedOffset, const SequenceLength expectedLength) {
        requireSequenceBase(expectedOffset, expectedLength, Operation::CiSequence);
    }

    /// Require a category operation with the expected category.
    void requireCategoryBase(const Category expectedCategory, const Operation expectedOperation) {
        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, expectedOperation);
        const auto actualCategory = reader->readCategory(programCounter);
        REQUIRE_EQUAL(actualCategory, expectedCategory);
    }

    /// Require CATEGORY with the given category.
    void requireCategory(const Category expectedCategory) {
        requireCategoryBase(expectedCategory, Operation::Category);
    }

    /// Require NOT CATEGORY with the given category.
    void requireNotCategory(const Category expectedCategory) {
        requireCategoryBase(expectedCategory, Operation::NotCategory);
    }

    /// Require ASSERT CATEGORY with the given category.
    void requireAssertCategory(const Category expectedCategory) {
        requireCategoryBase(expectedCategory, Operation::AssertCategory);
    }

    /// Require NOT ASSERT CATEGORY with the given category.
    void requireNotAssertCategory(const Category expectedCategory) {
        requireCategoryBase(expectedCategory, Operation::NotAssertCategory);
    }

    /// Require a counter operation with the expected index and value.
    void requireCounterBase(
        const CounterIndex expectedIndex, const CounterType expectedValue, const Operation expectedOperation) {

        REQUIRE(programCounter < engineData->program.size());
        const auto actualOperation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(actualOperation, expectedOperation);
        const auto [actualIndex, actualValue] = reader->readCounter(programCounter);
        REQUIRE_EQUAL(actualIndex, expectedIndex);
        REQUIRE_EQUAL(actualValue, expectedValue);
    }

    /// Require COUNTER with the given parameter.
    void requireCounter(const CounterIndex expectedIndex, const CounterType expectedValue) {
        requireCounterBase(expectedIndex, expectedValue, Operation::Counter);
    }

    /// Require ADD COUNTER with the given parameter.
    void requireAddCounter(const CounterIndex expectedIndex, const CounterType expectedValue) {
        requireCounterBase(expectedIndex, expectedValue, Operation::AddCounter);
    }

    /// Require MAXIMUM with the given parameter.
    void requireMaximum(const CounterIndex expectedIndex, const CounterType expectedValue) {
        requireCounterBase(expectedIndex, expectedValue, Operation::Maximum);
    }

    /// Require SKIP MAXIMUM with the given parameter.
    void requireSkipMaximum(const CounterIndex expectedIndex, const CounterType expectedValue) {
        requireCounterBase(expectedIndex, expectedValue, Operation::SkipIfMaximum);
    }

    /// Require MINIMUM with the given parameter.
    void requireMinimum(const CounterIndex expectedIndex, const CounterType expectedValue) {
        requireCounterBase(expectedIndex, expectedValue, Operation::Minimum);
    }

    /// Require a START ATOMIC operation with the expected group identifier.
    void requireStartAtomic(const AtomicGroupId expectedGroupId) {
        const auto operation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(operation, Operation::StartAtomic);
        const auto groupId = reader->readAtomic(programCounter);
        REQUIRE_EQUAL(groupId, expectedGroupId);
    }

    /// Require a STOP ATOMIC operation with the expected group identifier.
    void requireStopAtomic(const AtomicGroupId expectedGroupId) {
        const auto operation = reader->peekOperation(programCounter);
        REQUIRE_EQUAL(operation, Operation::StopAtomic);
        const auto groupId = reader->readAtomic(programCounter);
        REQUIRE_EQUAL(groupId, expectedGroupId);
    }
};
