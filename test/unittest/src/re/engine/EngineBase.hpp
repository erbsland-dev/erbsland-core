// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MockStringInput.hpp"

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/impl/diagnostics/Assembler.hpp>
#include <erbsland/re/impl/diagnostics/Disassembler.hpp>
#include <erbsland/re/impl/engine/Engine.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using impl::Assembler;
using impl::CharClassData;
using impl::ConstEngineDataPtr;
using impl::Disassembler;
using impl::Engine;
using impl::EngineHasMatch;
using impl::EnginePtr;
using impl::EngineState;
using impl::EngineStatePtr;
using impl::ProgramPtr;
using impl::SequenceData;

/// Provides shared setup, execution, and assertions for regular-expression engine tests.
/// @notest{This helper is exercised by its derived engine test suites.}
class EngineBase : public re_test::TestHelper {
public:
    ConstEngineDataPtr engineData;
    EnginePtr engine;
    EngineStatePtr state;
    MockStringInputPtr input;
    EngineHasMatch hasMatch;
    CaptureGroupList captureGroups;
    std::vector<std::string> assemblerListing;
    StringEditor text;

    /// Create diagnostic lines for the captured groups.
    auto createGroupLines() noexcept -> std::vector<std::string> {
        std::vector<std::string> result;
        for (std::size_t i = 0; i < captureGroups.size(); ++i) {
            const auto &group = captureGroups[i];
            auto line = std::format("{:02}: {:04}-{:04} '", i, group.begin(), group.end());
            if (group.begin() <= text.length().toSizeT() && group.end() <= text.length().toSizeT() &&
                group.begin() <= group.end()) {

                const auto groupText = String{text}.slice(
                    el::unit::ByteRange{el::unit::ByteIndex{group.begin()}, el::unit::ByteIndex{group.end()}});
                line += re_test::string_helper::toStdString(
                    groupText.toSafeString(el::unit::CpLength{200U}, el::text::SafeStringFlag::None));
            } else {
                line += "<invalid range>";
            }
            line += '\'';
            result.emplace_back(line);
        }
        return result;
    }

    auto additionalErrorMessages() -> std::string override {
        try {
            std::string msg;
            msg += std::format("text = \"{}\"\n", text.toSafeString(el::unit::CpLength{200U}));
            if (engineData != nullptr) {
                msg += "engineData =\n";
                Disassembler disassembler{engineData};
                for (const auto &line : disassembler.disassemble()) {
                    msg += re_test::string_helper::toStdString(line);
                    msg += '\n';
                }
            } else {
                msg += "engineData = null\n";
                if (!assemblerListing.empty()) {
                    msg += "assemblerListing =\n";
                    for (const auto &line : assemblerListing) {
                        msg += line;
                        msg += '\n';
                    }
                }
            }
            msg += std::format("hasMatch = {}\n", hasMatch == EngineHasMatch::Yes ? "Yes" : "No");
            if (!captureGroups.empty()) {
                msg += "captureGroups = \n";
                for (const auto &line : createGroupLines()) {
                    msg += line;
                    msg += '\n';
                }
            }
            return msg;
        } catch (...) {
            return "Unexpected exception while providing additional error messages";
        }
    }

    /// Assemble an engine program from Core string lines.
    void assembleProgram(const el::text::StringList &lines) {
        assemblerListing = {};
        auto lineNumber = std::size_t{1};
        for (const auto &line : lines) {
            assemblerListing.emplace_back(std::format("{:3}: {}", lineNumber, line));
            ++lineNumber;
        }
        Assembler assembler;
        REQUIRE_NOTHROW(engineData = assembler.compile(lines));
        engine = Engine::create(engineData);
    }

    /// Assemble an engine program from standard string-view lines.
    void assembleProgram(const std::initializer_list<std::string_view> lines) {
        assembleProgram(re_test::string_helper::toStringList(lines));
    }

    void setUp() override {
        engineData = {};
        engine = {};
        state = std::make_unique<EngineState>();
        input = {};
        hasMatch = EngineHasMatch::No;
        captureGroups.clear();
        text.clear();
    }

