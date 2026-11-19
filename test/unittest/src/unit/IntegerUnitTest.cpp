// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all_unit.hpp>
#include <erbsland/ArgumentCount.hpp>
#include <erbsland/ArgumentIndex.hpp>
#include <erbsland/CodeContinuousRange.hpp>
#include <erbsland/CodeLocation.hpp>
#include <erbsland/ColumnCount.hpp>
#include <erbsland/ColumnIndex.hpp>
#include <erbsland/ColumnOffset.hpp>
#include <erbsland/ColumnRange.hpp>
#include <erbsland/ColumnUnit.hpp>
#include <erbsland/LineCount.hpp>
#include <erbsland/LineIndex.hpp>
#include <erbsland/LineOffset.hpp>
#include <erbsland/LineRange.hpp>
#include <erbsland/LineUnit.hpp>
#include <erbsland/SaturatingInteger.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

using namespace el::unit;

TESTED_TARGETS(
    IntegerUnit ByteUnit U16Unit CharUnit LineUnit ColumnUnit CodeLocation CodeContinuousRange IntegerUnitIndex
        IntegerUnitCount IntegerUnitOffset IntegerUnitRange)
class IntegerUnitTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {

        static_assert(std::same_as<ArgumentIndex, IntegerUnitIndex<ArgumentUnit>>);
        static_assert(std::same_as<ArgumentCount, IntegerUnitCount<ArgumentUnit>>);
        static_assert(ArgumentIndex{2} + ArgumentCount{3} == ArgumentIndex{5});
        static_assert(std::same_as<ByteIndex, IntegerUnitIndex<ByteUnit>>);
        static_assert(std::same_as<ByteCount, IntegerUnitCount<ByteUnit>>);
        static_assert(std::same_as<ByteOffset, IntegerUnitOffset<ByteUnit>>);
        static_assert(std::same_as<ByteRange, IntegerUnitRange<ByteUnit>>);
        static_assert(std::same_as<CpIndex, IntegerUnitIndex<CpUnit>>);
        static_assert(std::same_as<CpCount, IntegerUnitCount<CpUnit>>);
        static_assert(std::same_as<CpOffset, IntegerUnitOffset<CpUnit>>);
        static_assert(std::same_as<CpRange, IntegerUnitRange<CpUnit>>);
        static_assert(std::same_as<LineIndex, IntegerUnitIndex<LineUnit>>);
        static_assert(std::same_as<LineCount, IntegerUnitCount<LineUnit>>);
        static_assert(std::same_as<LineOffset, IntegerUnitOffset<LineUnit>>);
        static_assert(std::same_as<LineRange, IntegerUnitRange<LineUnit>>);
        static_assert(std::same_as<ColumnIndex, IntegerUnitIndex<ColumnUnit>>);
        static_assert(std::same_as<ColumnCount, IntegerUnitCount<ColumnUnit>>);
        static_assert(std::same_as<ColumnOffset, IntegerUnitOffset<ColumnUnit>>);
        static_assert(std::same_as<ColumnRange, IntegerUnitRange<ColumnUnit>>);
        static_assert(std::same_as<typename ByteIndex::Value, uint64_t>);
        static_assert(std::same_as<typename ByteCount::Value, uint64_t>);
        static_assert(std::same_as<typename ByteOffset::Value, int64_t>);
        static_assert(std::same_as<typename CpIndex::Value, uint64_t>);
        static_assert(std::same_as<typename CpCount::Value, uint64_t>);
        static_assert(std::same_as<typename CpOffset::Value, int64_t>);
        static_assert(std::same_as<typename U16Index::Value, uint64_t>);
        static_assert(std::same_as<typename U16Count::Value, uint64_t>);
        static_assert(std::same_as<typename U16Offset::Value, int64_t>);

        static_assert(!std::equality_comparable_with<ByteIndex, CpIndex>);
        static_assert(!std::equality_comparable_with<ByteCount, CpCount>);
        static_assert(!std::equality_comparable_with<ByteOffset, CpOffset>);
        static_assert(!std::equality_comparable_with<ByteRange, CpRange>);
        static_assert(LineIndex::noIndex().isNoIndex());
        static_assert(ColumnIndex::noIndex().isNoIndex());
        static_assert(LineRange{LineIndex{3U}, LineCount{2U}}.endIndex() == LineIndex{5U});
        static_assert(ColumnRange{ColumnIndex{4U}, ColumnCount{3U}}.endIndex() == ColumnIndex{7U});
        static_assert(CodeLocation{}.line().isNoIndex());
        static_assert(CodeLocation{}.column().isNoIndex());
        static_assert(CodeLocation{}.position().isNoIndex());
        static_assert(
            CodeContinuousRange{.begin = CodeLocation{LineIndex{1U}}, .end = CodeLocation{LineIndex{2U}}}.end.line() ==
            LineIndex{2U});

