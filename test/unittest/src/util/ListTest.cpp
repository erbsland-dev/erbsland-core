// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MoveAwareTestValue.hpp"

#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unit/ElementIndex.hpp>
#include <erbsland/unit/ElementRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/List.hpp>
#include <erbsland/util/LoopStatus.hpp>

#include <compare>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

using el::unit::ElementCount;
using el::unit::ElementIndex;
using el::unit::ElementRange;
using el::util::LoopResult;
using el::util::LoopStatus;

TESTED_TARGETS(List ElementUnit ElementIndex ElementCount ElementRange ElementOffset)
class ListTest final : public el::UnitTest {
public:
    using IntList = el::util::List<int>;
    using MoveList = el::util::List<erbsland::test::MoveAwareTestValue>;
    using MoveValue = erbsland::test::MoveAwareTestValue;

    void testConstructionAndRawInterop() {
        const auto empty = IntList{};
        REQUIRE(empty.count().isZero());
        REQUIRE(empty.first() == 0);
        REQUIRE(empty.last() == 0);

        const auto single = IntList{7};
        REQUIRE_EQUAL(single.count(), ElementCount{1});
        REQUIRE_EQUAL(single.first(), 7);

        const auto repeated = IntList{ElementCount{3}, 4};
        REQUIRE_EQUAL(repeated.toStdVector(), (std::vector<int>{4, 4, 4}));

        const auto raw = std::vector<int>{1, 2, 3};
        const auto fromRaw = IntList{raw};
        REQUIRE_EQUAL(fromRaw.toRawValue(), raw);
        REQUIRE_EQUAL(fromRaw.toStdVector(), raw);
        REQUIRE(IntList{{3, 1, 3, 2, 1}}.toStdSet() == (std::set<int>{1, 2, 3}));
    }

    void testCopyOnWriteMutation() {
        auto first = IntList{{1, 2, 3}};
        auto second = first;

        second.set(ElementIndex{1}, 9).append(4);

        REQUIRE_EQUAL(first.toStdVector(), (std::vector<int>{1, 2, 3}));
        REQUIRE_EQUAL(second.toStdVector(), (std::vector<int>{1, 9, 3, 4}));
    }

    void testOperatorsAndChanges() {
        auto list = IntList{{2, 3}};

        list.prepend(1).append(4).insert(ElementIndex{2}, 9);
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
        REQUIRE_EQUAL(list.toRawValue()[0].value(), 1);

        auto prepended = MoveValue{2};
        const auto prependedCounts = prepended.counts();
        list.prepend(std::move(prepended));
        REQUIRE_EQUAL(prependedCounts->copies, 0);
        REQUIRE_EQUAL(list.toRawValue()[0].value(), 2);

        auto inserted = MoveValue{3};
        const auto insertedCounts = inserted.counts();
        list.insert(ElementIndex{1}, std::move(inserted));
        REQUIRE_EQUAL(insertedCounts->copies, 0);
        REQUIRE_EQUAL(list.toRawValue()[1].value(), 3);

        auto replacement = MoveValue{4};
        const auto replacementCounts = replacement.counts();
        list.set(ElementIndex{2}, std::move(replacement));
        REQUIRE_EQUAL(replacementCounts->copies, 0);
        REQUIRE_EQUAL(list.toRawValue()[2].value(), 4);

        auto plusValue = MoveValue{5};
        const auto plusCounts = plusValue.counts();
        const auto plusList = list + std::move(plusValue);
        REQUIRE_EQUAL(plusCounts->copies, 0);
        REQUIRE_EQUAL(plusList.toRawValue().back().value(), 5);

        auto frontValue = MoveValue{6};
        const auto frontCounts = frontValue.counts();
        const auto frontList = std::move(frontValue) + list;
        REQUIRE_EQUAL(frontCounts->copies, 0);
        REQUIRE_EQUAL(frontList.toRawValue().front().value(), 6);

        auto plusAssignValue = MoveValue{7};
        const auto plusAssignCounts = plusAssignValue.counts();
        list += std::move(plusAssignValue);
        REQUIRE_EQUAL(plusAssignCounts->copies, 0);
        REQUIRE_EQUAL(list.toRawValue().back().value(), 7);
    }

