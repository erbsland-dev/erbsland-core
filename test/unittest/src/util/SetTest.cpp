// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/List.hpp>
#include <erbsland/util/LoopStatus.hpp>
#include <erbsland/util/Set.hpp>

#include <set>
#include <unordered_set>
#include <vector>

using el::unit::ElementCount;
using el::util::LoopStatus;

TESTED_TARGETS(Set)
class SetTest final : public el::UnitTest {
public:
    using IntList = el::util::List<int>;
    using IntSet = el::util::Set<int>;

    void testConstructionAndElements() {
        const auto empty = IntSet{};
        REQUIRE(empty.count().isZero());
        REQUIRE_EQUAL(empty.first(), 0);
        REQUIRE_EQUAL(empty.last(), 0);

        const auto set = IntSet{{3, 1, 2, 1}};
        REQUIRE_EQUAL(set.count(), ElementCount{3});
        REQUIRE_EQUAL(set.first(), 1);
        REQUIRE_EQUAL(set.last(), 3);
        REQUIRE_EQUAL(set.toStdVector(), (std::vector<int>{1, 2, 3}));
        REQUIRE(set.toStdSet() == (std::set<int>{1, 2, 3}));
        REQUIRE(set.toStdUnorderedSet() == (std::unordered_set<int>{1, 2, 3}));
        const auto fromList = IntSet::fromList(IntList{{2, 2, 1}});
        REQUIRE_EQUAL(fromList.toStdVector(), (std::vector<int>{1, 2}));
        REQUIRE_EQUAL(fromList.toList().toStdVector(), (std::vector<int>{1, 2}));
    }

    void testCopyOnWriteMemoryAndChange() {
        auto first = IntSet{{1, 2}};
        auto second = first;

        second.reserve(ElementCount{32}).insert(3).remove(1).shrinkToFit();

        REQUIRE_EQUAL(first.toStdVector(), (std::vector<int>{1, 2}));
        REQUIRE_EQUAL(second.toStdVector(), (std::vector<int>{2, 3}));
        REQUIRE_EQUAL(second.capacity(), second.count());
        REQUIRE(second.tryInsert(4));
        REQUIRE(!second.tryInsert(4));
        REQUIRE(second.tryRemove(4));
        REQUIRE(!second.tryRemove(4));
    }

    void testRemoveAndAlgorithms() {
        auto set = IntSet{{1, 2, 3, 4}};
        set.removeIf([](int key) -> bool { return key % 2 == 0; });
        REQUIRE_EQUAL(set.toStdVector(), (std::vector<int>{1, 3}));

        const auto removed = set.removedIf([](int key) -> bool { return key == 3; });
        REQUIRE_EQUAL(removed.toStdVector(), (std::vector<int>{1}));
        REQUIRE_EQUAL(set.toStdVector(), (std::vector<int>{1, 3}));

        const auto constSet = set;
        auto visited = std::vector<int>{};
        constSet.forEach([&](int key) -> LoopStatus {
            visited.push_back(key);
            return key == 1 ? LoopStatus::Stop : LoopStatus::Continue;
        });
        REQUIRE_EQUAL(visited, (std::vector<int>{1}));

        auto reverseVisited = std::vector<int>{};
        constSet.forEachReverse([&](int key) { reverseVisited.push_back(key); });
        REQUIRE_EQUAL(reverseVisited, (std::vector<int>{3, 1}));
        REQUIRE_EQUAL(set.countIf([](int key) -> bool { return key > 1; }), ElementCount{1});
    }

    void testSetOperationsAndPredicates() {
        const auto first = IntSet{{1, 2, 3}};
        const auto second = IntSet{{3, 4}};
        auto singletonThree = IntSet{};
        singletonThree.insert(3);

        REQUIRE(first.unitedWith(second).compare(IntSet{{1, 2, 3, 4}}));
        REQUIRE(first.intersectedWith(second).compare(singletonThree));
        REQUIRE(first.subtractedBy(second).compare(IntSet{{1, 2}}));
        REQUIRE(first.symmetricDifferenceWith(second).compare(IntSet{{1, 2, 4}}));

        auto mutableSet = first;
        auto singletonFive = IntSet{};
        auto singletonSix = IntSet{};
        singletonFive.insert(5);
        singletonSix.insert(6);
        mutableSet.intersect(second).unite(singletonFive).subtract(singletonThree).symmetricDifference(IntSet{{5, 6}});
        REQUIRE(mutableSet.compare(singletonSix));

        REQUIRE(IntSet{{1, 2}}.isSubsetOf(first));
        REQUIRE(first.isSupersetOf(IntSet{{1, 2}}));
        REQUIRE(first.intersects(second));
        REQUIRE(first.isDisjointWith(IntSet{{4, 5}}));
        REQUIRE(first.contains(2));
        REQUIRE(first.allOf([](int key) -> bool { return key > 0; }));
        REQUIRE(first.anyOf([](int key) -> bool { return key == 2; }));
        REQUIRE(first.noneOf([](int key) -> bool { return key < 0; }));
    }
};
