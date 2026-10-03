// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ByteArray.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/ByteBuffer.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/List.hpp>
#include <erbsland/mem/impl/ByteDataView.hpp>
#include <erbsland/mem/impl/ByteReadTools.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <concepts>
#include <utility>

using namespace el::unit;

TESTED_TARGETS(IntegerUnitIndex IntegerUnitRange ByteArray ByteBlock ByteBlockEditor ByteBuffer ByteReadTools List)
class LastIndexTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        static_assert(ByteIndex::last(ByteCount::zero()).isNoIndex());
        static_assert(ByteIndex::last(ByteCount::one()) == ByteIndex::zero());
        static_assert(ByteIndex::last(ByteCount{7}) == ByteIndex{6});
        static_assert(ByteIndex::last(ByteCount::infinite()).isNoIndex());
        static_assert(ByteIndex::last(ByteCount::maximum()) == ByteIndex::maximum().decremented());
        static_assert(ItemIndex::last(ItemCount{3}) == ItemIndex{2});
        static_assert(CpIndex::last(CpCount{3}) == CpIndex{2});
        static_assert(!std::invocable<decltype(&ByteIndex::last), CpCount>);
        static_assert(noexcept(ByteIndex::last(std::declval<ByteCount>())));
        static_assert(noexcept(std::declval<ByteRange>().lastIndex()));
        static_assert(ByteRange{ByteIndex{4}, ByteCount{3}}.lastIndex() == ByteIndex{6});
        static_assert(ByteRange::emptyAt(ByteIndex{4}).lastIndex().isNoIndex());
        static_assert(ByteRange::all().lastIndex().isNoIndex());
        static_assert(el::mem::ByteArray<0>::lastIndex().isNoIndex());
        static_assert(el::mem::ByteArray<1>::lastIndex() == ByteIndex::zero());
        static_assert(el::mem::ByteArray<4>::lastIndex() == ByteIndex{3});
    }

    void testFactoryRuntimeBoundaries() {
        for (
            const auto &[length, expected] : std::array{
                std::pair{ByteCount::zero(), ByteIndex::noIndex()},
                std::pair{ByteCount::one(), ByteIndex::zero()},
                std::pair{ByteCount{2}, ByteIndex::one()},
                std::pair{ByteCount{127}, ByteIndex{126}},
                std::pair{ByteCount::maximum(), ByteIndex::maximum().decremented()},
                std::pair{ByteCount::infinite(), ByteIndex::noIndex()}}) {
            REQUIRE_EQUAL(ByteIndex::last(length), expected);
        }
    }

    void testRangeCoordinatesAndBoundaries() {
        const auto range = ByteRange{ByteIndex{11}, ByteCount{4}};
        REQUIRE_EQUAL(range.lastIndex(), ByteIndex{14});
        REQUIRE(range.contains(range.lastIndex()));
        REQUIRE_EQUAL(range.relativeTo(ByteIndex{11}).lastIndex(), ByteIndex{3});
        REQUIRE_EQUAL((ByteRange{ByteIndex{11}, ByteCount::one()}.lastIndex()), ByteIndex{11});
        for (
            const auto absent : std::array{
                ByteRange::empty(),
                ByteRange::emptyAt(ByteIndex{11}),
                ByteRange::emptyAt(ByteIndex::maximum()),
                ByteRange::noRange(),
                ByteRange::all(),
                ByteRange::fromBegin(ByteIndex{11}),
                ByteRange{ByteIndex::maximum(), ByteCount::one()}}) {
            REQUIRE(absent.lastIndex().isNoIndex());
        }
        const auto nearMaximum = ByteRange{ByteIndex::maximum().decremented(), ByteCount::one()};
        REQUIRE_EQUAL(nearMaximum.lastIndex(), nearMaximum.beginIndex());
        REQUIRE_EQUAL(ByteRange::fromLength(ByteCount::maximum()).lastIndex(), nearMaximum.beginIndex());
    }

    void testByteContainerIndexes() {
        REQUIRE(el::mem::ByteBlock{}.lastIndex().isNoIndex());
        REQUIRE(el::mem::ByteBlockEditor{}.lastIndex().isNoIndex());
        REQUIRE(el::mem::ByteBuffer{}.lastIndex().isNoIndex());

        const auto bytes = el::mem::ByteBlock{1U, 2U, 3U, 4U};
        REQUIRE_EQUAL(bytes.lastIndex(), ByteIndex{3});
        REQUIRE_EQUAL(bytes.get(bytes.lastIndex()), el::mem::Byte{4U});
        const auto slice = bytes.slice(ByteIndex{1}, ByteCount{2});
        REQUIRE_EQUAL(slice.lastIndex(), ByteIndex::one());
        REQUIRE_EQUAL(slice.get(slice.lastIndex()), el::mem::Byte{3U});
        REQUIRE(bytes.slice(bytes.endIndex(), ByteCount::one()).lastIndex().isNoIndex());

        auto editor = el::mem::ByteBlockEditor{slice};
        REQUIRE_EQUAL(editor.lastIndex(), ByteIndex::one());
        editor.resize(ByteCount::one());
        REQUIRE_EQUAL(editor.lastIndex(), ByteIndex::zero());
        editor.clear();
        REQUIRE(editor.lastIndex().isNoIndex());

        auto buffer = el::mem::ByteBuffer{bytes.span()};
        REQUIRE_EQUAL(buffer.lastIndex(), ByteIndex{3});
        buffer.resize(ByteCount::one());
        REQUIRE_EQUAL(buffer.lastIndex(), ByteIndex::zero());
        buffer.clear();
        REQUIRE(buffer.lastIndex().isNoIndex());

        const auto tools = el::mem::impl::ByteReadTools{
            el::mem::impl::ByteDataView{bytes.span(), ByteRange{ByteIndex{1}, ByteCount{2}}}};
        REQUIRE_EQUAL(tools.lastIndex(), ByteIndex::one());
        REQUIRE_EQUAL(tools.get(tools.lastIndex()), el::mem::Byte{3U});
        REQUIRE(el::mem::impl::ByteReadTools{el::mem::impl::ByteDataView{}}.lastIndex().isNoIndex());
    }

    void testListIndexesAndLastOperations() {
        auto values = el::util::List<int>{};
        REQUIRE(values.lastIndex().isNoIndex());
        REQUIRE(values.findLast(1).isNoIndex());
        auto called = false;
        REQUIRE(values
                .findLastIf([&](int) -> bool {
                    called = true;
                    return true;
                })
                .isNoIndex());
        REQUIRE_FALSE(called);
        REQUIRE_EQUAL(values.takeLast(), 0);
        values.removeLast();
        REQUIRE(values.isEmpty());

        values.append(7).append(8).append(7);
        REQUIRE_EQUAL(values.lastIndex(), ItemIndex{2});
        REQUIRE_EQUAL(values.get(values.lastIndex()), 7);
        REQUIRE_EQUAL(values.findLast(7), values.lastIndex());
        REQUIRE_EQUAL(values.findLastIf([](int value) -> bool { return value == 8; }), ItemIndex::one());
        const auto shared = values;
        REQUIRE_EQUAL(values.takeLast(), 7);
        REQUIRE_EQUAL(values.lastIndex(), ItemIndex::one());
        REQUIRE_EQUAL(shared.lastIndex(), ItemIndex{2});
        values.removeLast();
        REQUIRE_EQUAL(values.lastIndex(), ItemIndex::zero());
        values.removeLast();
        REQUIRE(values.lastIndex().isNoIndex());
    }
};