    void testSliceRemoveAndTake() {
        auto list = IntList{{1, 2, 3, 4, 5}};

        REQUIRE_EQUAL(
            list.slice(ElementRange{ElementIndex{1}, ElementCount{3}}).toStdVector(), (std::vector<int>{2, 3, 4}));
        REQUIRE_EQUAL(list.slice(ElementRange{ElementIndex{4}, ElementCount{3}}).toStdVector(), (std::vector<int>{5}));
        REQUIRE_EQUAL(
            list.slice(ElementRange{ElementIndex{2}, ElementCount::infinite()}).toStdVector(),
            (std::vector<int>{3, 4, 5}));
        REQUIRE(list.slice(ElementRange{ElementIndex{5}, ElementCount{1}}).count().isZero());
        REQUIRE_EQUAL(list.prefix(ElementCount{2}).toStdVector(), (std::vector<int>{1, 2}));
        REQUIRE_EQUAL(list.prefix(ElementCount{20}).toStdVector(), (std::vector<int>{1, 2, 3, 4, 5}));
        REQUIRE_EQUAL(list.suffix(ElementCount{2}).toStdVector(), (std::vector<int>{4, 5}));
        REQUIRE_EQUAL(list.suffix(ElementCount{20}).toStdVector(), (std::vector<int>{1, 2, 3, 4, 5}));

        const auto [first, rest] = list.sliceFirst();
        REQUIRE_EQUAL(first, 1);
        REQUIRE_EQUAL(rest.toStdVector(), (std::vector<int>{2, 3, 4, 5}));

        list.remove(ElementRange{ElementIndex{2}, ElementCount{99}});
        REQUIRE_EQUAL(list.toStdVector(), (std::vector<int>{1, 2}));

        list = IntList{{1, 2, 3, 4, 5}};
        REQUIRE_EQUAL(list.take(ElementIndex{2}), 3);
        REQUIRE_EQUAL(list.toStdVector(), (std::vector<int>{1, 2, 4, 5}));
        REQUIRE_EQUAL(
            list.take(ElementRange{ElementIndex{1}, ElementCount{99}}).toStdVector(), (std::vector<int>{2, 4, 5}));
        REQUIRE_EQUAL(list.toStdVector(), (std::vector<int>{1}));
        REQUIRE_EQUAL(list.take(ElementIndex{9}), 0);
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
            list.forEach([&](int value, const ElementIndex index) -> void {
                indexedValues.push_back(value);
                indexes.push_back(index.toRawValue());
            }),
            LoopResult::Success);
        REQUIRE_EQUAL(indexedValues, (std::vector<int>{1, 1, 2, 3, 3}));
        REQUIRE_EQUAL(indexes, (std::vector<uint64_t>{0U, 1U, 2U, 3U, 4U}));

        auto reverseIndexes = std::vector<uint64_t>{};
        REQUIRE_EQUAL(
            list.forEachReverse(
                [&](int, const ElementIndex index) -> void { reverseIndexes.push_back(index.toRawValue()); }),
            LoopResult::Success);
        REQUIRE_EQUAL(reverseIndexes, (std::vector<uint64_t>{4U, 3U, 2U, 1U, 0U}));

        const auto indexedStop = list.forEach([](int, const ElementIndex index) -> LoopStatus {
            return index == ElementIndex{2U} ? LoopStatus::Stop : LoopStatus::Continue;
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
        REQUIRE_EQUAL(list.findFirst(1), ElementIndex{1});
        REQUIRE_EQUAL(list.findFirst(1, ElementIndex{2}), ElementIndex{3});
        REQUIRE_EQUAL(list.findLast(1), ElementIndex{3});
        REQUIRE_EQUAL(list.findLast(1, ElementIndex{2}), ElementIndex{1});
        REQUIRE(list.findLast(1, ElementIndex{9}).isNoIndex());

        REQUIRE(list.contains(2));
        REQUIRE(list.anyOf([](int value) -> bool { return value == 3; }));
        REQUIRE(list.allOf([](int value) -> bool { return value > 0; }));
        REQUIRE(list.noneOf([](int value) -> bool { return value < 0; }));
        REQUIRE_EQUAL(list.compare(IntList{{3, 1, 2, 2}}), std::strong_ordering::less);
    }
};
