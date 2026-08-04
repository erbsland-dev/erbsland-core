// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteArray.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/Endianness.hpp>
#include <erbsland/mem/impl/ByteComparisonTools.hpp>
#include <erbsland/mem/impl/ByteDataView.hpp>
#include <erbsland/mem/impl/ByteReadTools.hpp>
#include <erbsland/mem/impl/ByteWriteTools.hpp>
#include <erbsland/mem/impl/UnsafeByteBlockBuffer.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>

using el::mem::Byte;
using el::mem::ByteArray;
using el::mem::ByteBlock;
using el::mem::ConstByteSpan;
using el::mem::impl::ByteComparisonTools;
using el::mem::impl::ByteDataView;
using el::mem::impl::ByteReadTools;
using el::mem::impl::ByteWriteTools;
using el::mem::impl::UnsafeByteBlockBuffer;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;

TESTED_TARGETS(ByteDataView ByteReadTools ByteComparisonTools ByteWriteTools UnsafeByteBlockBuffer)
class ByteDataViewTest final : public el::UnitTest {
public:
    void testRangeSelection() {
        const auto storage = ByteArray{Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}, Byte{5U}};
        const auto view = ByteDataView{ConstByteSpan{storage.span()}, ByteRange{ByteIndex{1U}, ByteLength{3U}}};

        REQUIRE_EQUAL(view.data().size(), std::size_t{5U});
        REQUIRE_EQUAL(view.length(), ByteLength{3U});
        REQUIRE_EQUAL(view.dataSpan()[0], Byte{2U});
        REQUIRE_EQUAL(view.dataSpan()[2], Byte{4U});

        const auto absolute = view.absoluteRange(ByteRange{ByteIndex{1U}, ByteLength::infinite()});
        REQUIRE_EQUAL(absolute.index(), ByteIndex{2U});
        REQUIRE_EQUAL(absolute.length(), ByteLength{2U});

