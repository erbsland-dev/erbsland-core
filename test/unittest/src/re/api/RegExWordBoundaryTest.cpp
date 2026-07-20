// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

#include <erbsland/re/StdFormatForRegEx.hpp>
#include <erbsland/unittest/FileHelper.hpp>

using namespace el::re;
using namespace el::unittest::fh;

TESTED_TARGETS(RegEx)
TAGS(Api)
class RegExWordBoundaryTest final : public UNITTEST_SUBCLASS(RegExBase) {
    static auto countFindAll(const RegEx &re, const String &text) -> std::size_t {
        std::size_t count = 0;
        for (const auto &match : re.findAll(text)) {
            static_cast<void>(match);
            count += 1;
        }
        return count;
    }

    static auto makeRepeatedWords(const std::size_t wordCount) -> StringEditor {
        auto text = StringEditor{};
        text.reserve(el::unit::ByteLength{wordCount * 5U});
        for (std::size_t i = 0; i < wordCount; ++i) {
            text.append("word"_el);
            text.append(" "_el);
        }
        return text;
    }

public:
    TESTED_TARGETS(findFirst)
    void testFindFirstWordBoundariesInShortText() {
        requireCompile(R"(\b)"_el);
        WITH_CONTEXT(requireFindFirstNoCaptures("ab cd"_el, 0, ""_el));

        requireCompile(R"(\B)"_el);
        WITH_CONTEXT(requireFindFirstNoCaptures("ab cd"_el, 1, ""_el));
    }

    TESTED_TARGETS(findAll)
    void testFindAllWordBoundariesInShortText() {
        requireCompile(R"(\b)"_el);
        WITH_CONTEXT(requireFindAll("ab cd"_el));
        const auto expectedWordBoundaryLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0000 ''",
            "Match 02:",
            "00: 0002-0002 ''",
            "Match 03:",
            "00: 0003-0003 ''",
            "Match 04:",
            "00: 0005-0005 ''",
        };
        WITH_CONTEXT(requireLines(matchLines, expectedWordBoundaryLines));

        requireCompile(R"(\B)"_el);
        WITH_CONTEXT(requireFindAll("ab cd"_el));
        const auto expectedNonWordBoundaryLines = std::vector<std::string>{
            "Match 01:",
            "00: 0001-0001 ''",
            "Match 02:",
            "00: 0004-0004 ''",
        };
        WITH_CONTEXT(requireLines(matchLines, expectedNonWordBoundaryLines));
    }

    TESTED_TARGETS(findAll)
    void testFindAllWordsInLongSyntheticText() {
        const auto text = makeRepeatedWords(10000);

        requireCompile(R"(\w+)"_el);
        const auto wordCountByW = countFindAll(*regex, text);
        REQUIRE_EQUAL(wordCountByW, 10000);

        requireCompile(R"(\b\w+\b)"_el);
        const auto wordCountByBoundaries = countFindAll(*regex, text);
        REQUIRE_EQUAL(wordCountByBoundaries, 10000);
    }
};
