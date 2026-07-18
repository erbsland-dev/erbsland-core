// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/impl/diagnostics/Disassembler.hpp>
#include <erbsland/re/impl/engine/EngineData.hpp>
#include <erbsland/re/impl/engine/ProgramWriter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using el::text::Char;
using impl::Category;
using impl::CharClass;
using impl::CharRange;
using impl::Disassembler;
using impl::DisassemblerFlag;
using impl::EngineData;
using impl::Operation;
using impl::ProgramCounter;
using impl::ProgramWriter;
using impl::TextAnchor;

TESTED_TARGETS(Disassembler)
TAGS(Diagnostics) class DisassemblerTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    EngineData data;
    el::text::StringList lines;

    void setUp() override {
        data = {};
        lines = {};
    }

    auto additionalErrorMessages() -> std::string override {
        std::string result = "Disassembly:\n";
        for (const auto &line : lines) {
            result += "  ";
            result += re_test::string_helper::toStdString(line);
            result += "\n";
        }
        return result;
    }

    void disassemble() {
        Disassembler disassembler{std::make_shared<EngineData>(data), DisassemblerFlag::TestOutput};
        lines = disassembler.disassemble();
    }

    void testOperations() {
        ProgramCounter pc = 0;
        ProgramWriter writer{data.program, pc};

        writer.writeNone();
        writer.writeMatch();
        writer.writeNotMatch();
        writer.writeSuccess();
        writer.writeFailure();
        writer.writeStartAtomic(1);
        writer.writeStopAtomic(1);
        writer.writeSplit(1, 2);
        writer.writeJump(3);
        writer.writeAnchor(TextAnchor::Start);
        writer.writeStartCapture(5);
        writer.writeStopCapture(5);
        writer.writeCounter(1, 10);
        writer.writeMaximum(1, 20);
        writer.writeMinimum(1, 5);
        writer.writeChar(Char('a'));
        writer.writeCiChar(Char('b'));
        writer.writeNotChar(Char('c'));
        writer.writeNotCiChar(Char('d'));
        writer.writeSequence(100, 10);
        writer.writeCiSequence(200, 20);
        writer.writeCategory(Category::DigitAscii);
        writer.writeNotCategory(Category::WordAscii);
        writer.writeClass(1);
        writer.writeCiClass(2);
        writer.writeNotClass(3);
        writer.writeNotCiClass(4);
        writer.writeAny();

        disassemble();

        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NONE",
                "$0001: MATCH",
                "$0002: NOT MATCH",
                "$0003: SUCCESS",
                "$0004: FAILURE",
                "$0005: START ATOMIC 1",
                "$0006: STOP ATOMIC 1",
                "$0007: SPLIT $0001, $0002",
                "$0009: JUMP $0003",
                "$000A: ANCHOR &Start",
                "$000B: START CAPTURE 5",
                "$000C: STOP CAPTURE 5",
                "$000D: COUNTER 1, 10",
                "$000E: MAXIMUM 1, 20",
                "$000F: MINIMUM 1, 5",
                "$0010: CHAR 'a'",
                "$0011: CI CHAR 'b'",
                "$0012: NOT CHAR 'c'",
                "$0013: NOT CI CHAR 'd'",
                "$0014: SEQUENCE $0064, $0a",
                "$0015: CI SEQUENCE $00c8, $14",
                "$0016: CATEGORY &DigitAscii",
                "$0017: NOT CATEGORY &WordAscii",
                "$0018: CLASS $0001",
                "$0019: CI CLASS $0002",
                "$001A: NOT CLASS $0003",
                "$001B: NOT CI CLASS $0004",
                "$001C: ANY",
            }));
    }

    void testSequenceData() {
        data.sequenceData = {Char('H'), Char('e'), Char('l'), Char('l'), Char('o')};

        disassemble();

        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                ".section &sequence",
                "$0000: .data \"Hello\"",
                ".section &program",
            }));
    }

    void testCharClassData() {
        CharClass class1;
        class1.add(Char('a'), Char('z'));
        class1.add(Char('0'), Char('9'));
        class1.prepareForUse();

        CharClass class2;
        class2.add(Char('A'), Char('Z'));
        class2.prepareForUse();

        data.charClassData = {class1, class2};

        disassemble();

        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                ".section &class",
                "$0000: .class",
                "$0000: .data $000030-$000039",
                "$0000: .data $000061-$00007A",
                "$0001: .class",
                "$0001: .data $000041-$00005A",
                ".section &program",
            }));
    }

    void testCombined() {
        ProgramCounter pc = 0;
        ProgramWriter writer{data.program, pc};

        data.sequenceData = {Char('a'), Char('b'), Char('c')};

        CharClass class1;
        class1.add(Char('0'), Char('9'));
        class1.prepareForUse();
        data.charClassData = {class1};

        writer.writeSequence(0, 3);
        writer.writeClass(0);
        writer.writeMatch();

        disassemble();

        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                ".section &sequence",
                "$0000: .data \"abc\"",
                ".section &class",
                "$0000: .class",
                "$0000: .data $000030-$000039",
                ".section &program",
                "$0000: SEQUENCE $0000, $03",
                "$0001: CLASS $0000",
                "$0002: MATCH",
            }));
    }
};
