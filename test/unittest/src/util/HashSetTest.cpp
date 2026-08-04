// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MoveAwareTestValueHash.hpp"

#include <erbsland/unit/ItemCount.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/HashSet.hpp>
#include <erbsland/util/List.hpp>
#include <erbsland/util/LoopStatus.hpp>

#include <algorithm>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

using el::unit::ItemCount;
using el::util::LoopStatus;

TESTED_TARGETS(HashSet)
class HashSetTest final : public el::UnitTest {
public:
    using IntHashSet = el::util::HashSet<int>;
    using IntList = el::util::List<int>;
    using MoveHashSet = el::util::HashSet<erbsland::test::MoveAwareTestValue, erbsland::test::MoveAwareTestValueHash>;
    using MoveValue = erbsland::test::MoveAwareTestValue;

    void testSharedDefaultStorage() {
        auto first = IntHashSet{};
        const auto second = IntHashSet{};

        REQUIRE_EQUAL(&first.toRawValue(), &second.toRawValue());

        first.insert(1);

        REQUIRE_NOT_EQUAL(&first.toRawValue(), &second.toRawValue());
        REQUIRE(first.contains(1));
        REQUIRE(second.count().isZero());
    }

    void testConstructionAndElements() {
        const auto empty = IntHashSet{};
        REQUIRE(empty.count().isZero());
        REQUIRE_EQUAL(empty.first(), 0);
        REQUIRE_EQUAL(empty.last(), 0);

        const auto set = IntHashSet{{3, 1, 2, 1}};
        REQUIRE_EQUAL(set.count(), ItemCount{3});
        REQUIRE(set.contains(set.first()));
        REQUIRE(set.contains(set.last()));
        const auto stdSet = set.toStdSet();
        const auto stdUnorderedSet = set.toStdUnorderedSet();
        REQUIRE_EQUAL(stdSet, (std::set<int>{1, 2, 3}));
        REQUIRE_EQUAL(stdUnorderedSet, (std::unordered_set<int>{1, 2, 3}));

        auto values = set.toStdVector();
        std::sort(values.begin(), values.end());
        REQUIRE_EQUAL(values, (std::vector<int>{1, 2, 3}));
        const auto fromList = IntHashSet::fromList(IntList{{2, 2, 1}});
        REQUIRE(fromList.compare(IntHashSet{{1, 2}}));
        auto listValues = fromList.toList().toStdVector();
        std::sort(listValues.begin(), listValues.end());
        REQUIRE_EQUAL(listValues, (std::vector<int>{1, 2}));
    }

    void testCopyOnWriteMemoryAndChange() {
        auto first = IntHashSet{{1, 2}};
        auto second = first;

        second.reserve(ItemCount{32}).insert(3).remove(1);

        const auto reservedCapacity = second.capacity().toSizeT();
        REQUIRE_GREATER_EQUAL(reservedCapacity, 32U);
        REQUIRE(first.compare(IntHashSet{{1, 2}}));
        REQUIRE(second.compare(IntHashSet{{2, 3}}));
        REQUIRE(second.tryInsert(4));
        REQUIRE(!second.tryInsert(4));
        REQUIRE(second.tryRemove(4));
        REQUIRE(!second.tryRemove(4));
        second.shrinkToFit();
        const auto capacity = second.capacity().toSizeT();
        const auto count = second.count().toSizeT();
        REQUIRE_GREATER_EQUAL(capacity, count);
    }

    void testMoveAwareInsert() {
        auto set = MoveHashSet{};

        auto inserted = MoveValue{1};
        const auto insertedCounts = inserted.counts();
        set.insert(std::move(inserted));
        REQUIRE_EQUAL(insertedCounts->copies, 0);
        REQUIRE(set.contains(MoveValue{1}));

        auto tryInserted = MoveValue{2};
        const auto tryInsertedCounts = tryInserted.counts();
        REQUIRE(set.tryInsert(std::move(tryInserted)));
        REQUIRE_EQUAL(tryInsertedCounts->copies, 0);
        REQUIRE(set.contains(MoveValue{2}));
    }

    void testRemoveAndAlgorithms() {
        auto set = IntHashSet{{1, 2, 3, 4}};
        set.removeIf([](int key) -> bool { return key % 2 == 0; });
        REQUIRE(set.compare(IntHashSet{{1, 3}}));

        const auto removed = set.removedIf([](int key) -> bool { return key == 3; });
        auto singletonOne = IntHashSet{};
        singletonOne.insert(1);
        REQUIRE(removed.compare(singletonOne));
        REQUIRE(set.compare(IntHashSet{{1, 3}}));

        const auto constSet = set;
        auto visited = std::vector<int>{};
        constSet.forEach([&](int key) -> LoopStatus {
            visited.push_back(key);
            return LoopStatus::Stop;
        });
        REQUIRE_EQUAL(visited.size(), std::size_t{1});
        REQUIRE_EQUAL(set.countIf([](int key) -> bool { return key > 1; }), ItemCount{1});
    }

    void testSetOperationsAndPredicates() {
        const auto first = IntHashSet{{1, 2, 3}};
        const auto second = IntHashSet{{3, 4}};
        auto singletonThree = IntHashSet{};
        singletonThree.insert(3);

        REQUIRE(first.unitedWith(second).compare(IntHashSet{{1, 2, 3, 4}}));
        REQUIRE(first.intersectedWith(second).compare(singletonThree));
        REQUIRE(first.subtractedBy(second).compare(IntHashSet{{1, 2}}));
        REQUIRE(first.symmetricDifferenceWith(second).compare(IntHashSet{{1, 2, 4}}));

        auto mutableSet = first;
        auto singletonFive = IntHashSet{};
        auto singletonSix = IntHashSet{};
        singletonFive.insert(5);
        singletonSix.insert(6);
        mutableSet.intersect(second)
            .unite(singletonFive)
            .subtract(singletonThree)
            .symmetricDifference(IntHashSet{{5, 6}});
        REQUIRE(mutableSet.compare(singletonSix));

        REQUIRE(IntHashSet{{1, 2}}.isSubsetOf(first));
        REQUIRE(first.isSupersetOf(IntHashSet{{1, 2}}));
        REQUIRE(first.intersects(second));
        REQUIRE(first.isDisjointWith(IntHashSet{{4, 5}}));
        REQUIRE(first.allOf([](int key) -> bool { return key > 0; }));
        REQUIRE(first.anyOf([](int key) -> bool { return key == 2; }));
        REQUIRE(first.noneOf([](int key) -> bool { return key < 0; }));
    }
};
