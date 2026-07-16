// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/impl/engine/Program.hpp>
#include <erbsland/re/impl/engine/ProgramReader.hpp>
#include <erbsland/re/impl/engine/ProgramWriter.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using el::text::Char;
using impl::AtomicGroupId;
using impl::Category;
using impl::Operation;
using impl::Program;
using impl::ProgramCounter;
using impl::ProgramPtr;
using impl::ProgramReader;
using impl::ProgramWriter;
using impl::TextAnchor;

TESTED_TARGETS(ProgramReader ProgramWriter)
TAGS(Matching)
class ProgramReaderWriterTest final : public el::UnitTest {
public:
    Program program;
    ProgramCounter programCounter;
    ProgramReader reader{program};
    ProgramWriter writer{program, programCounter};
    Operation operation;

    void setUp() override {
        program.clear();
        programCounter = 0;
    }

    void testSetup() {
        // make sure they share the same program.
        REQUIRE_EQUAL(programCounter, writer.programCounter());
        programCounter = 123;
        REQUIRE_EQUAL(programCounter, writer.programCounter());
    }

    void testNone() {
        writer.writeNone();
        REQUIRE_EQUAL(programCounter, 1U);
        REQUIRE_EQUAL(program.size(), 1U);
        REQUIRE_EQUAL(program.data()[0], 0x00000000U);
        programCounter = 0;
        operation = reader.peekOperation(programCounter);
        REQUIRE_EQUAL(operation, Operation::None);
    }

