// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <tool/idna/IdnaTool.hpp>

TESTED_TARGETS(IdnaData IdnaTool UnicodeSource)
class IdnaDataFullTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRangeAndPageSerialization() {
        _root = tool::frame::ProjectRoot{el::Path::currentDirectory(), TestToolApplication::coreProject()}.discover();
        runWithSources(el::StringList{tool::unicode::UnicodeSource::sourceIdentifier()}, [&] {
            const auto &data = tool::frame::app().source<tool::unicode::UnicodeSource>().idnaData();
            const auto ranges = data.ranges();
            const auto offsets = tool::idna::IdnaData::pageOffsets(ranges);
            REQUIRE_FALSE(ranges.isEmpty());
            REQUIRE_EQUAL(offsets.first(), el::ItemIndex::zero());
            REQUIRE_EQUAL(offsets.last(), el::ItemIndex::end(ranges.count()));
            auto restored = el::List<unsigned>{el::ItemCount{tool::idna::IdnaData::cCodePointCount}, 0U};
            auto previousEnd = 0U;
            for (
                auto page = el::ItemIndex::zero(); page.isWithin(offsets.count() - el::ItemCount::one());
                page.uncheckedIncrement()) {
                const auto first = offsets.get(page);
                const auto end = offsets.get(page.incremented());
                REQUIRE(first <= end);
                for (auto index = first; index.isWithin(end.distanceFromZero()); index.uncheckedIncrement()) {
                    const auto &range = ranges.get(index);
                    REQUIRE(range.first >= previousEnd);
                    REQUIRE(range.first <= range.last);
                    REQUIRE_EQUAL(range.first >> 16U, page.toRawValue());
                    REQUIRE_EQUAL(range.last >> 16U, page.toRawValue());
                    REQUIRE(range.attributes <= 0x1FFFU);
                    REQUIRE((range.attributes & 3U) != 0U);
                    for (auto codePoint = range.first; codePoint <= range.last; ++codePoint) {
                        restored.set(el::ItemIndex::fromInteger(static_cast<unsigned>(codePoint)), range.attributes);
                    }
                    previousEnd = static_cast<unsigned>(range.last) + 1U;
                }
            }
            for (auto codePoint = 0U; codePoint < tool::idna::IdnaData::cCodePointCount; ++codePoint) {
                REQUIRE_EQUAL(
                    restored.get(el::ItemIndex::fromInteger(codePoint)),
                    data.attributes(static_cast<char32_t>(codePoint)));
            }
            REQUIRE_EQUAL(data.attributes(static_cast<char32_t>(0x110000U)), 0U);
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCurrentOutputFreshness() {
        _root = tool::frame::ProjectRoot{el::Path::currentDirectory(), TestToolApplication::coreProject()}.discover();
        runWithSources(el::StringList{tool::unicode::UnicodeSource::sourceIdentifier()}, [&] {
            const auto &data = tool::frame::app().source<tool::unicode::UnicodeSource>().idnaData();
            const auto output = tool::idna::IdnaTool::render(data);
            REQUIRE_EQUAL(output, tool::idna::IdnaTool::render(data));
            REQUIRE_FALSE(
                files().updateText(el::Path{"src/erbsland/text/punycode/impl/IdnaData.cpp"_el}, output, true));
        });
    }
};
