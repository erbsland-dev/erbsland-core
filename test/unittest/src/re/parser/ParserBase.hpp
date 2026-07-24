// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/impl/parser/Parser.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <regex>

using namespace el::re;
using impl::GroupFlag;
using impl::GroupFlags;
using impl::Parser;
using impl::PatternNode;
using impl::PatternNodePtr;

/// Shared base class for parser tests.
class ParserBase : public re_test::TestHelper {
public:
    Parser parser;             ///< The last parser used.
    PatternNodePtr node;       ///< The last root node from the parser.
    el::text::StringList tree; ///< last parsed tree for diagnostics
    String lastPattern;        ///< last parsed pattern for diagnostics

    struct TestCase {
        String pattern;
        std::vector<std::string_view> expected;
    };
    using TestCases = std::vector<TestCase>;

    auto additionalErrorMessages() -> std::string override {
        std::string msg;
        try {
            if (!lastPattern.isEmpty()) {
                msg += std::format("pattern: \"{}\"\n", lastPattern.toSafeString(el::unit::CpLength{200U}));
            }
            if (!tree.isEmpty()) {
                msg += "parse tree:\n";
                for (const auto &line : tree) {
                    msg += re_test::string_helper::toStdString(line);
                    msg += '\n';
                }
            }
        } catch (...) {
            msg += "(unexpected exception while formatting diagnostics)";
        }
        return msg;
    }

    void setUp() override {
        parser = {};
        node = {};
        tree.clear();
        lastPattern = {};
    }

    /// Parse and remember the resulting tree.
    /// @param pattern The tested pattern.
    /// @param flags Optional flags
    /// @param settings Optional settings
    auto parseTree(const String &pattern, const GroupFlags flags = {}, const Settings &settings = {}) {

        lastPattern = pattern;
        parser = Parser{pattern, flags, settings};
        REQUIRE_NOTHROW(node = parser.parse());
        tree = node->toTestTree();
    }

    /// Parse and test a pattern against expected results
    /// @param pattern The tested pattern.
    /// @param expected The expected parsed tree.
    /// @param flags Optional flags.
    /// @param settings Optional settings.
    void parseAndTest(
        const String &pattern,
        const std::vector<std::string_view> expected,
        const GroupFlags flags = {},
        const Settings &settings = {}) {

        parseTree(pattern, flags, settings);
        WITH_CONTEXT(requireLines(tree, expected))
    }

    /// Verify a prefix of exact lines, then one of several acceptable alternatives, and finally an optional trail.
    /// Each entry can contain *one* '*', and multiple '?', that matches any character (classic glob).
    ///
    /// @param pattern The tested pattern.
    /// @param prefix The initial lines of the tree.
    /// @param alternatives A number of alternatives.
    /// @param trail An optional trail that must match the end of the tree.
    void parseAndTest(
        const String &pattern,
        const std::vector<std::string_view> &prefix,
        const std::vector<std::string_view> &alternatives,
        const std::vector<std::string_view> &trail = {},
        const GroupFlags flags = {}) {
        parseTree(pattern, flags);

        // Check prefix
        const auto treeSize = tree.count().toSizeT();
        REQUIRE_GREATER_EQUAL(treeSize, prefix.size());
        for (std::size_t i = 0; i < prefix.size(); ++i) {
            REQUIRE(compareWithStar(prefix[i], tree[el::unit::ElementIndex::fromSizeT(i)]));
        }

        // Check alternative next line
        const std::size_t altIndex = prefix.size();
        REQUIRE_GREATER(treeSize, altIndex);
        bool matchedAlt = false;
        for (const auto &alt : alternatives) {
            if (compareWithStar(alt, tree[el::unit::ElementIndex::fromSizeT(altIndex)])) {
                matchedAlt = true;
                break;
            }
        }
        REQUIRE(matchedAlt);

        // Optional trail at the end of the tree
        if (!trail.empty()) {
            REQUIRE_GREATER_EQUAL(treeSize, prefix.size() + trail.size() + 1);
            const std::size_t start = treeSize - trail.size();
            for (std::size_t i = 0; i < trail.size(); ++i) {
                REQUIRE(compareWithStar(trail[i], tree[el::unit::ElementIndex::fromSizeT(start + i)]));
            }
        }
    }

    /// Test a list of patterns against expected results.
    /// @param testCases A list of test cases.
    void requireTestCases(const TestCases &testCases) {
        for (const auto &[pattern, expected] : testCases) {
            runWithContext(
                SOURCE_LOCATION(),
                [&] { parseAndTest(pattern, expected); },
                [&]() -> std::string {
                    std::string message;
                    message += std::format("Failed at pattern \"{}\"", pattern.toSafeString(el::unit::CpLength{200U}));
                    return message;
                });
        }
    }

    /// Parse and expect an error.
    /// @param pattern The tested pattern.
    /// @param expectedError The expected error message.
    /// @param expectedIndex The expected character index of the error.
    /// @param settings Optional settings.
    void parseAndExpectError(
        const String &pattern,
        const std::string_view expectedError,
        const std::size_t expectedIndex,
        const Settings &settings = {}) {

        lastPattern = pattern;
        parser = Parser{pattern, {}, settings};

        try {
            (void)parser.parse();
            REQUIRE(false); // Expected an error
        } catch (const RegExError &e) {
            CHECK_EQUAL(e.title(), "Failed to parse regular expression"_el);
            CHECK_EQUAL(e.description(), String{expectedError});
            CHECK_EQUAL(e.position().toSizeT(), expectedIndex);
        }
    }
};
