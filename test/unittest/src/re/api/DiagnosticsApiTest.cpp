// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/diagnostics/Assembler.hpp>
#include <erbsland/re/diagnostics/Disassembler.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using namespace el::re::diagnostics;

TESTED_TARGETS(Assembler Disassembler)
TAGS(Api Diagnostics)
class DiagnosticsApiTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    void testAssemblerCompile() {
        Assembler assembler;
        auto regEx = assembler.compile(el::text::StringList{"None"_el, "Match"_el});
        REQUIRE(regEx);
    }

    void testAssemblerError() {
        Assembler assembler;
        REQUIRE_THROWS_AS(el::re::RegExError, assembler.compile(el::text::StringList{"INVALID_OP"_el}));
    }

    void testDisassembler() {
        auto regEx = RegEx::compile("a|b"_el);
        REQUIRE(regEx);

        Disassembler disassembler{regEx};
        auto lines = disassembler.disassemble();

        REQUIRE_FALSE(lines.isEmpty());

        // Basic check if it contains something expected
        bool foundMatch = false;
        for (const auto &line : lines) {
            if (line.contains("MATCH"_el)) {
                foundMatch = true;
                break;
            }
        }
        REQUIRE(foundMatch);
    }

    void testAssemblerAndDisassembler() {
        Assembler assembler;
        auto regEx = assembler.compile(el::text::StringList{"Char 'x'"_el, "Match"_el});
        REQUIRE(regEx);

        Disassembler disassembler{regEx};
        auto lines = disassembler.disassemble();

        REQUIRE_FALSE(lines.isEmpty());

        bool foundChar = false;
        for (const auto &line : lines) {
            if (line.contains("CHAR 'x'"_el)) {
                foundChar = true;
                break;
            }
        }
        REQUIRE(foundChar);
    }
};
