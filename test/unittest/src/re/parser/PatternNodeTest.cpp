// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/impl/parser/PatternNode.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>

using namespace el::re;
using namespace el::text::literals;
using el::text::Char;
using impl::CharClass;
using impl::CharRange;
using impl::PatternNode;
using impl::PatternNodePtr;
using namespace impl::node_data;

TESTED_TARGETS(PatternNode)
TAGS(Parsing)
class patternNodeTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    PatternNodePtr node;

    void testSequence() {
        node = std::make_shared<PatternNode>(0, Sequence{});
        REQUIRE(node->isSequence());
        REQUIRE_FALSE(node->isGroup());
        REQUIRE_FALSE(node->isCharacterSequence());
        REQUIRE_FALSE(node->isCharacterClass());
        REQUIRE_EQUAL(node->toTestString(), "Sequence(size=0)"_el);

        REQUIRE(node->isEmpty());
        node->addChild(std::make_shared<PatternNode>(1, Sequence{}));
        node->addChild(std::make_shared<PatternNode>(2, Sequence{}));
        node->addChild(std::make_shared<PatternNode>(3, Group{}));
        REQUIRE_THROWS_AS(RegExError, node->addCharacter(Char{U'a'}));
        REQUIRE_EQUAL(node->size(), 3U);
        REQUIRE(node->children()[0]->isSequence());
        REQUIRE(node->children()[1]->isSequence());
        REQUIRE(node->children()[2]->isGroup());

        const auto actual = node->toTestTree();
        const auto expected = std::vector<std::string_view>{
            "Sequence(size=3)",
            "  Sequence(size=0)",
            "  Sequence(size=0)",
            "  Group(size=0)",
        };
        WITH_CONTEXT(requireLines(actual, expected))
    }

    void testCharacterSequence() {
        node = std::make_shared<PatternNode>(0, CharacterSequence{});
        REQUIRE(node->isCharacterSequence());
        REQUIRE_FALSE(node->isGroup());
        REQUIRE_FALSE(node->isSequence());
        REQUIRE_FALSE(node->isCharacterClass());
        REQUIRE_EQUAL(node->toTestString(), "CharacterSequence(\"\")"_el);
        REQUIRE(node->isEmpty());
        REQUIRE_THROWS_AS(RegExError, node->addChild(std::make_shared<PatternNode>(1, Sequence{})));

        node->addCharacter(Char{U'a'});
        REQUIRE_EQUAL(node->toTestString(), "CharacterSequence(\"a\")"_el);
        node->addCharacter(Char{U'😄'});
        REQUIRE_EQUAL(node->toTestString(), "CharacterSequence(\"a😄\")"_el);
        node->addCharacter(Char{U'\n'});
        REQUIRE_EQUAL(node->toTestString(), "CharacterSequence(\"a😄\\u000A\")"_el);
    }

    void testCharacterClass() {
        auto charClass = CharClass{std::vector{CharRange{U'g', U'g'}, CharRange{U'A', U'Z'}}};
        charClass.prepareForUse();
        node = std::make_shared<PatternNode>(0, CharacterClass{std::move(charClass), false});
        REQUIRE(node->isCharacterClass());
        REQUIRE_FALSE(node->isGroup());
        REQUIRE_FALSE(node->isSequence());
        REQUIRE_FALSE(node->isCharacterSequence());
        REQUIRE_EQUAL(node->toTestString(), "CharacterClass([A-Zg])"_el);
        auto charClass2 = CharClass{std::vector{
            CharRange{U'😀', U'😄'},
        }};
        charClass2.prepareForUse();
        node = std::make_shared<PatternNode>(0, CharacterClass{std::move(charClass2), true});
        REQUIRE_EQUAL(node->toTestString(), "CharacterClass([^\\u{1F600}-\\u{1F604}])"_el);
    }

    void testTraverseEnterExit() {
        node = std::make_shared<PatternNode>(0, Sequence{});

        auto group = std::make_shared<PatternNode>(1, Group{});
        auto seq = std::make_shared<PatternNode>(2, Sequence{});
        seq->addChild(std::make_shared<PatternNode>(3, CharacterSequence{}));
        group->addChild(seq);
        group->addChild(std::make_shared<PatternNode>(4, CharacterSequence{}));

        node->addChild(group);
        node->addChild(std::make_shared<PatternNode>(5, CharacterSequence{}));

        std::vector<std::string> enter;
        std::vector<std::string> exit;

        node->traverse(
            [&enter](const PatternNode &n, const int depth) -> void {
                enter.push_back(std::format("{}:{}", n.id(), depth));
            },
            [&exit](const PatternNode &n, const int depth) -> void {
                exit.push_back(std::format("{}:{}", n.id(), depth));
            });

        const auto expectedEnter = std::vector<std::string_view>{
            "0:0",
            "1:1",
            "2:2",
            "3:3",
            "4:2",
            "5:1",
        };
        const auto expectedExit = std::vector<std::string_view>{
            "3:3",
            "2:2",
            "4:2",
            "1:1",
            "5:1",
            "0:0",
        };

        WITH_CONTEXT(requireLines(enter, expectedEnter))
        WITH_CONTEXT(requireLines(exit, expectedExit))
    }
};
