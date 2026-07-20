// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditorList.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/text/u16/U16StringEditorList.hpp>
#include <erbsland/text/u16/U16StringList.hpp>
#include <erbsland/text/u32/U32StringEditorList.hpp>
#include <erbsland/text/u32/U32StringList.hpp>
#include <erbsland/text/u8/U8StringEditorList.hpp>
#include <erbsland/text/u8/U8StringList.hpp>
#include <erbsland/unit/ElementIndex.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <string>
#include <string_view>
#include <type_traits>

using namespace el::text::literals;

using el::unit::ElementCount;
using el::unit::ElementIndex;
using namespace el::text;

static_assert(std::is_same_v<StringList::Element, String>);
static_assert(std::is_same_v<StringEditorList::Element, StringEditor>);
static_assert(std::is_same_v<U16StringList::Element, U16String>);
static_assert(std::is_same_v<U16StringEditorList::Element, U16StringEditor>);

TESTED_TARGETS(
    StringEditorList StringList U8StringEditorList U8StringList U16StringEditorList U16StringList U32StringEditorList
        U32StringList)
class StringListTest final : public el::UnitTest {
public:
    void testU8StringListOperations() {
        const auto list = U8StringEditorList{
            U8StringEditor{std::string_view{"Beta"}},
            U8StringEditor{},
            U8StringEditor{std::string_view{"alpha"}},
            U8StringEditor{std::string_view{"ALPHA"}},
        };

        REQUIRE_EQUAL(
            list.compare(
                U8StringEditorList{
                    U8StringEditor{std::string_view{"beta"}},
                    U8StringEditor{},
                    U8StringEditor{std::string_view{"Alpha"}},
                    U8StringEditor{std::string_view{"alpha"}},
                },
                Char::compareCaseFolded),
            std::strong_ordering::equal);
        REQUIRE_EQUAL(
            list.findFirst(U8StringEditor{std::string_view{"ALPHA"}}, Char::compareCaseFolded), ElementIndex{2});
        REQUIRE_EQUAL(
            list.findLast(U8StringEditor{std::string_view{"alpha"}}, Char::compareCaseFolded), ElementIndex{3});
        REQUIRE(list.contains(U8StringEditor{std::string_view{"BETA"}}, Char::compareCaseFolded));
        REQUIRE_EQUAL(list.removedEmpty().count().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(
            StringConverter{list.removedEmpty().join(U8StringEditor{std::string_view{"|"}})}.toStdString(),
            std::string{"Beta|alpha|ALPHA"});
        REQUIRE_EQUAL(
            StringConverter{list.removedEmpty().sort().join(U8StringEditor{std::string_view{"|"}})}.toStdString(),
            std::string{"ALPHA|Beta|alpha"});
        REQUIRE_EQUAL(
            StringConverter{list.removedEmpty().sorted().join(U8StringEditor{std::string_view{"|"}})}.toStdString(),
            std::string{"ALPHA|Beta|alpha"});
        REQUIRE_EQUAL(
            StringConverter{list.removedEmpty().sort(Char::compareCaseFolded).first()}.toStdString(),
            std::string{"alpha"});
    }

    void testU8StringListJoinKeepsStorageAlive() {

        const auto parts =
            StringList::fromSplit("A,,b"_el, CharSet{","_el}, ElementCount::infinite(), true).removedEmpty();
        const auto joined = parts.join("|"_el);

        REQUIRE_EQUAL(StringConverter{joined}.toStdString(), std::string{"A|b"});
    }

    void testFromSplitOptions() {

        const auto parts = StringList::fromSplit(",a,,b,"_el, CharSet{","_el});
        const auto keptParts = StringList::fromSplit(",a,,b,"_el, CharSet{","_el}, ElementCount::infinite(), true);
        const auto zeroSplitParts = StringList::fromSplit(",a,,b,"_el, CharSet{","_el}, ElementCount::zero(), true);
        const auto limitedParts = StringList::fromSplit("a,b,c"_el, CharSet{","_el}, ElementCount{2U});
        const auto limitedKeptParts = StringList::fromSplit(",a,,b,"_el, CharSet{","_el}, ElementCount{2U}, true);
        const auto limitedDroppedParts = StringList::fromSplit(",a,,b,"_el, CharSet{","_el}, ElementCount{2U}, false);
        const auto owningParts = StringEditorList::fromSplit("a,b"_el, CharSet{","_el});

        REQUIRE_EQUAL(parts.count().toSizeT(), std::size_t{2U});
        REQUIRE_EQUAL(keptParts.count().toSizeT(), std::size_t{5U});
        REQUIRE_EQUAL(zeroSplitParts.count().toSizeT(), std::size_t{1U});
        REQUIRE_EQUAL(zeroSplitParts[ElementIndex::zero()], ",a,,b,"_el);
        REQUIRE_EQUAL(limitedParts.count().toSizeT(), std::size_t{3U});
        REQUIRE_EQUAL(limitedParts[ElementIndex{2U}], "c"_el);
        REQUIRE_EQUAL(limitedKeptParts.count().toSizeT(), std::size_t{3U});
        REQUIRE_EQUAL(limitedKeptParts[ElementIndex::zero()], ""_el);
        REQUIRE_EQUAL(limitedKeptParts[ElementIndex{1U}], "a"_el);
        REQUIRE_EQUAL(limitedKeptParts[ElementIndex{2U}], ",b,"_el);
        REQUIRE_EQUAL(limitedDroppedParts.count().toSizeT(), std::size_t{2U});
        REQUIRE_EQUAL(limitedDroppedParts[ElementIndex::zero()], "a"_el);
        REQUIRE_EQUAL(limitedDroppedParts[ElementIndex{1U}], ",b,"_el);
        REQUIRE_EQUAL(StringConverter{owningParts.join()}.toStdString(), std::string{"ab"});
    }

    void testAliasesAndWideStringLists() {

        const auto common = StringEditorList{StringEditor{std::string_view{"a"}}};
        const auto commonView = StringList::fromSplit("a,b"_el, CharSet{","_el});
        REQUIRE_EQUAL(StringConverter{common.first()}.toStdString(), std::string{"a"});
        REQUIRE_EQUAL(StringConverter{commonView.join("|"_el)}.toStdString(), std::string{"a|b"});

        const auto u16Parts = U16StringList::fromSplit(u"A,B"_el, CharSet{Char{U','}});
        REQUIRE_EQUAL(StringConverter{u16Parts.join(u"+"_el)}.toStdString(), std::string{"A+B"});

        const auto u32Parts = U32StringList::fromSplit(U"A,B"_el, CharSet{Char{U','}});
        REQUIRE_EQUAL(StringConverter{u32Parts.join(U"+"_el)}.toStdString(), std::string{"A+B"});
    }
};
