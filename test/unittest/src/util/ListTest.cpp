// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MoveAwareTestValue.hpp"

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/unit/ItemCount.hpp>
#include <erbsland/unit/ItemIndex.hpp>
#include <erbsland/unit/ItemRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/List.hpp>
#include <erbsland/util/LoopStatus.hpp>

#include <compare>
#include <concepts>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

using el::unit::ItemCount;
using el::unit::ItemIndex;
using el::unit::ItemRange;
using el::util::LoopResult;
using el::util::LoopStatus;

TESTED_TARGETS(List ItemUnit ItemIndex ItemCount ItemRange ItemOffset)
class ListTest final : public el::UnitTest {
public:
    using IntList = el::util::List<int>;
    using MoveList = el::util::List<erbsland::test::MoveAwareTestValue>;
    using MoveValue = erbsland::test::MoveAwareTestValue;

    void testSharedDefaultStorage() {
        auto first = IntList{};
        const auto second = IntList{};

        REQUIRE_EQUAL(&first.toRawValue(), &second.toRawValue());

        first.append(1);

        REQUIRE_NOT_EQUAL(&first.toRawValue(), &second.toRawValue());
        REQUIRE_EQUAL(first.toStdVector(), (std::vector<int>{1}));
        REQUIRE(second.isEmpty());
    }

    void testConstructionAndRawInterop() {
        const auto empty = IntList{};
        REQUIRE(empty.count().isZero());
        REQUIRE_EQUAL(empty.first(), 0);
        REQUIRE_EQUAL(empty.last(), 0);

        const auto single = IntList{7};
        REQUIRE_EQUAL(single.count(), ItemCount{1});
        REQUIRE_EQUAL(single.first(), 7);

        const auto repeated = IntList{ItemCount{3}, 4};
        REQUIRE_EQUAL(repeated.toStdVector(), (std::vector<int>{4, 4, 4}));

        const auto raw = std::vector<int>{1, 2, 3};
        const auto fromRaw = IntList{raw};
        REQUIRE_EQUAL(fromRaw.toRawValue(), raw);
        REQUIRE_EQUAL(fromRaw.toStdVector(), raw);
        const auto distinct = IntList{{3, 1, 3, 2, 1}}.toStdSet();
        REQUIRE_EQUAL(distinct, (std::set<int>{1, 2, 3}));
    }

    void testCopyOnWriteMutation() {
        auto first = IntList{{1, 2, 3}};
        auto second = first;

        second.set(ItemIndex{1}, 9).append(4);

        REQUIRE_EQUAL(first.toStdVector(), (std::vector<int>{1, 2, 3}));
        REQUIRE_EQUAL(second.toStdVector(), (std::vector<int>{1, 9, 3, 4}));
    }