        static_assert(ByteCount::zero().isZero());
        static_assert(ByteCount::one().isOne());
        static_assert(ByteCount::maximum().isMaximum());
        static_assert(ByteCount::infinite().isInfinite());
        static_assert(ByteCount{2}.added(ByteCount{3}).toRawValue() == 5U);
        static_assert(ByteCount::maximum().added(ByteCount::one()).isMaximum());
        static_assert((ByteCount{2} + ByteCount{3}) == ByteCount{5});
        static_assert((ByteCount{2} - ByteCount{3}).isZero());
        static_assert((ByteCount{2} * 3U) == ByteCount{6});
        static_assert((3U * ByteCount{2}) == ByteCount{6});
        static_assert((ByteCount{9} / 3U) == ByteCount{3});
        static_assert((ByteCount{10} % 4U) == ByteCount{2});
        static_assert((CpCount{2} * el::math::SaturatingInteger<uint16_t>{3U}) == CpCount{6});
        static_assert((el::math::SaturatingInteger<uint16_t>{3U} * CpCount{2}) == CpCount{6});
        static_assert((CpCount{2} * std::size_t{3U}) == CpCount{6});

        static_assert(ByteOffset::zero().isZero());
        static_assert(ByteOffset::one().isOne());
        static_assert(ByteOffset::minusOne().isMinusOne());
        static_assert(ByteOffset{-3}.isNegative());
        static_assert(ByteOffset{3}.isPositive());
        static_assert(ByteOffset{-3}.absoluteLength() == ByteCount{3});
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
        static_assert(ByteIndex{5}.isWithin(ByteCount{6}));
        static_assert(!ByteIndex{6}.isWithin(ByteCount{6}));
        static_assert(!ByteIndex::noIndex().isWithin(ByteCount::infinite()));
        static_assert(ByteIndex{5}.advanced(ByteCount{3}) == ByteIndex{8});
        static_assert(ByteIndex{5}.retreated(ByteCount{8}).isZero());
        static_assert(ByteIndex{5}.moved(ByteOffset{-2}) == ByteIndex{3});
        static_assert([]() {
            auto index = ByteIndex{5};
            index.uncheckedAdvance(ByteCount{3}).uncheckedIncrement();
            return index;
        }() == ByteIndex{9});
        static_assert(ByteIndex{5}.absoluteDistanceTo(ByteIndex{2}) == ByteCount{3});
        static_assert((ByteIndex{5} + ByteCount{3}) == ByteIndex{8});
        static_assert((ByteIndex{5} - ByteCount{8}).isZero());
        static_assert((ByteIndex{5} + ByteOffset{-2}) == ByteIndex{3});
        static_assert((ByteIndex{5} - ByteOffset{-2}) == ByteIndex{7});
        static_assert(ByteIndex{5}.offsetFromZero() == ByteOffset{5});
        static_assert(ByteIndex{5}.offsetTo(ByteIndex{2}) == ByteOffset{-3});
        static_assert(ByteIndex::end(ByteCount::infinite()).isNoIndex());

