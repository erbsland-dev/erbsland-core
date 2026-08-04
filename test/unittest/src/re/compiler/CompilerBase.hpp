// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/impl/compiler/Compiler.hpp>
#include <erbsland/re/impl/diagnostics/Assembler.hpp>
#include <erbsland/re/impl/diagnostics/Disassembler.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using impl::Compiler;
using impl::Disassembler;
using impl::DisassemblerFlag;
using impl::EnginePtr;
using impl::GroupFlag;
using impl::GroupFlags;

/// Shared fixture for regular-expression compiler unit tests.
/// @notest{Used only by compiler unit tests.}
class CompilerBase : public re_test::TestHelper {
public:
    String pattern;
    GroupFlags groupFlags;
    EnginePtr engine;
    el::text::StringList lines; // disassembly lines

    auto additionalErrorMessages() -> std::string override {
        try {
            std::string result;
            result += std::format("pattern: \"{}\"\n", pattern.toSafeString(el::unit::CpLength{200U}));
            result += std::format("groupFlags: \"{}\"\n", groupFlags.toString());
            return result;
        } catch (...) {
            return "Unexpected exception during generating additional error messages.";
        }
    }

    void setUp() override { engine = {}; }

    /// Compile a pattern and generate its disassembly.
    void compileAndDisassemble(
        const String &patternView, const GroupFlags flags = GroupFlags{}, const Settings &settings = Settings{}) {

        pattern = patternView;
        groupFlags = flags;
        Compiler compiler{el::text::StringCharReader{pattern}, flags, settings};
        engine = compiler.buildEngine();
        Disassembler disassembler{engine->data(), DisassemblerFlag::TestOutput};
        lines = disassembler.disassemble();
    }
};