    void testReferenceAccess() {
        static_assert(std::same_as<decltype(std::declval<const IntList &>().getRef(ItemIndex{})), const int &>);
        static_assert(std::same_as<decltype(std::declval<const IntList &>().getRefOrThrow(ItemIndex{})), const int &>);

        auto value = MoveValue{42};
        const auto counts = value.counts();
        auto list = MoveList{};
        list.append(std::move(value));
        const auto copiesBeforeAccess = counts->copies;

        const auto &reference = list.getRef(ItemIndex{});
        REQUIRE_EQUAL(reference.value(), 42);
        REQUIRE_EQUAL(counts->copies, copiesBeforeAccess);
        REQUIRE_EQUAL(&reference, &list.getRefOrThrow(ItemIndex{}));

        const auto &defaultReference = list.getRef(ItemIndex{1});
        REQUIRE_EQUAL(defaultReference.value(), 0);
        REQUIRE_EQUAL(&defaultReference, &list.getRef(ItemIndex::noIndex()));

        auto sharedCopy = list;
        const auto &sharedReference = sharedCopy.getRefOrThrow(ItemIndex{});
        REQUIRE_EQUAL(&sharedReference, &reference);
        sharedCopy.set(ItemIndex{}, MoveValue{7});
        REQUIRE_EQUAL(reference.value(), 42);

        REQUIRE_THROWS_AS(el::err::OutOfRangeError, list.getRefOrThrow(ItemIndex{1}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, list.getRefOrThrow(ItemIndex::noIndex()));
    }

    void testOperatorsAndChanges() {
        auto list = IntList{{2, 3}};

        list.prepend(1).append(4).insert(ItemIndex{2}, 9);
        REQUIRE_EQUAL(list.toStdVector(), (std::vector<int>{1, 2, 9, 3, 4}));

        const auto combined = IntList{{1, 2}} + IntList{{3, 4}};
        REQUIRE_EQUAL(combined.toStdVector(), (std::vector<int>{1, 2, 3, 4}));
        REQUIRE_EQUAL((0 + combined).toStdVector(), (std::vector<int>{0, 1, 2, 3, 4}));

        auto selfAppend = IntList{{1, 2}};
        selfAppend.append(selfAppend);
        REQUIRE_EQUAL(selfAppend.toStdVector(), (std::vector<int>{1, 2, 1, 2}));
    }

    void testMoveAwareChanges() {
        auto list = MoveList{};

        auto appended = MoveValue{1};
        const auto appendedCounts = appended.counts();
        list.append(std::move(appended));
        REQUIRE_EQUAL(appendedCounts->copies, 0);
        REQUIRE_EQUAL(list.getRefOrThrow(ItemIndex{0}).value(), 1);

        auto prepended = MoveValue{2};
        const auto prependedCounts = prepended.counts();
        list.prepend(std::move(prepended));
        REQUIRE_EQUAL(prependedCounts->copies, 0);
        REQUIRE_EQUAL(list.getRefOrThrow(ItemIndex{0}).value(), 2);

        auto inserted = MoveValue{3};
        const auto insertedCounts = inserted.counts();
        list.insert(ItemIndex{1}, std::move(inserted));
        REQUIRE_EQUAL(insertedCounts->copies, 0);
        REQUIRE_EQUAL(list.getRefOrThrow(ItemIndex{1}).value(), 3);

        auto replacement = MoveValue{4};
        const auto replacementCounts = replacement.counts();
        list.set(ItemIndex{2}, std::move(replacement));
        REQUIRE_EQUAL(replacementCounts->copies, 0);
        REQUIRE_EQUAL(list.getRefOrThrow(ItemIndex{2}).value(), 4);

        auto plusValue = MoveValue{5};
        const auto plusCounts = plusValue.counts();
        const auto plusList = list + std::move(plusValue);
        REQUIRE_EQUAL(plusCounts->copies, 0);
        REQUIRE_EQUAL(plusList.getRefOrThrow(ItemIndex{3}).value(), 5);

        auto frontValue = MoveValue{6};
        const auto frontCounts = frontValue.counts();
        const auto frontList = std::move(frontValue) + list;
        REQUIRE_EQUAL(frontCounts->copies, 0);
        REQUIRE_EQUAL(frontList.getRefOrThrow(ItemIndex{0}).value(), 6);

        auto plusAssignValue = MoveValue{7};
        const auto plusAssignCounts = plusAssignValue.counts();
        list += std::move(plusAssignValue);
        REQUIRE_EQUAL(plusAssignCounts->copies, 0);
        REQUIRE_EQUAL(list.getRefOrThrow(ItemIndex{3}).value(), 7);
    }

    void testSliceRemoveAndTake() {
        auto list = IntList{{1, 2, 3, 4, 5}};

        REQUIRE_EQUAL(list.slice(ItemRange{ItemIndex{1}, ItemCount{3}}).toStdVector(), (std::vector<int>{2, 3, 4}));
        REQUIRE_EQUAL(list.slice(ItemRange{ItemIndex{4}, ItemCount{3}}).toStdVector(), (std::vector<int>{5}));
        REQUIRE_EQUAL(
            list.slice(ItemRange{ItemIndex{2}, ItemCount::infinite()}).toStdVector(), (std::vector<int>{3, 4, 5}));
        REQUIRE(list.slice(ItemRange{ItemIndex{5}, ItemCount{1}}).count().isZero());
        REQUIRE_EQUAL(list.prefix(ItemCount{2}).toStdVector(), (std::vector<int>{1, 2}));
        REQUIRE_EQUAL(list.prefix(ItemCount{20}).toStdVector(), (std::vector<int>{1, 2, 3, 4, 5}));
        REQUIRE_EQUAL(list.suffix(ItemCount{2}).toStdVector(), (std::vector<int>{4, 5}));
        REQUIRE_EQUAL(list.suffix(ItemCount{20}).toStdVector(), (std::vector<int>{1, 2, 3, 4, 5}));

        const auto [first, rest] = list.sliceFirst();
        REQUIRE_EQUAL(first, 1);
        REQUIRE_EQUAL(rest.toStdVector(), (std::vector<int>{2, 3, 4, 5}));

        list.remove(ItemRange{ItemIndex{2}, ItemCount{99}});
        REQUIRE_EQUAL(list.toStdVector(), (std::vector<int>{1, 2}));

        list = IntList{{1, 2, 3, 4, 5}};
        REQUIRE_EQUAL(list.take(ItemIndex{2}), 3);
        REQUIRE_EQUAL(list.toStdVector(), (std::vector<int>{1, 2, 4, 5}));
        REQUIRE_EQUAL(list.take(ItemRange{ItemIndex{1}, ItemCount{99}}).toStdVector(), (std::vector<int>{2, 4, 5}));
        REQUIRE_EQUAL(list.toStdVector(), (std::vector<int>{1}));
        REQUIRE_EQUAL(list.take(ItemIndex{9}), 0);
    }

    void testAlgorithmsAndCallbacks() {
        const auto list = IntList{{1, 1, 2, 3, 3}};
        auto sum = 0;
        list.forEach([&](int value) { sum += value; });
        REQUIRE_EQUAL(sum, 10);

        auto visited = std::vector<int>{};
        list.forEach([&](int value) -> LoopStatus {
            visited.push_back(value);
            return value == 2 ? LoopStatus::Stop : LoopStatus::Continue;
        });
        REQUIRE_EQUAL(visited, (std::vector<int>{1, 1, 2}));

        auto reverseVisited = std::vector<int>{};
        list.forEachReverse([&](int value) { reverseVisited.push_back(value); });
        REQUIRE_EQUAL(reverseVisited, (std::vector<int>{3, 3, 2, 1, 1}));

        auto indexedValues = std::vector<int>{};
        auto indexes = std::vector<uint64_t>{};
        REQUIRE_EQUAL(
            list.forEach([&](int value, const ItemIndex index) -> void {
                indexedValues.push_back(value);
                indexes.push_back(index.toRawValue());
            }),
            LoopResult::Success);
        REQUIRE_EQUAL(indexedValues, (std::vector<int>{1, 1, 2, 3, 3}));
        REQUIRE_EQUAL(indexes, (std::vector<uint64_t>{0U, 1U, 2U, 3U, 4U}));

        auto reverseIndexes = std::vector<uint64_t>{};
        REQUIRE_EQUAL(
            list.forEachReverse(
                [&](int, const ItemIndex index) -> void { reverseIndexes.push_back(index.toRawValue()); }),
            LoopResult::Success);
        REQUIRE_EQUAL(reverseIndexes, (std::vector<uint64_t>{4U, 3U, 2U, 1U, 0U}));

        const auto indexedStop = list.forEach([](int, const ItemIndex index) -> LoopStatus {
            return index == ItemIndex{2U} ? LoopStatus::Stop : LoopStatus::Continue;
        });
        REQUIRE_EQUAL(indexedStop, LoopResult::Stopped);

        REQUIRE_EQUAL(
            list.mapped([](int value) -> int { return value * 2; }).toStdVector(), (std::vector<int>{2, 2, 4, 6, 6}));
        REQUIRE_EQUAL(list.collapsed().toStdVector(), (std::vector<int>{1, 2, 3}));

        auto mutableList = list;
        const auto taken = mutableList.takeIf([](int value) -> bool { return value % 2 == 1; });
        REQUIRE_EQUAL(taken.toStdVector(), (std::vector<int>{1, 1, 3, 3}));
        REQUIRE_EQUAL(mutableList.toStdVector(), (std::vector<int>{2}));
    }

    void testSortSearchAndPredicates() {
        const auto list = IntList{{3, 1, 2, 1}};

        REQUIRE_EQUAL(list.sorted().toStdVector(), (std::vector<int>{1, 1, 2, 3}));
        REQUIRE_EQUAL(list.findFirst(1), ItemIndex{1});
        REQUIRE_EQUAL(list.findFirst(1, ItemIndex{2}), ItemIndex{3});
        REQUIRE_EQUAL(list.findLast(1), ItemIndex{3});
        REQUIRE_EQUAL(list.findLast(1, ItemIndex{2}), ItemIndex{1});
        REQUIRE(list.findLast(1, ItemIndex{9}).isNoIndex());

        REQUIRE(list.contains(2));
        REQUIRE(list.anyOf([](int value) -> bool { return value == 3; }));
        REQUIRE(list.allOf([](int value) -> bool { return value > 0; }));
        REQUIRE(list.noneOf([](int value) -> bool { return value < 0; }));
        REQUIRE_EQUAL(list.compare(IntList{{3, 1, 2, 2}}), std::strong_ordering::less);
    }
};