    /// Run a prefix match against test input.
    void runEngineMatch(const String &textToMatch) {
        text = StringEditor{textToMatch};
        input = std::make_shared<MockStringInput>(text);
        state = engine->createState(input);
        hasMatch = engine->match(*state);
        if (hasMatch == EngineHasMatch::Yes) {
            captureGroups = state->createCaptureGroups(engineData->captureGroupNames);
        } else {
            captureGroups.clear();
        }
    }

    /// Run a full match against test input.
    void runEngineFullMatch(const String &textToMatch) {
        text = StringEditor{textToMatch};
        input = std::make_shared<MockStringInput>(text);
        state = engine->createState(input);
        hasMatch = engine->fullMatch(*state);
        if (hasMatch == EngineHasMatch::Yes) {
            captureGroups = state->createCaptureGroups(engineData->captureGroupNames);
        } else {
            captureGroups.clear();
        }
    }

    /// Run a first-match search against test input.
    void runEngineFindFirst(const String &textToMatch) {
        text = StringEditor{textToMatch};
        input = std::make_shared<MockStringInput>(text);
        state = engine->createState(input);
        hasMatch = engine->findFirst(*state);
        if (hasMatch == EngineHasMatch::Yes) {
            captureGroups = state->createCaptureGroups(engineData->captureGroupNames);
        } else {
            captureGroups.clear();
        }
    }

    /// require no match with a pattern that has zero capture groups.
    void requireNoMatch(const String &textToMatch) {
        runEngineMatch(textToMatch);
        REQUIRE_EQUAL(hasMatch, EngineHasMatch::No);
    }

    /// require a match with the given text and no capture groups.
    void requireMatch(const String &textToMatch, const std::optional<std::size_t> matchEnd = std::nullopt) {
        runEngineMatch(textToMatch);
        REQUIRE_EQUAL(hasMatch, EngineHasMatch::Yes);
        REQUIRE_GREATER_EQUAL(captureGroups.size(), 1);
        REQUIRE_EQUAL(captureGroups[0].begin(), 0);
        if (matchEnd.has_value()) {
            REQUIRE_EQUAL(captureGroups[0].end(), matchEnd);
        }
    }

    /// require no match with a pattern that has zero capture groups.
    void requireNoFullMatch(const String &textToMatch) {
        runEngineFullMatch(textToMatch);
        REQUIRE_EQUAL(hasMatch, EngineHasMatch::No);
    }

    /// require a match with the given text and no capture groups.
    void requireFullMatch(const String &textToMatch, const std::optional<std::size_t> matchEnd = std::nullopt) {
        runEngineFullMatch(textToMatch);
        REQUIRE_EQUAL(hasMatch, EngineHasMatch::Yes);
        REQUIRE_GREATER_EQUAL(captureGroups.size(), 1);
        REQUIRE_EQUAL(captureGroups[0].begin(), 0);
        if (matchEnd.has_value()) {
            REQUIRE_EQUAL(captureGroups[0].end(), matchEnd);
        }
    }

    /// Require that a first-match search finds no match.
    void requireNoFindFirst(const String &textToMatch) {
        runEngineFindFirst(textToMatch);
        REQUIRE_EQUAL(hasMatch, EngineHasMatch::No);
    }

    /// Require that a first-match search finds the expected match range.
    void requireFindFirst(const String &textToMatch, const std::optional<CaptureRange> &matchRange = std::nullopt) {
        runEngineFindFirst(textToMatch);
        REQUIRE_EQUAL(hasMatch, EngineHasMatch::Yes);
        REQUIRE_GREATER_EQUAL(captureGroups.size(), 1);
        if (matchRange.has_value()) {
            REQUIRE_EQUAL(captureGroups[0].range(), matchRange.value());
        }
    }

    /// require an exception.
    void requireErrorException(const String &textToMatch, ErrorCategory expectedCategory) {
        try {
            runEngineMatch(textToMatch);
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), expectedCategory);
        }
    }

    /// Require captured-group diagnostics to match expected lines.
    void requireGroups(const std::vector<std::string> &expectedGroups) {
        const auto actualGroups = createGroupLines();
        WITH_CONTEXT(requireLines(actualGroups, expectedGroups));
    }
};
