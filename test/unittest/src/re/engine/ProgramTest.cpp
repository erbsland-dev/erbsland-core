// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/impl/engine/Program.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using impl::Program;
using impl::ProgramCounter;
using impl::ProgramPtr;

TESTED_TARGETS(Program)
TAGS(Matching)
class ProgramTest final : public el::UnitTest {
public:
    ProgramPtr program;
    ProgramCounter programCounter;

    void setUp() override {
        program = std::make_shared<Program>();
        programCounter = 0;
    }

    void testConstruction() { REQUIRE_EQUAL(program->size(), 0U); }

    void testSize() {
        REQUIRE_EQUAL(program->size(), 0U);
        program->writeCode(0x00000000U, programCounter);
        REQUIRE_EQUAL(program->size(), 1U);
        program->writeCode(0xffffffffU, programCounter);
        REQUIRE_EQUAL(program->size(), 2U);
    }

    void testWriteCode() {
        program->writeCode(0x00000000U, programCounter);
        REQUIRE_EQUAL(program->size(), 1U);
        REQUIRE_EQUAL(program->data()[0], 0x00000000U);
        REQUIRE_EQUAL(programCounter, 1U);
        program->writeCode(0xffffffffU, programCounter);
        REQUIRE_EQUAL(program->size(), 2U);
        REQUIRE_EQUAL(program->data()[0], 0x00000000U);
        REQUIRE_EQUAL(program->data()[1], 0xffffffffU);
        REQUIRE_EQUAL(programCounter, 2U);
        program->writeCode(0x12345678U, programCounter);
        REQUIRE_EQUAL(program->size(), 3U);
        REQUIRE_EQUAL(program->data()[0], 0x00000000U);
        REQUIRE_EQUAL(program->data()[1], 0xffffffffU);
        REQUIRE_EQUAL(program->data()[2], 0x12345678U);
        REQUIRE_EQUAL(programCounter, 3U);
        programCounter = 0;
        program->writeCode(0x87654321U, programCounter);
        REQUIRE_EQUAL(program->size(), 3U);
        REQUIRE_EQUAL(program->data()[0], 0x87654321U);
        REQUIRE_EQUAL(program->data()[1], 0xffffffffU);
        REQUIRE_EQUAL(program->data()[2], 0x12345678U);
        REQUIRE_EQUAL(programCounter, 1U);
        programCounter = 2;
        program->writeCode(0x11111111U, programCounter);
        REQUIRE_EQUAL(program->size(), 3U);
        REQUIRE_EQUAL(program->data()[0], 0x87654321U);
        REQUIRE_EQUAL(program->data()[1], 0xffffffffU);
        REQUIRE_EQUAL(program->data()[2], 0x11111111U);
        REQUIRE_EQUAL(programCounter, 3U);
        program->writeCode(0x22222222U, programCounter);
        REQUIRE_EQUAL(program->size(), 4U);
        REQUIRE_EQUAL(program->data()[0], 0x87654321U);
        REQUIRE_EQUAL(program->data()[1], 0xffffffffU);
        REQUIRE_EQUAL(program->data()[2], 0x11111111U);
        REQUIRE_EQUAL(program->data()[3], 0x22222222U);
        REQUIRE_EQUAL(programCounter, 4U);
    }

    void testPeekCode() {
        program->writeCode(0x11111111U, programCounter);
        program->writeCode(0x22222222U, programCounter);
        program->writeCode(0x33333333U, programCounter);
        program->writeCode(0x44444444U, programCounter);
        programCounter = 2;
        REQUIRE_EQUAL(program->peekCode(programCounter), 0x33333333U);
        REQUIRE_EQUAL(programCounter, 2U);
        programCounter = 3;
        REQUIRE_EQUAL(program->peekCode(programCounter), 0x44444444U);
        REQUIRE_EQUAL(programCounter, 3U);
    }

    void testSkipCode() {
        program->writeCode(0x11111111U, programCounter);
        program->writeCode(0x22222222U, programCounter);
        program->writeCode(0x33333333U, programCounter);
        program->writeCode(0x44444444U, programCounter);
        programCounter = 2;
        program->skipCode(programCounter);
        REQUIRE_EQUAL(programCounter, 3U);
    }

    void testReserve() {
        program->reserve(100);
        REQUIRE_EQUAL(program->size(), 0U);
    }

    void testClear() {
        program->writeCode(0x11111111U, programCounter);
        program->clear();
        REQUIRE_EQUAL(program->size(), 0U);
    }

    void testErrors() {
        // empty, read is not possible.
        REQUIRE_THROWS(program->readCode(programCounter));
        program->writeCode(0x11111111U, programCounter);
        REQUIRE_THROWS(program->readCode(programCounter));
        programCounter = 0;
        REQUIRE_NOTHROW(program->readCode(programCounter));
        REQUIRE_THROWS(program->readCode(programCounter));
        programCounter = 100;
        REQUIRE_THROWS(program->writeCode(0x22222222U, programCounter));
    }
};
