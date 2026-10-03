// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <concepts>
#include <utility>

using namespace el::unit;

TESTED_TARGETS(IntegerUnitCount IntegerUnitRange)
class IntegerUnitRemainingTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        static_assert(ByteCount{9}.remainingFrom(ByteIndex{4}) == ByteCount{5});
        static_assert(ByteCount::infinite().remainingFrom(ByteIndex::maximum()).isInfinite());
        static_assert(ByteCount::infinite().remainingFrom(ByteIndex::noIndex()).isZero());
        static_assert(ItemCount{9}.remainingFrom(ItemIndex{4}) == ItemCount{5});
        static_assert(std::same_as<decltype(ByteCount{9}.remainingFrom(ByteIndex{4})), ByteCount>);
        static_assert(!std::convertible_to<CpIndex, ByteIndex>);
        static_assert(ByteRange::fromBegin(ByteIndex{4}).beginIndex() == ByteIndex{4});
        static_assert(ByteRange::fromBegin(ByteIndex{4}).isInfinite());
        static_assert(ByteRange::fromBegin(ByteIndex::noIndex()).isNoRange());
        static_assert(ByteRange::fromBegin(ByteIndex::noIndex()).isInfinite());
        static_assert(
            ByteRange{ByteIndex{4}, ByteCount{5}}.remainingFrom(ByteIndex{6}) == ByteRange{ByteIndex{6}, ByteCount{3}});
        static_assert(
            ByteRange::all().remainingFrom(ByteIndex::maximum()) == ByteRange::fromBegin(ByteIndex::maximum()));
        static_assert(noexcept(std::declval<ByteCount>().remainingFrom(std::declval<ByteIndex>())));
        static_assert(noexcept(ByteRange::fromBegin(std::declval<ByteIndex>())));
        static_assert(noexcept(std::declval<ByteRange>().remainingFrom(std::declval<ByteIndex>())));
    }

    void testFiniteAmountRemainders() {
        const auto length = ByteCount{9};
        REQUIRE_EQUAL(length.remainingFrom(ByteIndex::zero()), length);
        REQUIRE_EQUAL(length.remainingFrom(ByteIndex{4}), ByteCount{5});
        REQUIRE_EQUAL(length.remainingFrom(ByteIndex{8}), ByteCount::one());
        for (const auto index : std::array{ByteIndex{9}, ByteIndex{10}, ByteIndex::maximum(), ByteIndex::noIndex()}) {
            REQUIRE_EQUAL(length.remainingFrom(index), ByteCount::zero());
            REQUIRE_EQUAL(ByteCount::zero().remainingFrom(index), ByteCount::zero());
        }
        REQUIRE_EQUAL(ByteCount::zero().remainingFrom(ByteIndex::zero()), ByteCount::zero());
        REQUIRE_EQUAL(ByteCount::maximum().remainingFrom(ByteIndex::maximum()), ByteCount::zero());
        REQUIRE_EQUAL(ByteCount::maximum().remainingFrom(ByteIndex::maximum().decremented()), ByteCount::one());
    }

    void testInfiniteAmountRemainders() {
        for (const auto index : std::array{ByteIndex::zero(), ByteIndex{42}, ByteIndex::maximum()}) {
            REQUIRE_EQUAL(ByteCount::infinite().remainingFrom(index), ByteCount::infinite());
        }
        REQUIRE_EQUAL(ByteCount::infinite().remainingFrom(ByteIndex::noIndex()), ByteCount::zero());
    }

    void testFiniteRangeRemainders() {
        const auto range = ByteRange{ByteIndex{4}, ByteIndex{9}};
        REQUIRE_EQUAL(range.remainingFrom(ByteIndex::zero()), range);
        REQUIRE_EQUAL(range.remainingFrom(ByteIndex{4}), range);
        REQUIRE_EQUAL(range.remainingFrom(ByteIndex{6}), (ByteRange{ByteIndex{6}, ByteIndex{9}}));
        for (const auto index : std::array{ByteIndex{9}, ByteIndex{10}, ByteIndex::maximum()}) {
            REQUIRE_EQUAL(range.remainingFrom(index), ByteRange::emptyAt(ByteIndex{9}));
        }
        REQUIRE_EQUAL(range.remainingFrom(ByteIndex::noIndex()), ByteRange::noRange());
        const auto empty = ByteRange::emptyAt(ByteIndex{4});
        REQUIRE_EQUAL(empty.remainingFrom(ByteIndex::zero()), empty);
        REQUIRE_EQUAL(empty.remainingFrom(ByteIndex{4}), empty);
        REQUIRE_EQUAL(empty.remainingFrom(ByteIndex{5}), empty);
        const auto last = ByteRange{ByteIndex::maximum().decremented(), ByteCount::one()};
        REQUIRE_EQUAL(last.remainingFrom(last.beginIndex()), last);
        REQUIRE_EQUAL(last.remainingFrom(ByteIndex::maximum()), ByteRange::emptyAt(ByteIndex::maximum()));
    }

    void testOpenAndInvalidRangeRemainders() {
        const auto range = ByteRange::fromBegin(ByteIndex{4});
        REQUIRE(range.isValid());
        REQUIRE(range.endIndex().isNoIndex());
        REQUIRE_EQUAL(range.remainingFrom(ByteIndex::zero()), range);
        REQUIRE_EQUAL(range.remainingFrom(ByteIndex{4}), range);
        REQUIRE_EQUAL(range.remainingFrom(ByteIndex{6}), ByteRange::fromBegin(ByteIndex{6}));
        REQUIRE_EQUAL(range.remainingFrom(ByteIndex::maximum()), ByteRange::fromBegin(ByteIndex::maximum()));
        REQUIRE_EQUAL(range.remainingFrom(ByteIndex::noIndex()), ByteRange::noRange());
        REQUIRE_EQUAL(ByteRange::fromBegin(ByteIndex::zero()), ByteRange::all());
        const auto absent = ByteRange::fromBegin(ByteIndex::noIndex());
        REQUIRE_EQUAL(absent.beginIndex(), ByteIndex::noIndex());
        REQUIRE(absent.isNoRange());
        REQUIRE(absent.isInfinite());
        REQUIRE_FALSE(absent.isValid());
        const auto overflow = ByteRange{ByteIndex::maximum(), ByteCount::one()};
        for (const auto index : std::array{ByteIndex::zero(), ByteIndex::maximum(), ByteIndex::noIndex()}) {
            REQUIRE_EQUAL(ByteRange::noRange().remainingFrom(index), ByteRange::noRange());
            REQUIRE_EQUAL(overflow.remainingFrom(index), ByteRange::noRange());
        }
    }
};