        const auto clamped = ByteDataView{ConstByteSpan{storage.span()}, ByteRange{ByteIndex{4U}, ByteLength{99U}}};
        REQUIRE_EQUAL(clamped.length(), ByteLength{1U});
        REQUIRE_EQUAL(clamped.dataSpan()[0], Byte{5U});
        REQUIRE(ByteDataView{ConstByteSpan{storage.span()}, ByteRange::noRange()}.dataSpan().empty());
        REQUIRE(
            ByteDataView{ConstByteSpan{storage.span()}, ByteRange{ByteIndex{99U}, ByteLength{1U}}}.dataSpan().empty());
    }

    void testReadAndComparisonTools() {
        const auto storage = ByteArray{Byte{1U}, Byte{2U}, Byte{3U}, Byte{2U}, Byte{3U}};
        const auto needleStorage = ByteArray{Byte{2U}, Byte{3U}};
        const auto view = ByteDataView{ConstByteSpan{storage.span()}};
        const auto needle = ByteDataView{ConstByteSpan{needleStorage.span()}};

        const auto read = ByteReadTools{view};
        REQUIRE_EQUAL(read.get(ByteIndex{2U}), Byte{3U});
        REQUIRE_EQUAL(read.get(ByteIndex{99U}, Byte{9U}), Byte{9U});
        REQUIRE_THROWS(read.getOrThrow(ByteIndex::noIndex()));
        REQUIRE_EQUAL(read.toVector().size(), std::size_t{5U});

        const auto compare = ByteComparisonTools{view};
        REQUIRE_FALSE(compare.startsWith(needle));
        REQUIRE(compare.endsWith(needle));
        REQUIRE(compare.contains(needle));
        REQUIRE_EQUAL(compare.find(needle), ByteIndex{1U});
        REQUIRE_EQUAL(compare.find(needle, ByteIndex{2U}), ByteIndex{3U});
        REQUIRE_EQUAL(compare.findLast(needle), ByteIndex{3U});
        const auto sameComparison = compare.compare(ByteDataView{ConstByteSpan{storage.span()}});
        const auto greaterComparison = compare.compare(ByteDataView{ByteArray{Byte{1U}, Byte{2U}, Byte{4U}}.span()});
        const auto lessComparison = compare.compare(ByteDataView{ByteArray{Byte{1U}, Byte{2U}}.span()});
        REQUIRE_EQUAL(sameComparison, std::strong_ordering::equal);
        REQUIRE_LESS(greaterComparison, 0);
        REQUIRE_GREATER(lessComparison, 0);

        const auto differentFirst = ByteArray{Byte{9U}, Byte{2U}, Byte{3U}, Byte{2U}, Byte{3U}};
        const auto differentMiddle = ByteArray{Byte{1U}, Byte{2U}, Byte{9U}, Byte{2U}, Byte{3U}};
        const auto differentLast = ByteArray{Byte{1U}, Byte{2U}, Byte{3U}, Byte{2U}, Byte{9U}};
        REQUIRE(compare.isEqualConstTime(ByteDataView{ConstByteSpan{storage.span()}}));
        REQUIRE_FALSE(compare.isEqualConstTime(ByteDataView{ConstByteSpan{differentFirst.span()}}));
        REQUIRE_FALSE(compare.isEqualConstTime(ByteDataView{ConstByteSpan{differentMiddle.span()}}));
        REQUIRE_FALSE(compare.isEqualConstTime(ByteDataView{ConstByteSpan{differentLast.span()}}));
        REQUIRE_FALSE(compare.isEqualConstTime(ByteDataView{ConstByteSpan{needleStorage.span()}}));
        REQUIRE(ByteComparisonTools{ByteDataView{ConstByteSpan{}}}.isEqualConstTime(ByteDataView{ConstByteSpan{}}));
    }

    void testWriteTools() {
        auto storage = std::array{Byte{0U}, Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}, Byte{5U}};
        auto write = ByteWriteTools{el::mem::ByteSpan{storage}};

        write.set(ByteIndex{0U}, Byte{9U});
        write.set(ByteIndex::noIndex(), Byte{8U});
        write.xorAt(ByteIndex{1U}, Byte{0xffU});
        REQUIRE_THROWS(write.setOrThrow(ByteIndex{99U}, Byte{}));
        REQUIRE_THROWS(write.xorAtOrThrow(ByteIndex::noIndex(), Byte{}));
        REQUIRE_EQUAL(storage[0], Byte{9U});
        REQUIRE_EQUAL(storage[1], Byte{0xfeU});

        write.fill(ByteRange{ByteIndex{2U}, ByteLength{2U}}, Byte{0xaaU});
        REQUIRE_EQUAL(storage[2], Byte{0xaaU});
        REQUIRE_EQUAL(storage[3], Byte{0xaaU});

        const auto overlappingSource = ByteDataView{el::mem::ConstByteSpan{storage}.first(4U)};
        static_cast<void>(write.overwrite(ByteRange{ByteIndex{1U}, ByteLength{4U}}, overlappingSource));
        const auto expectedAfterOverwrite =
            std::array{Byte{9U}, Byte{9U}, Byte{0xfeU}, Byte{0xaaU}, Byte{0xaaU}, Byte{5U}};
        REQUIRE_EQUAL(storage, expectedAfterOverwrite);

        const auto xorSource = ByteDataView{el::mem::ConstByteSpan{storage}.subspan(0U, 3U)};
        write.xorWith(ByteRange{ByteIndex{1U}, ByteLength{3U}}, xorSource);
        REQUIRE_EQUAL(storage[1], Byte{0U});
        REQUIRE_EQUAL(storage[2], Byte{0xf7U});
        REQUIRE_EQUAL(storage[3], Byte{0x54U});

        REQUIRE(write.setInteger(ByteIndex{2U}, uint16_t{0x1234U}, el::mem::Endianness::Big));
        const auto read = ByteReadTools{ByteDataView{el::mem::ConstByteSpan{storage}}};
        REQUIRE_EQUAL(read.getInteger<uint16_t>(ByteIndex{2U}, el::mem::Endianness::Big), uint16_t{0x1234U});
        REQUIRE_EQUAL(read.span(ByteRange{ByteIndex{4U}, ByteLength{99U}}).size(), std::size_t{2U});
    }

    void testUnsafeGrowingBuffer() {
        auto buffer = UnsafeByteBlockBuffer{ByteLength{2U}};
        auto initial = buffer.data();
        initial[0] = Byte{1U};
        initial[1] = Byte{2U};

        const auto grown = buffer.grow(ByteLength{5U}, ByteLength{2U}, ByteLength{7U});
        REQUIRE_GREATER_EQUAL(buffer.capacity(), ByteLength{5U});
        REQUIRE_LESS_EQUAL(buffer.capacity(), ByteLength{7U});
        REQUIRE_EQUAL(grown[0], Byte{1U});
        REQUIRE_EQUAL(grown[1], Byte{2U});
        grown[2] = Byte{3U};
        grown[3] = Byte{4U};

        const auto block = ByteBlock{buffer.take(ByteLength{4U})};
        REQUIRE_EQUAL(block, ByteBlock({1U, 2U, 3U, 4U}));

        auto emptyBuffer = UnsafeByteBlockBuffer{ByteLength{4U}};
        REQUIRE(emptyBuffer.take(ByteLength::zero()).isEmpty());
    }
};
