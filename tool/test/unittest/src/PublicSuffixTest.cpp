// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/StringJoins.hpp>
#include <tool/psl/EncodedData.hpp>
#include <tool/psl/HuffmanTree.hpp>

TESTED_TARGETS(EncodedData HuffmanTree)
class PublicSuffixTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testSingletonAndBlockBoundaries() {
        const auto singleton = tool::psl::EncodedData{el::List<tool::psl::Rule>{{"a"_el, 0U}}};
        singleton.verify();
        REQUIRE_EQUAL(singleton.prefixTree().length().toSizeT(), 2U);
        REQUIRE_EQUAL(singleton.blockOffsets().count().toSizeT(), 2U);
        for (const auto count : {31U, 32U, 33U, 64U, 65U}) {
            auto rules = el::List<tool::psl::Rule>{};
            for (auto index = 0U; index < count; ++index) {
                rules.append(tool::psl::Rule{el::StringFormat{"a{:03}"_el}.build(index), index % 3U});
            }
            const auto first = tool::psl::EncodedData{rules};
            const auto second = tool::psl::EncodedData{rules};
            first.verify();
            REQUIRE_EQUAL(first.encodedRules(), second.encodedRules());
            REQUIRE_EQUAL(first.prefixTree(), second.prefixTree());
            REQUIRE_EQUAL(first.contentTree(), second.contentTree());
            REQUIRE_EQUAL(first.blockOffsets(), second.blockOffsets());
            REQUIRE_EQUAL(first.blockOffsets().count().toSizeT(), (count + 31U) / 32U + 1U);
        }
    }

    void testPrefixesAlphabetAndLengthLimits() {
        // Include a complete preceding rule, a mismatch at its last byte, and no common prefix.
        auto rules = el::List<tool::psl::Rule>{
            {"a"_el, 0U},
            {"aa"_el, 1U},
            {"ab"_el, 2U},
            {"b"_el, 0U},
            {"b-.0123456789abcdefghijklmnopqrstuvwxyz"_el, 1U}};
        const auto data = tool::psl::EncodedData{rules};
        data.verify();
        REQUIRE_EQUAL(data.maximumRuleLength().toSizeT(), 39U);
        REQUIRE_EQUAL(data.maximumLabelCount(), 2U);

        const auto prefix127 = el::String::fromCharacter(U'a', el::CpCount{127});
        const auto prefix128 = el::String::fromCharacter(U'a', el::CpCount{128});
        const auto accepted =
            tool::psl::EncodedData{el::List<tool::psl::Rule>{{prefix127, 0U}, {el::join(prefix127, "b"_el), 1U}}};
        accepted.verify();
        REQUIRE_THROWS(
            tool::psl::EncodedData{el::List<tool::psl::Rule>{{prefix128, 0U}, {el::join(prefix128, "b"_el), 1U}}});

        const auto longest =
            tool::psl::EncodedData{el::List<tool::psl::Rule>{{el::String::fromCharacter(U'a', el::CpCount{255}), 0U}}};
        REQUIRE_EQUAL(longest.maximumRuleLength().toSizeT(), 255U);
        REQUIRE_THROWS(
            tool::psl::EncodedData{el::List<tool::psl::Rule>{{el::String::fromCharacter(U'a', el::CpCount{256}), 0U}}});
        REQUIRE_THROWS(tool::psl::EncodedData{el::List<tool::psl::Rule>{{"é"_el, 0U}}});
    }

    void testRejectedRepresentationsAndCorruption() {
        REQUIRE_THROWS(tool::psl::EncodedData{el::List<tool::psl::Rule>{}});
        REQUIRE_THROWS(tool::psl::EncodedData{el::List<tool::psl::Rule>{{"a"_el, 3U}}});
        REQUIRE_THROWS(tool::psl::EncodedData{el::List<tool::psl::Rule>{{"a"_el, 0U}, {"a"_el, 1U}}});
        REQUIRE_THROWS(tool::psl::EncodedData{el::List<tool::psl::Rule>{{"A"_el, 0U}}});
        auto corrupted = tool::psl::EncodedData{el::List<tool::psl::Rule>{{"abc"_el, 0U}}};
        // Deliberately bypass the read-only accessor to exercise corrupted payload validation.
        const_cast<el::ByteBlock &>(corrupted.encodedRules()) = el::ByteBlock{};
        REQUIRE_THROWS(corrupted.verify());
    }
};