        static_assert(ByteRange::empty().isEmpty());
        static_assert(ByteRange::all().isInfinite());
        static_assert(ByteRange::all().contains(ByteIndex{42}));
        static_assert(ByteRange{ByteIndex{2}, ByteCount{3}}.contains(ByteIndex{4}));
        static_assert(!ByteRange{ByteIndex{2}, ByteCount{3}}.contains(ByteIndex{5}));
        static_assert(ByteRange{ByteIndex{2}, ByteCount{3}}.endIndex() == ByteIndex{5});
        static_assert(ByteRange::emptyAt(ByteIndex{10}).isWithin(ByteCount{10}));
        static_assert(!ByteRange{ByteIndex{8}, ByteCount{3}}.isWithin(ByteCount{10}));
        static_assert(
            ByteRange{ByteIndex{2}, ByteCount{5}}.clampedTo(ByteCount{4}) == ByteRange{ByteIndex{2}, ByteCount{2}});
        static_assert(
            ByteRange{ByteIndex{2}, ByteCount::infinite()}.clampedTo(ByteCount{4}) ==
            ByteRange{ByteIndex{2}, ByteCount{2}});
        static_assert(
            ByteRange{ByteIndex{5}, ByteCount{1}}.clampedTo(ByteCount{4}) == ByteRange::emptyAt(ByteIndex{4}));
        static_assert(
            ByteRange{ByteIndex{1}, ByteCount{2}}.toAbsolute(ByteIndex{10}) == ByteRange{ByteIndex{11}, ByteCount{2}});
        static_assert((ByteRange{ByteIndex{5}, ByteCount{2}} + ByteOffset{-3}).beginIndex() == ByteIndex{2});
        static_assert((ByteRange{ByteIndex{5}, ByteCount{2}} - ByteOffset{-3}).beginIndex() == ByteIndex{8});
    }

    void testLengthOperators() {

        auto length = ByteCount{2};
        length += ByteCount{3};
        REQUIRE_EQUAL(length, ByteCount{5});
        length -= ByteCount{10};
        REQUIRE(length.isZero());
        --length;
        REQUIRE(length.isZero());
        ++length;
        REQUIRE(length.isOne());
    }

    void testLengthSteps() {
        static_assert(ByteCount{2}.incremented() == ByteCount{3});
        static_assert(ByteCount{2}.decremented() == ByteCount{1});
        static_assert(ByteCount::maximum().incremented().isMaximum());
        static_assert(ByteCount::zero().decremented().isZero());
        static_assert(ByteCount::infinite().incremented().isInfinite());
        static_assert(ByteCount::infinite().decremented().isInfinite());

        auto length = ByteCount::infinite();
        REQUIRE(length.increment().decrement().isInfinite());
        REQUIRE(length.isInfinite());
    }

    void testLengthScalarOperators() {
        using el::math::SaturatingInteger;

        auto length = ByteCount{10};
        REQUIRE_EQUAL(length * 4U, ByteCount{40});
        REQUIRE_EQUAL(4U * length, ByteCount{40});
        REQUIRE_EQUAL(length / 4U, ByteCount{2});
        REQUIRE_EQUAL(length % 4U, ByteCount{2});
        REQUIRE_EQUAL(CpCount{6} * SaturatingInteger<uint16_t>{3U}, CpCount{18});
        REQUIRE_EQUAL(CpCount{6} * std::size_t{3U}, CpCount{18});

        length *= 3U;
        REQUIRE_EQUAL(length, ByteCount{30});
        length /= 4U;
        REQUIRE_EQUAL(length, ByteCount{7});
        length %= 5U;
        REQUIRE_EQUAL(length, ByteCount{2});

        REQUIRE(ByteCount::maximum().multiplied(2U).isMaximum());
        REQUIRE(CpCount::maximum().multiplied(std::numeric_limits<std::size_t>::max()).isMaximum());
        REQUIRE(ByteCount::infinite().multiplied(0U).isInfinite());
        REQUIRE(ByteCount::infinite().divided(2U).isInfinite());
        REQUIRE(ByteCount::infinite().modulo(2U).isInfinite());
    }

    void testLengthThrowingVariants() {

        auto length = ByteCount{2};
        length.addOrThrow(ByteCount{3});
        REQUIRE_EQUAL(length, ByteCount{5});
        REQUIRE_THROWS(ByteCount::maximum().addOrThrow(ByteCount::one()));
        REQUIRE_THROWS(ByteCount::infinite().addedOrThrow(ByteCount::one()));
        REQUIRE_THROWS(ByteCount{2}.subtractOrThrow(ByteCount{3}));
    }

    void testOffsetOperatorsAndThrowingVariants() {

        auto offset = ByteOffset{2};
        offset += ByteOffset{3};
        REQUIRE_EQUAL(offset, ByteOffset{5});
        offset -= ByteOffset{8};
        REQUIRE_EQUAL(offset, ByteOffset{-3});
        REQUIRE_EQUAL((-offset), ByteOffset{3});
        const auto offsetBeforeIncrement = offset++;
        REQUIRE_EQUAL(offsetBeforeIncrement, ByteOffset{-3});
        REQUIRE_EQUAL(offset, ByteOffset{-2});
        REQUIRE_EQUAL((--offset), ByteOffset{-3});

        REQUIRE(ByteOffset::maximum().added(ByteOffset::one()).isMaximum());
        REQUIRE(ByteOffset::minimum().subtracted(ByteOffset::one()).isMinimum());
        REQUIRE_THROWS(ByteOffset::maximum().addOrThrow(ByteOffset::one()));
        REQUIRE_THROWS(ByteOffset::minimum().subtractOrThrow(ByteOffset::one()));
    }

    void testOffsetStepsAndNegation() {
        static_assert(ByteOffset{-2}.incremented() == ByteOffset{-1});
        static_assert(ByteOffset{-2}.decremented() == ByteOffset{-3});
        static_assert(ByteOffset::maximum().incremented().isMaximum());
        static_assert(ByteOffset::minimum().decremented().isMinimum());

        auto offset = ByteOffset{-2};
        REQUIRE_EQUAL(offset.decrement().increment(), ByteOffset{-2});
        offset.negate();
        REQUIRE_EQUAL(offset, ByteOffset{2});
        offset = ByteOffset::minimum();
        REQUIRE(offset.decrement().isMinimum());
        offset.negate();
        REQUIRE(offset.isMaximum());
        REQUIRE(offset.increment().isMaximum());
    }

    void testOffsetScalarOperators() {
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

        auto index = ByteIndex{5};
        index.advanceOrThrow(ByteCount{3});
        REQUIRE_EQUAL(index, ByteIndex{8});
        index.retreatOrThrow(ByteCount{4});
        REQUIRE_EQUAL(index, ByteIndex{4});
        index.moveOrThrow(ByteOffset{-2});
        REQUIRE_EQUAL(index, ByteIndex{2});

        REQUIRE_EQUAL(index.offsetFromZeroOrThrow(), ByteOffset{2});
        REQUIRE_EQUAL(index.offsetToOrThrow(ByteIndex{5}), ByteOffset{3});
        REQUIRE_THROWS(ByteIndex::noIndex().advanceOrThrow(ByteCount::one()));
        REQUIRE_THROWS(ByteIndex{2}.retreatOrThrow(ByteCount{3}));
        REQUIRE_THROWS(ByteIndex::maximum().advanceOrThrow(ByteCount::one()));
        REQUIRE_THROWS(ByteIndex{2}.moveOrThrow(ByteOffset{-3}));
        REQUIRE_THROWS(ByteIndex::noIndex().offsetFromZeroOrThrow());
        REQUIRE_THROWS(ByteIndex::noIndex().offsetToOrThrow(ByteIndex::zero()));
    }

    void testIndexOffsetConversions() {

        static constexpr auto cInt64Max = static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
        REQUIRE_EQUAL(ByteIndex{cInt64Max}.offsetFromZero(), ByteOffset::maximum());
        REQUIRE(ByteIndex{cInt64Max + 1U}.offsetFromZero().isMaximum());

        REQUIRE_EQUAL(CpIndex{5}.offsetTo(CpIndex{2}), CpOffset{-3});
        REQUIRE(CpIndex{0x8000000000000000ULL}.offsetTo(CpIndex{0}).isMinimum());
        REQUIRE(CpIndex{0x8000000000000001ULL}.offsetTo(CpIndex{0}).isMinimum());
        REQUIRE(CpIndex{0}.offsetTo(CpIndex{0x8000000000000000ULL}).isMaximum());
    }

    void testRangeRuntimeBehavior() {

        auto range = ByteRange{ByteIndex{4}, ByteCount{3}};
        REQUIRE(range.contains(ByteIndex{4}));
        REQUIRE(range.contains(ByteIndex{6}));
        REQUIRE(!range.contains(ByteIndex{7}));
        REQUIRE(range.isWithin(ByteCount{7}));
        REQUIRE(!range.isWithin(ByteCount{6}));

        range.move(ByteOffset{-2});
        REQUIRE_EQUAL(range.beginIndex(), ByteIndex{2});
        REQUIRE_EQUAL(range.endIndex(), ByteIndex{5});
        range += ByteOffset{2};
        REQUIRE_EQUAL(range.beginIndex(), ByteIndex{4});
        REQUIRE_EQUAL((range - ByteOffset{2}).beginIndex(), ByteIndex{2});
        REQUIRE_EQUAL(range.clampedTo(ByteCount{6}), (ByteRange{ByteIndex{4}, ByteCount{2}}));
        REQUIRE_EQUAL(range.toAbsolute(ByteIndex{10}), (ByteRange{ByteIndex{14}, ByteCount{3}}));

        auto noRange = ByteRange::noRange();
        REQUIRE(!noRange.isValid());
        REQUIRE(!noRange.contains(ByteIndex::zero()));
        REQUIRE(noRange.endIndex().isNoIndex());
        REQUIRE(noRange.clampedTo(ByteCount{10}).isEmpty());
        REQUIRE(!noRange.toAbsolute(ByteIndex{10}).isValid());
    }

    void testAmountForEach() {
        auto visited = std::vector<ByteIndex>{};
        const auto append = [&](const ByteIndex index) -> void { visited.push_back(index); };
        REQUIRE_EQUAL(ByteCount{3}.forEach(append), el::util::LoopResult::Success);
        REQUIRE_EQUAL(visited, (std::vector<ByteIndex>{ByteIndex{0}, ByteIndex{1}, ByteIndex{2}}));
        visited.clear();
        REQUIRE_EQUAL(ByteCount::zero().forEach(append), el::util::LoopResult::Success);
        REQUIRE_EQUAL(ByteCount::infinite().forEach(append), el::util::LoopResult::Error);
        REQUIRE(visited.empty());
        REQUIRE_EQUAL(
            ByteCount::maximum().forEach([&](const ByteIndex index) -> el::util::LoopStatus {
                visited.push_back(index);
                if (index == ByteIndex{2}) {
                    return el::util::LoopStatus::Stop;
                }
                return el::util::LoopStatus::Continue;
            }),
            el::util::LoopResult::Stopped);
        REQUIRE_EQUAL(visited.size(), std::size_t{3});
        REQUIRE_EQUAL(
            ByteCount{1}.forEach([](ByteIndex) -> el::util::LoopStatus { return el::util::LoopStatus::Error; }),
            el::util::LoopResult::Error);
        // Iteration captures the original bound, even if a callback changes the amount.
        auto length = ByteCount{3};
        visited.clear();
        length.forEach([&](const ByteIndex index) -> void {
            append(index);
            length = ByteCount::zero();
        });
        REQUIRE_EQUAL(visited.size(), std::size_t{3});
    }

    void testIterationCallbackContracts() {
        auto calls = std::size_t{};
        const auto callback = [&](ByteIndex &&) -> void { ++calls; };
        REQUIRE_EQUAL(ByteCount{2}.forEach(callback), el::util::LoopResult::Success);
        REQUIRE_EQUAL((ByteRange{ByteIndex{2}, ByteCount{2}}.forEach(callback)), el::util::LoopResult::Success);
        REQUIRE_EQUAL(calls, std::size_t{4});
        REQUIRE_THROWS(ByteCount{1}.forEach([](ByteIndex) -> void { throw std::runtime_error{"callback"}; }));
        REQUIRE_THROWS(ByteRange::all().forEach([](ByteIndex) -> void { throw std::runtime_error{"callback"}; }));
    }

    void testUncheckedMovement() {
        static_assert(noexcept(ByteIndex{1U, ByteIndex::Unchecked{}}));
        static_assert(ByteIndex{9}.uncheckedAdvanced(ByteCount{3}) == ByteIndex{12});
        static_assert(ByteIndex{9}.uncheckedRetreated(ByteCount{3}) == ByteIndex{6});
        static_assert(ByteIndex{9}.uncheckedMoved(ByteOffset{-3}) == ByteIndex{6});
        static_assert(ByteIndex{9}.uncheckedIncremented() == ByteIndex{10});
        static_assert(ByteIndex{9}.uncheckedDecremented() == ByteIndex{8});
        static_assert(ByteIndex{uint64_t{1} << 63}.uncheckedMoved(ByteOffset::minimum()).isZero());
        static_assert(ByteCount{9}.uncheckedAdded(ByteCount{3}) == ByteCount{12});
        static_assert(ByteCount{9}.uncheckedSubtracted(ByteCount{3}) == ByteCount{6});
        static_assert(ByteCount::maximum().decremented().uncheckedIncremented().isMaximum());
        static_assert(ByteCount::one().uncheckedDecremented().isZero());
        auto index = ByteIndex::maximum().decremented();
        REQUIRE_EQUAL(index.uncheckedIncrement(), ByteIndex::maximum());
        REQUIRE_EQUAL(index.uncheckedDecrement(), ByteIndex::maximum().decremented());
        index = ByteIndex{9};
        index.uncheckedRetreat(ByteCount{3}).uncheckedMove(ByteOffset{-6});
        REQUIRE(index.isZero());
        auto length = ByteCount::maximum();
        length.uncheckedDecrement().uncheckedIncrement();
        REQUIRE(length.isMaximum());
        length = ByteCount{9};
        length.uncheckedAdd(ByteCount{3}).uncheckedSubtract(ByteCount{12});
        REQUIRE(length.isZero());
    }

    void testArithmeticBoundaryMatrix() {
        static_assert(ByteCount{3}.multiplied(ByteCount::cRawInfinite / 3U).isMaximum());
        const auto unsignedValues = std::array<uint64_t, 8>{
            0,
            1,
            2,
            (uint64_t{1} << 63) - 1,
            uint64_t{1} << 63,
            ByteCount::cRawMaximum - 1,
            ByteCount::cRawMaximum,
            ByteCount::cRawInfinite};
        for (const auto first : unsignedValues) {
            for (const auto second : unsignedValues) {
                const auto index = ByteIndex{first, ByteIndex::Unchecked{}};
                const auto length = ByteCount{first, ByteCount::Unchecked{}};
                const auto step = ByteCount{second, ByteCount::Unchecked{}};
                auto expectedAdvance = first;
                auto expectedRetreat = first;
                if (index.isValid()) {
                    expectedAdvance =
                        el::math::saturatingAddBounded(first, second, uint64_t{0}, ByteIndex::cRawMaximum);
                    expectedRetreat =
                        el::math::saturatingSubtractBounded(first, second, uint64_t{0}, ByteIndex::cRawMaximum);
                }
                REQUIRE_EQUAL(index.advanced(step).toRawValue(), expectedAdvance);
                REQUIRE_EQUAL(index.retreated(step).toRawValue(), expectedRetreat);
                REQUIRE_EQUAL(index.isWithin(step), first < second);
                if (length.isInfinite() || step.isInfinite()) {
                    REQUIRE(length.added(step).isInfinite());
                } else {
                    REQUIRE_EQUAL(length.added(step).toRawValue(), expectedAdvance);
                }
                REQUIRE_EQUAL(length.subtracted(step).toRawValue(), expectedRetreat);
                if (length.isFinite()) {
                    REQUIRE_EQUAL(
                        length.multiplied(second).toRawValue(),
                        el::math::saturatingMultiplyBounded(first, second, uint64_t{0}, ByteCount::cRawMaximum));
                    if (second != 0) {
                        REQUIRE_EQUAL(length.divided(second).toRawValue(), first / second);
                        REQUIRE_EQUAL(length.modulo(second).toRawValue(), first % second);
                    }
                }
            }
        }
        const auto signedValues = std::array<int64_t, 7>{
            ByteOffset::cRawMinimum,
            ByteOffset::cRawMinimum + 1,
            -1,
            0,
            1,
            ByteOffset::cRawMaximum - 1,
            ByteOffset::cRawMaximum};
        for (const auto first : signedValues) {
            for (const auto second : signedValues) {
                const auto offset = ByteOffset{first, ByteOffset::Unchecked{}};
                const auto other = ByteOffset{second, ByteOffset::Unchecked{}};
                REQUIRE_EQUAL(
                    offset.added(other).toRawValue(),
                    el::math::saturatingAddBounded(first, second, ByteOffset::cRawMinimum, ByteOffset::cRawMaximum));
                REQUIRE_EQUAL(
                    offset.subtracted(other).toRawValue(),
                    el::math::saturatingSubtractBounded(
                        first, second, ByteOffset::cRawMinimum, ByteOffset::cRawMaximum));
                if (second != 0) {
                    REQUIRE_EQUAL(
                        offset.divided(second).toRawValue(),
                        el::math::saturatingDivideBounded(
                            first, second, ByteOffset::cRawMinimum, ByteOffset::cRawMaximum));
                    REQUIRE_EQUAL(
                        offset.modulo(second).toRawValue(),
                        el::math::saturatingModuloBounded(
                            first, second, ByteOffset::cRawMinimum, ByteOffset::cRawMaximum));
                }
                for (const auto rawIndex : unsignedValues) {
                    const auto index = ByteIndex{rawIndex, ByteIndex::Unchecked{}};
                    if (index.isValid()) {
                        REQUIRE_EQUAL(
                            index.moved(offset).toRawValue(),
                            el::math::saturatingAddBounded(rawIndex, first, uint64_t{0}, ByteIndex::cRawMaximum));
                    } else {
                        REQUIRE(index.moved(offset).isNoIndex());
                    }
                }
            }
        }
    }

    void testRangeForEach() {
        auto visited = std::vector<uint64_t>{};
        const auto completed = ByteRange{ByteIndex{4}, ByteCount{3}}.forEach(
            [&](const ByteIndex index) -> void { visited.push_back(index.toRawValue()); });
        REQUIRE_EQUAL(completed, el::util::LoopResult::Success);
        REQUIRE_EQUAL(visited, (std::vector<uint64_t>{4U, 5U, 6U}));

        auto visitCount = std::size_t{};
        const auto stopped =
            ByteRange{ByteIndex{4}, ByteCount{3}}.forEach([&](const ByteIndex index) -> el::util::LoopStatus {
                ++visitCount;
                return index == ByteIndex{5} ? el::util::LoopStatus::Stop : el::util::LoopStatus::Continue;
            });
        REQUIRE_EQUAL(stopped, el::util::LoopResult::Stopped);
        REQUIRE_EQUAL(visitCount, std::size_t{2U});

        const auto failed = ByteRange{ByteIndex{4}, ByteCount{3}}.forEach(
            [](const ByteIndex) -> el::util::LoopStatus { return el::util::LoopStatus::Error; });
        REQUIRE_EQUAL(failed, el::util::LoopResult::Error);

        auto called = false;
        REQUIRE_EQUAL(
            ByteRange::emptyAt(ByteIndex{4}).forEach([&](const ByteIndex) -> void { called = true; }),
            el::util::LoopResult::Success);
        REQUIRE_FALSE(called);
        REQUIRE_EQUAL(ByteRange::noRange().forEach([](const ByteIndex) -> void {}), el::util::LoopResult::Error);

        auto maximumIndex = ByteIndex{};
        const auto maximumResult = ByteRange{ByteIndex::maximum(), ByteCount::infinite()}.forEach(
            [&](const ByteIndex index) -> void { maximumIndex = index; });
        REQUIRE_EQUAL(maximumResult, el::util::LoopResult::Success);
        REQUIRE_EQUAL(maximumIndex, ByteIndex::maximum());
    }

    void testRangeForEachNearMaximum() {
        auto visited = std::vector<ByteIndex>{};
        const auto begin = ByteIndex::maximum().decremented();
        const auto append = [&](const ByteIndex index) -> void { visited.push_back(index); };
        REQUIRE_EQUAL((ByteRange{begin, ByteCount::one()}.forEach(append)), el::util::LoopResult::Success);
        REQUIRE_EQUAL(visited, (std::vector<ByteIndex>{begin}));
        visited.clear();
        REQUIRE_EQUAL((ByteRange{begin, ByteCount::infinite()}.forEach(append)), el::util::LoopResult::Success);
        REQUIRE_EQUAL(visited, (std::vector<ByteIndex>{begin, ByteIndex::maximum()}));
    }

    void testSafeConstructionAndMovement() {
        static_assert(ByteIndex(42) == ByteIndex{42});
        static_assert(ByteCount(42) == ByteCount{42});
        static_assert(ByteOffset(-42) == ByteOffset{-42});
        static_assert(ByteOffset{std::numeric_limits<int64_t>::min()} == ByteOffset::minimum());
        static_assert(!std::constructible_from<ByteIndex, double>);
        static_assert(!std::constructible_from<ByteCount, bool>);
        static_assert(!std::constructible_from<ByteOffset, float>);
        static_assert(sizeof(CpIndex) == 8 && sizeof(CpRange) == 16);
        static_assert(!ByteIndex{2}.canRetreat(ByteCount{5}, ByteCount{10}));
        static_assert(ByteIndex{10}.canRetreat(ByteCount{1}, ByteCount{10}));
        static_assert(ByteIndex{2}.wouldRetreatSaturate(ByteCount{5}));
        static_assert(ByteIndex::maximum().wouldAdvanceSaturate(ByteCount{1}));
        static_assert(ByteIndex{2}.wouldMoveSaturate(ByteOffset{-3}));
        static_assert(ByteIndex::noIndex().wouldMoveSaturate(ByteOffset{}));
        static_assert(ByteIndex{}.wouldAdvanceSaturate(ByteCount::infinite()));
        static_assert(ByteIndex{2}.mirroredWithin(ByteCount{5}) == ByteIndex{2});
        static_assert(ByteIndex{5}.mirroredWithin(ByteCount{5}).isNoIndex());
        static_assert(ByteIndex{}.mirroredWithin(ByteCount{}).isNoIndex());
        REQUIRE_EQUAL(ByteIndex::fromInteger(-1), ByteIndex{});
        REQUIRE_EQUAL(ByteCount::fromInteger(UINT64_MAX), ByteCount::maximum());
        REQUIRE_EQUAL(ByteOffset::fromInteger(UINT64_MAX), ByteOffset::maximum());
        REQUIRE_THROWS(ByteIndex::fromIntegerOrThrow(-1));
        REQUIRE_THROWS(ByteCount::fromIntegerOrThrow(UINT64_MAX));
        REQUIRE_THROWS(ByteOffset::fromIntegerOrThrow(UINT64_MAX));
        REQUIRE_EQUAL(ByteIndex::noIndex().toSizeTOrThrow(), std::numeric_limits<std::size_t>::max());
    }

    void testRangeBoundsAndTranslation() {
        const auto valid = ByteRange{ByteIndex{4}, ByteCount{3}};
        REQUIRE_EQUAL(valid.toAbsolute(ByteIndex{10}).relativeTo(ByteIndex{10}), valid);
        REQUIRE(valid.relativeTo(ByteIndex{5}).isNoRange());
        REQUIRE(valid.toAbsolute(ByteIndex::maximum()).isNoRange());
        REQUIRE(valid.toAbsolute(ByteIndex::noIndex()).isNoRange());
        REQUIRE_EQUAL(ByteRange::fromBeginEnd(ByteIndex{5}, ByteIndex{2}), ByteRange::emptyAt(ByteIndex{5}));
        REQUIRE_THROWS(ByteRange::fromBeginEndOrThrow(ByteIndex{5}, ByteIndex{2}));
        REQUIRE_THROWS(ByteRange::fromBeginEndOrThrow(ByteIndex::noIndex(), ByteIndex{2}));
        REQUIRE(ByteRange::fromBeginEndOrThrow(ByteIndex{5}, ByteIndex::noIndex()).isInfinite());
        REQUIRE((ByteRange{ByteIndex::noIndex(), ByteIndex::noIndex()}.isNoRange()));
        REQUIRE(ByteRange::emptyAt(ByteIndex::maximum()).isValid());
        const auto invalid = ByteRange{ByteIndex::maximum(), ByteCount{1}};
        REQUIRE_FALSE(invalid.isValid());
        REQUIRE_FALSE(invalid.isNoRange());
        REQUIRE(invalid.endIndex().isNoIndex());
        REQUIRE_FALSE(invalid.contains(ByteIndex::maximum()));
        REQUIRE(invalid.clampedTo(ByteCount::infinite()).isNoRange());
        REQUIRE(invalid.moved(ByteOffset{-1}).isNoRange());
        REQUIRE_EQUAL(invalid.forEach([](ByteIndex) {}), el::util::LoopResult::Error);
        REQUIRE_THROWS(invalid.movedOrThrow(ByteOffset{}));
        const auto moved = valid.advanced(ByteCount::maximum());
        REQUIRE(moved.isValid());
        REQUIRE_EQUAL(moved.length(), valid.length());
        REQUIRE_EQUAL(moved.endIndex(), ByteIndex::maximum());
        REQUIRE_THROWS(valid.advancedOrThrow(ByteCount::maximum()));
        REQUIRE_THROWS(valid.retreatedOrThrow(ByteCount{5}));
        REQUIRE_THROWS(valid.movedOrThrow(ByteOffset::minimum()));
        REQUIRE_EQUAL(valid.moved(ByteOffset::minimum()).beginIndex(), ByteIndex{});
        REQUIRE_EQUAL((valid - ByteOffset::minimum()).endIndex(), ByteIndex{0x8000000000000007ULL});
        REQUIRE(ByteRange::all().contains(ByteIndex::maximum()));
    }

    void testRangeTranslationOperators() {
        constexpr auto range = ByteRange{ByteIndex{4}, ByteCount{3}};
        static_assert(range + ByteCount{2} == ByteRange{ByteIndex{6}, ByteCount{3}});
        static_assert(range - ByteCount{2} == ByteRange{ByteIndex{2}, ByteCount{3}});
        static_assert([range] {
            auto translated = range;
            translated += ByteCount{2};
            translated -= ByteOffset{1};
            translated += ByteOffset{2};
            translated -= ByteCount{3};
            return translated == range;
        }());

        REQUIRE_EQUAL((range + ByteCount::infinite()).endIndex(), ByteIndex::maximum());
        REQUIRE_EQUAL((range - ByteCount::infinite()).beginIndex(), ByteIndex::zero());
        REQUIRE((ByteRange::noRange() + ByteCount{2}).isNoRange());
    }

    void testRangeTranslationPredicates() {
        constexpr auto range = ByteRange{ByteIndex::maximum().decremented(), ByteCount::one()};
        static_assert(!range.beginIndex().wouldAdvanceSaturate(ByteCount::one()));
        static_assert(range.wouldAdvanceSaturate(ByteCount::one()));
        static_assert(range.wouldMoveSaturate(ByteOffset::one()));
        static_assert(range.wouldRetreatSaturate(ByteCount::infinite()));
        static_assert(!range.wouldMoveSaturate(ByteOffset::minimum()));
        static_assert(ByteRange::all().wouldRetreatSaturate(ByteCount::one()));
        static_assert(!ByteRange::all().wouldAdvanceSaturate(ByteCount::maximum()));
        static_assert(ByteRange::noRange().wouldMoveSaturate(ByteOffset::zero()));

        REQUIRE_THROWS(range.advancedOrThrow(ByteCount::one()));
        REQUIRE_THROWS(range.retreatedOrThrow(ByteCount::infinite()));
        REQUIRE_THROWS(range.movedOrThrow(ByteOffset::one()));
        REQUIRE_EQUAL(range.movedOrThrow(ByteOffset::minimum()), range.moved(ByteOffset::minimum()));
    }

    void testRangeFinitenessAndClamping() {
        static_assert(ByteRange::empty().isFinite());
        static_assert(ByteRange::noRange().isFinite());
        static_assert(!ByteRange::all().isFinite());
        static_assert(!ByteRange::fromBegin(ByteIndex::noIndex()).isFinite());

        auto range = ByteRange::fromBegin(ByteIndex{2});
        REQUIRE_EQUAL(range.clampTo(ByteCount{4}), (ByteRange{ByteIndex{2}, ByteCount{2}}));
        REQUIRE_EQUAL(range.clampTo(ByteCount::infinite()), (ByteRange{ByteIndex{2}, ByteCount{2}}));
        REQUIRE_EQUAL(range.clampTo(ByteCount::one()), ByteRange::emptyAt(ByteIndex::one()));
        range = ByteRange{ByteIndex::maximum(), ByteCount::one()};
        REQUIRE(range.clampTo(ByteCount::infinite()).isNoRange());
    }

    void testRangeIntersectionAndContainment() {
        const auto first = ByteRange{ByteIndex{2}, ByteIndex{6}};
        const auto second = ByteRange{ByteIndex{4}, ByteIndex{8}};
        REQUIRE_EQUAL(first.intersection(second), (ByteRange{ByteIndex{4}, ByteIndex{6}}));
        REQUIRE_EQUAL(first.intersection(second), second.intersection(first));
        REQUIRE(first.contains(ByteRange::emptyAt(ByteIndex{6})));
        REQUIRE_FALSE(first.contains(ByteRange::emptyAt(ByteIndex{7})));
        REQUIRE(first.contains(ByteRange{ByteIndex{3}, ByteCount{2}}));
        REQUIRE_FALSE(first.contains(second));
        REQUIRE_EQUAL(first.intersection(ByteRange::emptyAt(ByteIndex{8})), ByteRange::emptyAt(ByteIndex{8}));
        const auto open = ByteRange{ByteIndex{4}, ByteCount::infinite()};
        REQUIRE_EQUAL(open.intersection(first), (ByteRange{ByteIndex{4}, ByteIndex{6}}));
        REQUIRE_EQUAL(open.intersection(ByteRange::all()), open);
        REQUIRE(ByteRange::all().contains(open));
        REQUIRE_FALSE(first.contains(open));
        REQUIRE_FALSE(first.contains(ByteRange::noRange()));
        REQUIRE(first.intersection(ByteRange::noRange()).isNoRange());
    }
};