    void testMatchNoMatchSuccessFailureAny() {
        writer.writeMatch();
        writer.writeNotMatch();
        writer.writeSuccess();
        writer.writeFailure();
        writer.writeAny();

        REQUIRE_EQUAL(programCounter, 5U);
        REQUIRE_EQUAL(program.size(), 5U);

        REQUIRE_EQUAL(program.data()[0], Operation{Operation::Match}.toCode());
        REQUIRE_EQUAL(program.data()[1], Operation{Operation::NotMatch}.toCode());
        REQUIRE_EQUAL(program.data()[2], Operation{Operation::Success}.toCode());
        REQUIRE_EQUAL(program.data()[3], Operation{Operation::Failure}.toCode());
        REQUIRE_EQUAL(program.data()[4], Operation{Operation::Any}.toCode());

        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Match);
        reader.skipOperation(programCounter);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::NotMatch);
        reader.skipOperation(programCounter);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Success);
        reader.skipOperation(programCounter);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Failure);
        reader.skipOperation(programCounter);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Any);
        reader.skipOperation(programCounter);
        REQUIRE_EQUAL(programCounter, 5U);
    }

    void testSplit() {
        const ProgramCounter a = 0x1234U;
        const ProgramCounter b = 0x00ABU;
        writer.writeSplit(a, b);
        REQUIRE_EQUAL(programCounter, 2U);
        REQUIRE_EQUAL(program.size(), 2U);
        // First word encodes op + low 24-bit payload (here: a)
        REQUIRE_EQUAL(program.data()[0], static_cast<Program::Code>((Operation{Operation::Split}.toCode() | a)));
        // Second word is the raw B target
        REQUIRE_EQUAL(program.data()[1], static_cast<Program::Code>(b));

        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Split);
        auto [ra, rb] = reader.readSplit(programCounter);
        REQUIRE_EQUAL(ra, static_cast<ProgramCounter>(a & 0xFFFFU));
        REQUIRE_EQUAL(rb, b);
        REQUIRE_EQUAL(programCounter, 2U);
    }

    void testJump() {
        const ProgramCounter dest = 0x3456U;
        writer.writeJump(dest);
        REQUIRE_EQUAL(programCounter, 1U);
        REQUIRE_EQUAL(program.size(), 1U);
        REQUIRE_EQUAL(program.data()[0], static_cast<Program::Code>(Operation{Operation::Jump}.toCode() | dest));
        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Jump);
        const auto rd = reader.readJump(programCounter);
        REQUIRE_EQUAL(rd, static_cast<ProgramCounter>(dest & 0xFFFFU));
        REQUIRE_EQUAL(programCounter, 1U);
    }

    void testAnchor() {
        const auto anchor = TextAnchor::LineEnd;
        writer.writeAnchor(anchor);
        REQUIRE_EQUAL(programCounter, 1U);
        REQUIRE_EQUAL(program.size(), 1U);
        REQUIRE_EQUAL(
            program.data()[0],
            static_cast<Program::Code>(
                Operation{Operation::Anchor}.toCode() | (static_cast<Program::Code>(anchor) & 0xFFU)));
        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Anchor);
        const auto ra = reader.readAnchor(programCounter);
        REQUIRE(ra == TextAnchor::LineEnd);
        REQUIRE_EQUAL(programCounter, 1U);
    }

    void testStartStopCapture() {
        const std::size_t startIdx = 7U;
        const std::size_t stopIdx = 8U;

        writer.writeStartCapture(startIdx);
        writer.writeStopCapture(stopIdx);

        REQUIRE_EQUAL(programCounter, 2U);
        REQUIRE_EQUAL(program.size(), 2U);
        const auto startCode =
            static_cast<Program::Code>(Operation{Operation::StartCapture}.toCode() | (startIdx & 0xFFFFU));
        REQUIRE_EQUAL(program.data()[0], startCode);
        const auto stopCode =
            static_cast<Program::Code>(Operation{Operation::StopCapture}.toCode() | (stopIdx & 0xFFFFU));
        REQUIRE_EQUAL(program.data()[1], stopCode);

        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::StartCapture);
        const auto rs = reader.readCapture(programCounter);
        REQUIRE_EQUAL(rs, startIdx & 0xFFU);

        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::StopCapture);
        const auto re = reader.readCapture(programCounter);
        REQUIRE_EQUAL(re, stopIdx & 0xFFU);

        REQUIRE_EQUAL(programCounter, 2U);
    }

    void testStartStopAtomic() {
        const AtomicGroupId atomicGroupId = 42;
        writer.writeStartAtomic(atomicGroupId);
        writer.writeStopAtomic(atomicGroupId);

        REQUIRE_EQUAL(programCounter, 2U);
        REQUIRE_EQUAL(program.size(), 2U);

        REQUIRE_EQUAL(program.data()[0], Operation{Operation::StartAtomic}.toCode() | (atomicGroupId & 0xFFFFU));
        REQUIRE_EQUAL(program.data()[1], Operation{Operation::StopAtomic}.toCode() | (atomicGroupId & 0xFFFFU));

        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::StartAtomic);
        REQUIRE_EQUAL(reader.readAtomic(programCounter), atomicGroupId);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::StopAtomic);
        REQUIRE_EQUAL(reader.readAtomic(programCounter), atomicGroupId);
        REQUIRE_EQUAL(programCounter, 2U);
    }

    void testChar() {
        const Char ch{'A'};
        writer.writeChar(ch);
        REQUIRE_EQUAL(programCounter, 1U);
        REQUIRE_EQUAL(program.size(), 1U);
        REQUIRE_EQUAL(
            program.data()[0],
            static_cast<Program::Code>(
                Operation{Operation::Char}.toCode() | (static_cast<Program::Code>(ch.toRawValue()) & 0x00FFFFFFU)));
        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Char);
        const auto rc = reader.readChar(programCounter);
        REQUIRE_EQUAL(rc.toRawValue(), ch.toRawValue());
        REQUIRE_EQUAL(programCounter, 1U);
    }

    void testNullCharacterAndInvalidCharacterBoundary() {
        writer.writeChar(Char::null());
        REQUIRE_EQUAL(programCounter, 1U);
        REQUIRE_EQUAL(program.data()[0], Operation{Operation::Char}.toCode());

        programCounter = 0;
        const auto nullCharacter = reader.readChar(programCounter);
        REQUIRE(nullCharacter.isNull());
        REQUIRE_FALSE(nullCharacter.isEndOfData());

        REQUIRE_THROWS_AS(RegExError, writer.writeChar(Char::endOfData()));
        REQUIRE_THROWS_AS(RegExError, writer.writeChar(Char{0xD800U}));
        REQUIRE_EQUAL(program.size(), 1U);
    }

    void testNegatedAndCaseInsensitiveChar() {
        const Char ch{U'Z'};
        writer.writeCiChar(ch);
        writer.writeNotChar(ch);
        writer.writeNotCiChar(ch);

        REQUIRE_EQUAL(programCounter, 3U);
        REQUIRE_EQUAL(program.size(), 3U);
        REQUIRE_EQUAL(program.data()[0], Operation{Operation::CiChar}.toCode() | (ch.toRawValue() & 0x00FFFFFFU));
        REQUIRE_EQUAL(program.data()[1], Operation{Operation::NotChar}.toCode() | (ch.toRawValue() & 0x00FFFFFFU));
        REQUIRE_EQUAL(program.data()[2], Operation{Operation::NotCiChar}.toCode() | (ch.toRawValue() & 0x00FFFFFFU));

        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::CiChar);
        REQUIRE_EQUAL(reader.readChar(programCounter).toRawValue(), ch.toRawValue());
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::NotChar);
        REQUIRE_EQUAL(reader.readChar(programCounter).toRawValue(), ch.toRawValue());
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::NotCiChar);
        REQUIRE_EQUAL(reader.readChar(programCounter).toRawValue(), ch.toRawValue());
        REQUIRE_EQUAL(programCounter, 3U);
    }

    void testCategory() {
        const Category category{Category::WordAscii};
        writer.writeCategory(category);
        writer.writeNotCategory(category);

        REQUIRE_EQUAL(programCounter, 2U);
        REQUIRE_EQUAL(program.size(), 2U);
        REQUIRE_EQUAL(program.data()[0], Operation{Operation::Category}.toCode() | (category.mask() & 0x00FFFFFFU));
        REQUIRE_EQUAL(program.data()[1], Operation{Operation::NotCategory}.toCode() | (category.mask() & 0x00FFFFFFU));

        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Category);
        REQUIRE(reader.readCategory(programCounter) == category);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::NotCategory);
        REQUIRE(reader.readCategory(programCounter) == category);
        REQUIRE_EQUAL(programCounter, 2U);
    }

    void testClass() {
        const std::size_t classIndex = 0x00B5U; // within 16-bit
        writer.writeClass(classIndex);
        writer.writeCiClass(classIndex);
        writer.writeNotClass(classIndex);
        writer.writeNotCiClass(classIndex);

        REQUIRE_EQUAL(programCounter, 4U);
        REQUIRE_EQUAL(program.size(), 4U);
        REQUIRE_EQUAL(program.data()[0], Operation{Operation::Class}.toCode() | (classIndex & 0xFFFFU));
        REQUIRE_EQUAL(program.data()[1], Operation{Operation::CiClass}.toCode() | (classIndex & 0xFFFFU));
        REQUIRE_EQUAL(program.data()[2], Operation{Operation::NotClass}.toCode() | (classIndex & 0xFFFFU));
        REQUIRE_EQUAL(program.data()[3], Operation{Operation::NotCiClass}.toCode() | (classIndex & 0xFFFFU));

        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Class);
        REQUIRE_EQUAL(reader.readClass(programCounter), classIndex & 0xFFFFU);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::CiClass);
        REQUIRE_EQUAL(reader.readClass(programCounter), classIndex & 0xFFFFU);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::NotClass);
        REQUIRE_EQUAL(reader.readClass(programCounter), classIndex & 0xFFFFU);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::NotCiClass);
        REQUIRE_EQUAL(reader.readClass(programCounter), classIndex & 0xFFFFU);
        REQUIRE_EQUAL(programCounter, 4U);
    }

    void testSequence() {
        const uint32_t offset = 0xBEEFU; // uses low 16 bits
        const uint32_t length = 0x7FU;   // uses 8 bits (bit 16..23)

        writer.writeSequence(offset, length);
        REQUIRE_EQUAL(programCounter, 1U);
        REQUIRE_EQUAL(program.size(), 1U);

        const auto expected = static_cast<Program::Code>(
            Operation{Operation::Sequence}.toCode() | static_cast<Program::Code>(offset & 0x0000FFFFU) |
            static_cast<Program::Code>((length & 0x000000FFU) << 16));
        REQUIRE_EQUAL(program.data()[0], expected);

        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Sequence);
        const auto [roffset, rlength] = reader.readSequence(programCounter);
        REQUIRE_EQUAL(roffset, static_cast<uint32_t>(offset & 0x0000FFFFU));
        REQUIRE_EQUAL(rlength, static_cast<uint32_t>(length & 0x000000FFU));
        REQUIRE_EQUAL(programCounter, 1U);
    }

    void testPatch() {
        writer.writeJump(0x0001U);
        writer.writeJump(0x0002U);
        writer.writeJump(0x0003U);
        writer.writeSplit(0x0004U, 0x0005U);
        writer.writeSplit(0x0006U, 0x0007U);
        writer.writeSequence(0x1111U, 0x22U);
        writer.writeCiSequence(0x2222U, 0x33U);
        writer.writeClass(0x4444U);
        writer.writeCiClass(0x5555U);
        writer.writeNone();
        REQUIRE_EQUAL(programCounter, 12U);
        REQUIRE_EQUAL(program.size(), 12U);

        writer.setProgramCounter(0x0001U);
        writer.patchOffset(0xabcdU, 0);
        writer.setProgramCounter(0x0003U);
        writer.patchOffset(0x1234U, 0);
        writer.setProgramCounter(0x0003U);
        writer.patchOffset(0x5678U, 1);
        writer.setProgramCounter(0x0007U); // Sequence
        writer.patchOffset(0x9999U, 0);
        writer.setProgramCounter(0x0008U); // CiSequence
        writer.patchOffset(0x8888U, 0);
        writer.setProgramCounter(0x0009U); // Class
        writer.patchOffset(0xaaaaU, 0);
        writer.setProgramCounter(0x000AU); // CiClass
        writer.patchOffset(0xbbbbU, 0);

        REQUIRE_EQUAL(program.size(), 12U);

        programCounter = 0;
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Jump);
        REQUIRE_EQUAL(reader.readJump(programCounter), 0x0001U);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Jump);
        REQUIRE_EQUAL(reader.readJump(programCounter), 0xabcdU);
        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Jump);
        REQUIRE_EQUAL(reader.readJump(programCounter), 0x0003U);

        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Split);
        auto [a, b] = reader.readSplit(programCounter);
        REQUIRE_EQUAL(a, 0x1234U);
        REQUIRE_EQUAL(b, 0x5678U);

        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Split);
        std::tie(a, b) = reader.readSplit(programCounter);
        REQUIRE_EQUAL(a, 0x0006U);
        REQUIRE_EQUAL(b, 0x0007U);

        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Sequence);
        const auto [seqOffset, seqLength] = reader.readSequence(programCounter);
        REQUIRE_EQUAL(seqOffset, 0x9999U);
        REQUIRE_EQUAL(seqLength, 0x22U);

        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::CiSequence);
        const auto [ciSeqOffset, ciSeqLength] = reader.readSequence(programCounter);
        REQUIRE_EQUAL(ciSeqOffset, 0x8888U);
        REQUIRE_EQUAL(ciSeqLength, 0x33U);

        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::Class);
        REQUIRE_EQUAL(reader.readClass(programCounter), 0xaaaaU);

        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::CiClass);
        REQUIRE_EQUAL(reader.readClass(programCounter), 0xbbbbU);

        REQUIRE_EQUAL(reader.peekOperation(programCounter), Operation::None);

        writer.setProgramCounter(0x0004U);              // -> Split (second word of first Split)
        REQUIRE_THROWS(writer.patchOffset(0x0001U, 0));
        writer.setProgramCounter(0x000BU);              // -> None
        REQUIRE_THROWS(writer.patchOffset(0x0001U, 0)); // None can't be patched.
    }
};
