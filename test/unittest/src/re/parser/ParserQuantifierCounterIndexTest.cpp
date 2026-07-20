// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

#include <erbsland/re/StdFormatForRegEx.hpp>

using impl::limits::maximumCounterCount;

TESTED_TARGETS(Parser)
TAGS(Parsing) class ParserQuantifierCounterIndexTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void requireValidCounterIndexNesting(const PatternNodePtr &root) {
        using impl::node_data::Quantifier;

        REQUIRE(root != nullptr);

        Quantifier::CounterIndex nestingLevel = 0;
        const PatternNode &rootNode = *root;
        const std::function<void(const PatternNode &, int)> onEnter = [this, &nestingLevel](
                                                                          const PatternNode &node, int) {
            if (!node.isQuantifier()) {
                return;
            }
            const auto &q = std::get<Quantifier>(node.data());
            REQUIRE_LESS(q.counterIndex, maximumCounterCount);
            REQUIRE_EQUAL(q.counterIndex, nestingLevel);
            nestingLevel += 1;
        };
        const std::function<void(const PatternNode &, int)> onExit = [this, &nestingLevel](
                                                                         const PatternNode &node, int) {
            if (node.isQuantifier()) {
                nestingLevel -= 1;
            }
        };
        rootNode.traverse(onEnter, onExit);
    }

    void testCounterIndex_AssignedByNestingLevel_NoConflicts() {
        struct TestCase {
            String pattern;
        };

        const std::vector<TestCase> testCases = {
            // No nesting: counters may be reused.
            {"a+b+"_el},

            // Nested quantifiers via groups and sequences.
            {"(a+)+"_el},
            {"(ab*c)+"_el},
            {"((a{2,3})+b?)*"_el},
            {"(a(bc+)?d*)+"_el},
            {"((ab)+|(cd)*)+"_el},
        };

        for (const auto &testCase : testCases) {
            runWithContext(
                SOURCE_LOCATION(),
                [&] {
                    parseTree(testCase.pattern);
                    WITH_CONTEXT(this->requireValidCounterIndexNesting(node));
                },
                [&]() -> std::string {
                    return std::format(
                        "Failed at pattern \"{}\"", testCase.pattern.toSafeString(el::unit::CpLength{200U}));
                });
        }
    }
};
