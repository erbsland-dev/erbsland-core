// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../TestHelper.hpp"

#include <erbsland/re/impl/compiler/DataCollector.hpp>
#include <erbsland/re/impl/Limits.hpp>
#include <erbsland/re/impl/parser/PatternNode.hpp>

using namespace el::re;
using el::text::Char;
using impl::CharClass;
using impl::CharRange;
using impl::EngineData;
using impl::PatternNode;
using impl::PatternNodeId;
using impl::PatternNodePtr;
using namespace impl::node_data;

static auto nextTestNodeId() -> PatternNodeId {
    static PatternNodeId id{0};
    id += 1;
    return id;
}

static auto makeCharSeqNode(std::u32string_view sv) -> PatternNodePtr {
    auto node = std::make_shared<PatternNode>(nextTestNodeId(), CharacterSequence{});
    for (char32_t ch : sv) {
        node->addCharacter(Char{ch});
    }
    return node;
}

static auto makeRangesNode(const CharClass &ranges, bool isNegated) -> PatternNodePtr {
    return std::make_shared<PatternNode>(nextTestNodeId(), CharacterClass{ranges, isNegated});
}

TESTED_TARGETS(DataCollector)
TAGS(Compilation) class DataCollectorTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    void testCharacterSequenceCollection() {
        // Build a deeply nested tree with sequences of various lengths and duplicates
        auto root = std::make_shared<PatternNode>(nextTestNodeId(), Group{});

        auto seq1 = std::make_shared<PatternNode>(nextTestNodeId(), Sequence{});
        auto n_abcd = makeCharSeqNode(U"abcd"); // len 4 -> collected
        auto n_hi = makeCharSeqNode(U"hi");     // len 2 -> ignored
        seq1->addChild(n_abcd);
        seq1->addChild(n_hi);

        auto grp2 = std::make_shared<PatternNode>(nextTestNodeId(), Group{});
        auto seq2 = std::make_shared<PatternNode>(nextTestNodeId(), Sequence{});
        auto n_abcd_dup = makeCharSeqNode(U"abcd"); // duplicate -> folded to index of first
        seq2->addChild(n_abcd_dup);
        grp2->addChild(seq2);
        seq1->addChild(grp2);

        auto seq3 = std::make_shared<PatternNode>(nextTestNodeId(), Sequence{});
        auto n_xyz12 = makeCharSeqNode(U"xyz12"); // len 5 -> collected
        auto n_wxyz = makeCharSeqNode(U"wxyz");   // len 4 -> collected
        seq3->addChild(n_xyz12);
        seq3->addChild(n_wxyz);

        root->addChild(seq1);
        root->addChild(seq3);

        const auto engineData = std::make_shared<EngineData>();
        impl::DataCollector collector{root, engineData};
        collector.collect();

        // Validate collected data buffer
        const auto &data = engineData->sequenceData;
        // expected order: "abcd" + "xyz12" + "wxyz" (duplicate "abcd" is folded, "hi" ignored)
        REQUIRE_EQUAL(data.size(), std::size_t{4 + 5 + 4});
        REQUIRE(data[0] == Char{U'a'});
        REQUIRE(data[1] == Char{U'b'});
        REQUIRE(data[2] == Char{U'c'});
        REQUIRE(data[3] == Char{U'd'});
        REQUIRE(data[4] == Char{U'x'});
        REQUIRE(data[5] == Char{U'y'});
        REQUIRE(data[6] == Char{U'z'});
        REQUIRE(data[7] == Char{U'1'});
        REQUIRE(data[8] == Char{U'2'});
        REQUIRE(data[9] == Char{U'w'});
        REQUIRE(data[10] == Char{U'x'});
        REQUIRE(data[11] == Char{U'y'});
        REQUIRE(data[12] == Char{U'z'});

        // Validate indices on nodes
        const auto maxShort = impl::limits::minimumCharacterSequenceLength; // 3
        REQUIRE_EQUAL(maxShort, std::size_t{3});

        // "abcd" first occurrence starts at index 0
        REQUIRE_EQUAL(std::get<CharacterSequence>(n_abcd->data()).dataIndex, 0);
        // "hi" ignored (short)
        REQUIRE_EQUAL(std::get<CharacterSequence>(n_hi->data()).dataIndex, -1);
        // duplicate of "abcd" folded to index 0
        REQUIRE_EQUAL(std::get<CharacterSequence>(n_abcd_dup->data()).dataIndex, 0);
        // "xyz12" starts at index 4
        REQUIRE_EQUAL(std::get<CharacterSequence>(n_xyz12->data()).dataIndex, 4);
        // "wxyz" starts at index 9
        REQUIRE_EQUAL(std::get<CharacterSequence>(n_wxyz->data()).dataIndex, 9);
    }

    void testCharacterClassCollection() {
        // Prepare ranges
        CharClass r1 = CharClass::createPrepared(std::vector{CharRange{U'A', U'Z'}});
        CharClass r2 = CharClass::createPrepared(std::vector{CharRange{U'a', U'c'}, CharRange{U'e', U'e'}});
        CharClass r4 = CharClass::createPrepared(std::vector{CharRange{U'0', U'9'}});

        // Build a deeply nested tree with ranges and duplicates/mixed negation
        auto root = std::make_shared<PatternNode>(nextTestNodeId(), Group{});

        auto seq1 = std::make_shared<PatternNode>(nextTestNodeId(), Sequence{});
        auto n_r1 = makeRangesNode(r1, false); // index 0
        auto n_r2 = makeRangesNode(r2, false); // index 1
        seq1->addChild(n_r1);
        seq1->addChild(n_r2);

        auto grp2 = std::make_shared<PatternNode>(nextTestNodeId(), Group{});
        auto seq2 = std::make_shared<PatternNode>(nextTestNodeId(), Sequence{});
        auto n_r1_dup_neg = makeRangesNode(r1, true); // duplicate ranges, different negation -> still folded
        seq2->addChild(n_r1_dup_neg);
        grp2->addChild(seq2);
        seq1->addChild(grp2);

        auto seq3 = std::make_shared<PatternNode>(nextTestNodeId(), Sequence{});
        auto n_r4 = makeRangesNode(r4, false);     // index 2
        auto n_r2_dup = makeRangesNode(r2, false); // duplicate -> index 1
        seq3->addChild(n_r4);
        seq3->addChild(n_r2_dup);

        root->addChild(seq1);
        root->addChild(seq3);

        const auto engineData = std::make_shared<EngineData>();
        impl::DataCollector collector{root, engineData};
        collector.collect();

        const auto &rangesData = engineData->charClassData;
        REQUIRE_EQUAL(rangesData.size(), std::size_t{3});
        REQUIRE(rangesData[0] == r1);
        REQUIRE(rangesData[1] == r2);
        REQUIRE(rangesData[2] == r4);

        // Validate indices on nodes
        REQUIRE_EQUAL(std::get<CharacterClass>(n_r1->data()).dataIndex, 0);
        REQUIRE_EQUAL(std::get<CharacterClass>(n_r2->data()).dataIndex, 1);
        REQUIRE_EQUAL(std::get<CharacterClass>(n_r1_dup_neg->data()).dataIndex, 0);
        REQUIRE_EQUAL(std::get<CharacterClass>(n_r4->data()).dataIndex, 2);
        REQUIRE_EQUAL(std::get<CharacterClass>(n_r2_dup->data()).dataIndex, 1);
    }
};
