// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/text/StringViewList.hpp>
#include <erbsland/text/u16/U16StringList.hpp>
#include <erbsland/text/u16/U16StringViewList.hpp>
#include <erbsland/text/u32/U32StringList.hpp>
#include <erbsland/text/u32/U32StringViewList.hpp>
#include <erbsland/text/u8/U8StringList.hpp>
#include <erbsland/text/u8/U8StringViewList.hpp>
#include <erbsland/unit/ElementIndex.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <string>
#include <string_view>

using el::unit::ElementCount;
using el::unit::ElementIndex;
using namespace el::text;

TESTED_TARGETS(
    StringList StringViewList U8StringList U8StringViewList U16StringList U16StringViewList U32StringList
        U32StringViewList)
class StringListTest final : public el::UnitTest {
public:
    void testU8StringListOperations() {
        const auto list = U8StringList{
            U8String{std::string_view{"Beta"}},
            U8String{},
            U8String{std::string_view{"alpha"}},
            U8String{std::string_view{"ALPHA"}},
        };

        REQUIRE_EQUAL(
            list.compare(
                U8StringList{
                    U8String{std::string_view{"beta"}},
                    U8String{},
                    U8String{std::string_view{"Alpha"}},
                    U8String{std::string_view{"alpha"}},
                },
                Char::compareCaseFolded),
            std::strong_ordering::equal);
        REQUIRE_EQUAL(list.findFirst(U8String{std::string_view{"ALPHA"}}, Char::compareCaseFolded), ElementIndex{2});
        REQUIRE_EQUAL(list.findLast(U8String{std::string_view{"alpha"}}, Char::compareCaseFolded), ElementIndex{3});
        REQUIRE(list.contains(U8String{std::string_view{"BETA"}}, Char::compareCaseFolded));
        REQUIRE_EQUAL(list.removedEmpty().count().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(
            StringConverter{list.removedEmpty().join(U8String{std::string_view{"|"}})}.toStdString(),
            std::string{"Beta|alpha|ALPHA"});
        REQUIRE_EQUAL(
            StringConverter{list.removedEmpty().sort().join(U8String{std::string_view{"|"}})}.toStdString(),
            std::string{"ALPHA|Beta|alpha"});
        REQUIRE_EQUAL(
            StringConverter{list.removedEmpty().sorted().join(U8String{std::string_view{"|"}})}.toStdString(),
            std::string{"ALPHA|Beta|alpha"});
        REQUIRE_EQUAL(
            StringConverter{list.removedEmpty().sort(Char::compareCaseFolded).first()}.toStdString(),
            std::string{"alpha"});
    }

    void testU8StringViewListJoinKeepsStorageAlive() {
        using namespace el::text::literals;

        const auto parts =
            StringViewList::fromSplit("A,,b"_els, CharSet{","_elv}, ElementCount::infinite(), true).removedEmpty();
        const auto joined = parts.join("|"_elv);

        REQUIRE_EQUAL(StringConverter{joined}.toStdString(), std::string{"A|b"});
    }

    void testFromSplitOptions() {
        using namespace el::text::literals;

        const auto parts = StringViewList::fromSplit(",a,,b,"_elv, CharSet{","_elv});
        const auto keptParts =
            StringViewList::fromSplit(",a,,b,"_elv, CharSet{","_elv}, ElementCount::infinite(), true);
        const auto zeroSplitParts =
            StringViewList::fromSplit(",a,,b,"_elv, CharSet{","_elv}, ElementCount::zero(), true);
        const auto limitedParts = StringViewList::fromSplit("a,b,c"_elv, CharSet{","_elv}, ElementCount{2U});
        const auto limitedKeptParts = StringViewList::fromSplit(",a,,b,"_elv, CharSet{","_elv}, ElementCount{2U}, true);
        const auto limitedDroppedParts =
            StringViewList::fromSplit(",a,,b,"_elv, CharSet{","_elv}, ElementCount{2U}, false);
        const auto owningParts = StringList::fromSplit("a,b"_elv, CharSet{","_elv});

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
        using namespace el::text::literals;

        const auto common = StringList{String{std::string_view{"a"}}};
        const auto commonView = StringViewList::fromSplit("a,b"_els, CharSet{","_elv});
        REQUIRE_EQUAL(StringConverter{common.first()}.toStdString(), std::string{"a"});
        REQUIRE_EQUAL(StringConverter{commonView.join("|"_elv)}.toStdString(), std::string{"a|b"});

        const auto u16Parts = U16StringViewList::fromSplit(u"A,B"_els, CharSet{Char{U','}});
        REQUIRE_EQUAL(StringConverter{u16Parts.join(u"+"_elv)}.toStdString(), std::string{"A+B"});

        const auto u32Parts = U32StringViewList::fromSplit(U"A,B"_els, CharSet{Char{U','}});
        REQUIRE_EQUAL(StringConverter{u32Parts.join(U"+"_elv)}.toStdString(), std::string{"A+B"});
    }
};
