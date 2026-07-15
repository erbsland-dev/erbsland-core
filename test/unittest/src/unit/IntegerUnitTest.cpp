// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/SaturatingInteger.hpp>
#include <erbsland/unit/all.hpp>
#include <erbsland/unit/CodeContinuousRange.hpp>
#include <erbsland/unit/CodeLocation.hpp>
#include <erbsland/unit/ColumnCount.hpp>
#include <erbsland/unit/ColumnIndex.hpp>
#include <erbsland/unit/ColumnOffset.hpp>
#include <erbsland/unit/ColumnRange.hpp>
#include <erbsland/unit/ColumnUnit.hpp>
#include <erbsland/unit/LineCount.hpp>
#include <erbsland/unit/LineIndex.hpp>
#include <erbsland/unit/LineOffset.hpp>
#include <erbsland/unit/LineRange.hpp>
#include <erbsland/unit/LineUnit.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

TESTED_TARGETS(
    IntegerUnit ByteUnit U16DataUnit CharUnit LineUnit ColumnUnit CodeLocation CodeContinuousRange IntegerUnitIndex
        IntegerUnitAmount IntegerUnitOffset IntegerUnitRange)
class IntegerUnitTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        using namespace el::unit;

        static_assert(std::same_as<ByteIndex, IntegerUnitIndex<ByteUnit>>);
        static_assert(std::same_as<ByteLength, IntegerUnitAmount<ByteUnit>>);
        static_assert(std::same_as<ByteOffset, IntegerUnitOffset<ByteUnit>>);
        static_assert(std::same_as<ByteRange, IntegerUnitRange<ByteUnit>>);
        static_assert(std::same_as<CpIndex, IntegerUnitIndex<CpUnit>>);
        static_assert(std::same_as<CpLength, IntegerUnitAmount<CpUnit>>);
        static_assert(std::same_as<CpOffset, IntegerUnitOffset<CpUnit>>);
        static_assert(std::same_as<CpRange, IntegerUnitRange<CpUnit>>);
        static_assert(std::same_as<LineIndex, IntegerUnitIndex<LineUnit>>);
        static_assert(std::same_as<LineCount, IntegerUnitAmount<LineUnit>>);
        static_assert(std::same_as<LineOffset, IntegerUnitOffset<LineUnit>>);
        static_assert(std::same_as<LineRange, IntegerUnitRange<LineUnit>>);
        static_assert(std::same_as<ColumnIndex, IntegerUnitIndex<ColumnUnit>>);
        static_assert(std::same_as<ColumnCount, IntegerUnitAmount<ColumnUnit>>);
        static_assert(std::same_as<ColumnOffset, IntegerUnitOffset<ColumnUnit>>);
        static_assert(std::same_as<ColumnRange, IntegerUnitRange<ColumnUnit>>);
        static_assert(std::same_as<typename ByteIndex::Value, uint64_t>);
        static_assert(std::same_as<typename ByteLength::Value, uint64_t>);
        static_assert(std::same_as<typename ByteOffset::Value, int64_t>);
        static_assert(std::same_as<typename CpIndex::Value, uint32_t>);
        static_assert(std::same_as<typename CpLength::Value, uint32_t>);
        static_assert(std::same_as<typename CpOffset::Value, int32_t>);
        static_assert(std::same_as<typename U16DataIndex::Value, uint32_t>);
        static_assert(std::same_as<typename U16DataLength::Value, uint32_t>);
        static_assert(std::same_as<typename U16DataOffset::Value, int32_t>);

        static_assert(!std::equality_comparable_with<ByteIndex, CpIndex>);
        static_assert(!std::equality_comparable_with<ByteLength, CpLength>);
        static_assert(!std::equality_comparable_with<ByteOffset, CpOffset>);
        static_assert(!std::equality_comparable_with<ByteRange, CpRange>);
        static_assert(LineIndex::noIndex().isNoIndex());
        static_assert(ColumnIndex::noIndex().isNoIndex());
        static_assert(LineRange{LineIndex{3U}, LineCount{2U}}.endIndex() == LineIndex{5U});
        static_assert(ColumnRange{ColumnIndex{4U}, ColumnCount{3U}}.endIndex() == ColumnIndex{7U});
        static_assert(CodeLocation{}.line.isNoIndex());
        static_assert(CodeLocation{}.column.isNoIndex());
        static_assert(CodeLocation{}.position.isNoIndex());
        static_assert(
            CodeContinuousRange{.begin = {.line = LineIndex{1U}}, .end = {.line = LineIndex{2U}}}.end.line ==
            LineIndex{2U});

        static_assert(ByteLength::zero().isZero());
        static_assert(ByteLength::one().isOne());
        static_assert(ByteLength::maximum().isMaximum());
        static_assert(ByteLength::infinite().isInfinite());
        static_assert(ByteLength{2}.added(ByteLength{3}).toRawValue() == 5U);
        static_assert(ByteLength::maximum().added(ByteLength::one()).isMaximum());
        static_assert((ByteLength{2} + ByteLength{3}) == ByteLength{5});
        static_assert((ByteLength{2} - ByteLength{3}).isZero());
        static_assert((ByteLength{2} * 3U) == ByteLength{6});
        static_assert((3U * ByteLength{2}) == ByteLength{6});
        static_assert((ByteLength{9} / 3U) == ByteLength{3});
        static_assert((ByteLength{10} % 4U) == ByteLength{2});
        static_assert((CpLength{2} * el::math::SaturatingInteger<uint16_t>{3U}) == CpLength{6});
        static_assert((el::math::SaturatingInteger<uint16_t>{3U} * CpLength{2}) == CpLength{6});
        static_assert((CpLength{2} * std::size_t{3U}) == CpLength{6});

        static_assert(ByteOffset::zero().isZero());
        static_assert(ByteOffset::one().isOne());
        static_assert(ByteOffset::minusOne().isMinusOne());
        static_assert(ByteOffset{-3}.isNegative());
        static_assert(ByteOffset{3}.isPositive());
        static_assert(ByteOffset{-3}.absoluteLength() == ByteLength{3});
        static_assert(ByteOffset{-3}.negated() == ByteOffset{3});
        static_assert((ByteOffset{2} + ByteOffset{-5}) == ByteOffset{-3});
        static_assert((ByteOffset{2} - ByteOffset{5}) == ByteOffset{-3});
        static_assert((ByteOffset{-2} * 3) == ByteOffset{-6});
        static_assert((3 * ByteOffset{-2}) == ByteOffset{-6});
        static_assert((ByteOffset{-9} / 3) == ByteOffset{-3});
        static_assert((ByteOffset{-10} % 4) == ByteOffset{-2});
        static_assert((CpOffset{-2} * el::math::SaturatingInteger<int16_t>{3}) == CpOffset{-6});
        static_assert((el::math::SaturatingInteger<int16_t>{3} * CpOffset{-2}) == CpOffset{-6});
        static_assert((CpOffset{-2} * int64_t{3}) == CpOffset{-6});
        static_assert(-ByteOffset{-3} == ByteOffset{3});

        static_assert(ByteIndex::zero().isZero());
        static_assert(ByteIndex::one().isOne());
        static_assert(ByteIndex::maximum().isMaximum());
        static_assert(ByteIndex::noIndex().isNoIndex());
        static_assert(ByteIndex{5}.isWithin(ByteLength{6}));
        static_assert(!ByteIndex{6}.isWithin(ByteLength{6}));
        static_assert(!ByteIndex::noIndex().isWithin(ByteLength::infinite()));
        static_assert(ByteIndex{5}.advanced(ByteLength{3}) == ByteIndex{8});
        static_assert(ByteIndex{5}.retreated(ByteLength{8}).isZero());
        static_assert(ByteIndex{5}.moved(ByteOffset{-2}) == ByteIndex{3});
        static_assert(ByteIndex{5}.absoluteDistanceTo(ByteIndex{2}) == ByteLength{3});
        static_assert((ByteIndex{5} + ByteLength{3}) == ByteIndex{8});
        static_assert((ByteIndex{5} - ByteLength{8}).isZero());
        static_assert((ByteIndex{5} + ByteOffset{-2}) == ByteIndex{3});
        static_assert((ByteIndex{5} - ByteOffset{-2}) == ByteIndex{7});
        static_assert(ByteIndex{5}.offsetFromZero() == ByteOffset{5});
        static_assert(ByteIndex{5}.offsetTo(ByteIndex{2}) == ByteOffset{-3});
        static_assert(!ByteIndex{5}.wouldOffsetToSaturate(ByteIndex{2}));
        static_assert(ByteIndex::end(ByteLength::infinite()).isNoIndex());

        static_assert(ByteRange::empty().isEmpty());
        static_assert(ByteRange::all().isInfinite());
        static_assert(ByteRange::all().contains(ByteIndex{42}));
        static_assert(ByteRange{ByteIndex{2}, ByteLength{3}}.contains(ByteIndex{4}));
        static_assert(!ByteRange{ByteIndex{2}, ByteLength{3}}.contains(ByteIndex{5}));
        static_assert(ByteRange{ByteIndex{2}, ByteLength{3}}.endIndex() == ByteIndex{5});
        static_assert(ByteRange::emptyAt(ByteIndex{10}).isWithin(ByteLength{10}));
        static_assert(!ByteRange{ByteIndex{8}, ByteLength{3}}.isWithin(ByteLength{10}));
        static_assert(
            ByteRange{ByteIndex{2}, ByteLength{5}}.clampedTo(ByteLength{4}) == ByteRange{ByteIndex{2}, ByteLength{2}});
        static_assert(
            ByteRange{ByteIndex{2}, ByteLength::infinite()}.clampedTo(ByteLength{4}) ==
            ByteRange{ByteIndex{2}, ByteLength{2}});
        static_assert(
            ByteRange{ByteIndex{5}, ByteLength{1}}.clampedTo(ByteLength{4}) == ByteRange::emptyAt(ByteIndex{4}));
        static_assert(
            ByteRange{ByteIndex{1}, ByteLength{2}}.withOrigin(ByteIndex{10}) ==
            ByteRange{ByteIndex{11}, ByteLength{2}});
        static_assert((ByteRange{ByteIndex{5}, ByteLength{2}} + ByteOffset{-3}).index() == ByteIndex{2});
        static_assert((ByteRange{ByteIndex{5}, ByteLength{2}} - ByteOffset{-3}).index() == ByteIndex{8});
    }

    void testLengthOperators() {
        using namespace el::unit;

        auto length = ByteLength{2};
        length += ByteLength{3};
        REQUIRE(length == ByteLength{5});
        length -= ByteLength{10};
        REQUIRE(length.isZero());
        --length;
        REQUIRE(length.isZero());
        ++length;
        REQUIRE(length.isOne());
    }

    void testLengthScalarOperators() {
        using namespace el::unit;
        using el::math::SaturatingInteger;

        auto length = ByteLength{10};
        REQUIRE_EQUAL(length * 4U, ByteLength{40});
        REQUIRE_EQUAL(4U * length, ByteLength{40});
        REQUIRE_EQUAL(length / 4U, ByteLength{2});
        REQUIRE_EQUAL(length % 4U, ByteLength{2});
        REQUIRE_EQUAL(CpLength{6} * SaturatingInteger<uint16_t>{3U}, CpLength{18});
        REQUIRE_EQUAL(CpLength{6} * std::size_t{3U}, CpLength{18});

        length *= 3U;
        REQUIRE_EQUAL(length, ByteLength{30});
        length /= 4U;
        REQUIRE_EQUAL(length, ByteLength{7});
        length %= 5U;
        REQUIRE_EQUAL(length, ByteLength{2});

        REQUIRE(ByteLength::maximum().multiplied(2U).isMaximum());
        REQUIRE(CpLength::maximum().multiplied(std::numeric_limits<std::size_t>::max()).isMaximum());
        REQUIRE(ByteLength::infinite().multiplied(0U).isInfinite());
        REQUIRE(ByteLength::infinite().divided(2U).isInfinite());
        REQUIRE(ByteLength::infinite().modulo(2U).isInfinite());
    }

    void testLengthThrowingVariants() {
        using namespace el::unit;

        auto length = ByteLength{2};
        length.addOrThrow(ByteLength{3});
        REQUIRE(length == ByteLength{5});
        REQUIRE_THROWS(ByteLength::maximum().addOrThrow(ByteLength::one()));
        REQUIRE_THROWS(ByteLength::infinite().addedOrThrow(ByteLength::one()));
        REQUIRE_THROWS(ByteLength{2}.subtractOrThrow(ByteLength{3}));
    }

    void testOffsetOperatorsAndThrowingVariants() {
        using namespace el::unit;

        auto offset = ByteOffset{2};
        offset += ByteOffset{3};
        REQUIRE(offset == ByteOffset{5});
        offset -= ByteOffset{8};
        REQUIRE(offset == ByteOffset{-3});
        REQUIRE((-offset) == ByteOffset{3});
        REQUIRE((offset++ == ByteOffset{-3}));
        REQUIRE(offset == ByteOffset{-2});
        REQUIRE((--offset) == ByteOffset{-3});

        REQUIRE(ByteOffset::maximum().added(ByteOffset::one()).isMaximum());
        REQUIRE(ByteOffset::minimum().subtracted(ByteOffset::one()).isMinimum());
        REQUIRE_THROWS(ByteOffset::maximum().addOrThrow(ByteOffset::one()));
        REQUIRE_THROWS(ByteOffset::minimum().subtractOrThrow(ByteOffset::one()));
    }

    void testOffsetScalarOperators() {
        using namespace el::unit;
        using el::math::SaturatingInteger;

        auto offset = ByteOffset{-10};
        REQUIRE_EQUAL(offset * 4, ByteOffset{-40});
        REQUIRE_EQUAL(4 * offset, ByteOffset{-40});
        REQUIRE_EQUAL(offset / 4, ByteOffset{-2});
        REQUIRE_EQUAL(offset % 4, ByteOffset{-2});
        REQUIRE_EQUAL(CpOffset{-6} * SaturatingInteger<int16_t>{3}, CpOffset{-18});
        REQUIRE_EQUAL(CpOffset{-6} * int64_t{3}, CpOffset{-18});

        offset *= -3;
        REQUIRE_EQUAL(offset, ByteOffset{30});
        offset /= -4;
        REQUIRE_EQUAL(offset, ByteOffset{-7});
        offset %= 5;
        REQUIRE_EQUAL(offset, ByteOffset{-2});

        REQUIRE(ByteOffset::maximum().multiplied(2).isMaximum());
        REQUIRE(CpOffset::maximum().multiplied(std::numeric_limits<int64_t>::max()).isMaximum());
        REQUIRE(ByteOffset::minimum().multiplied(2).isMinimum());
        REQUIRE(ByteOffset::minimum().divided(-1).isMaximum());
        REQUIRE(ByteOffset::minimum().modulo(-1).isZero());
    }

    void testIndexThrowingVariants() {
        using namespace el::unit;

        auto index = ByteIndex{5};
        index.advanceOrThrow(ByteLength{3});
        REQUIRE(index == ByteIndex{8});
        index.retreatOrThrow(ByteLength{4});
        REQUIRE(index == ByteIndex{4});
        index.moveOrThrow(ByteOffset{-2});
        REQUIRE(index == ByteIndex{2});

        REQUIRE(index.offsetFromZeroOrThrow() == ByteOffset{2});
        REQUIRE(index.offsetToOrThrow(ByteIndex{5}) == ByteOffset{3});
        REQUIRE_THROWS(ByteIndex::noIndex().advanceOrThrow(ByteLength::one()));
        REQUIRE_THROWS(ByteIndex{2}.retreatOrThrow(ByteLength{3}));
        REQUIRE_THROWS(ByteIndex::maximum().advanceOrThrow(ByteLength::one()));
        REQUIRE_THROWS(ByteIndex{2}.moveOrThrow(ByteOffset{-3}));
        REQUIRE_THROWS(ByteIndex::noIndex().offsetFromZeroOrThrow());
        REQUIRE_THROWS(ByteIndex::noIndex().offsetToOrThrow(ByteIndex::zero()));
    }

    void testIndexOffsetConversions() {
        using namespace el::unit;

        static constexpr auto cInt64Max = static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
        REQUIRE(!ByteIndex{cInt64Max}.wouldOffsetFromZeroSaturate());
        REQUIRE(ByteIndex{cInt64Max}.offsetFromZero() == ByteOffset::maximum());
        REQUIRE(ByteIndex{cInt64Max + 1U}.wouldOffsetFromZeroSaturate());
        REQUIRE(ByteIndex{cInt64Max + 1U}.offsetFromZero().isMaximum());

        REQUIRE(!CpIndex{5}.wouldOffsetToSaturate(CpIndex{2}));
        REQUIRE(CpIndex{5}.offsetTo(CpIndex{2}) == CpOffset{-3});
        REQUIRE(!CpIndex{0x80000000U}.wouldOffsetToSaturate(CpIndex{0}));
        REQUIRE(CpIndex{0x80000000U}.offsetTo(CpIndex{0}).isMinimum());
        REQUIRE(CpIndex{0x80000001U}.wouldOffsetToSaturate(CpIndex{0}));
        REQUIRE(CpIndex{0x80000001U}.offsetTo(CpIndex{0}).isMinimum());
        REQUIRE(CpIndex{0}.wouldOffsetToSaturate(CpIndex{0x80000000U}));
        REQUIRE(CpIndex{0}.offsetTo(CpIndex{0x80000000U}).isMaximum());
    }

    void testRangeRuntimeBehavior() {
        using namespace el::unit;

        auto range = ByteRange{ByteIndex{4}, ByteLength{3}};
        REQUIRE(range.contains(ByteIndex{4}));
        REQUIRE(range.contains(ByteIndex{6}));
        REQUIRE(!range.contains(ByteIndex{7}));
        REQUIRE(range.isWithin(ByteLength{7}));
        REQUIRE(!range.isWithin(ByteLength{6}));

        range.move(ByteOffset{-2});
        REQUIRE(range.index() == ByteIndex{2});
        REQUIRE(range.endIndex() == ByteIndex{5});
        range += ByteOffset{2};
        REQUIRE(range.index() == ByteIndex{4});
        REQUIRE((range - ByteOffset{2}).index() == ByteIndex{2});
        REQUIRE(range.clampedTo(ByteLength{6}) == ByteRange{ByteIndex{4}, ByteLength{2}});
        REQUIRE(range.withOrigin(ByteIndex{10}) == ByteRange{ByteIndex{14}, ByteLength{3}});

        auto noRange = ByteRange::noRange();
        REQUIRE(!noRange.isValid());
        REQUIRE(!noRange.contains(ByteIndex::zero()));
        REQUIRE(noRange.endIndex().isNoIndex());
        REQUIRE(noRange.clampedTo(ByteLength{10}).isEmpty());
        REQUIRE(!noRange.withOrigin(ByteIndex{10}).isValid());
    }
};
