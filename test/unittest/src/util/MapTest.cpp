// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/LoopStatus.hpp>
#include <erbsland/util/Map.hpp>

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using el::unit::ElementCount;
using el::util::LoopStatus;

TESTED_TARGETS(Map)
class MapTest final : public el::UnitTest {
public:
    using IntMap = el::util::Map<int, int>;

    void testConstructionAndElements() {
        const auto empty = IntMap{};
        REQUIRE(empty.count().isZero());
        REQUIRE_EQUAL(empty.first(), (std::pair<int, int>{0, 0}));
        REQUIRE_EQUAL(empty.last(), (std::pair<int, int>{0, 0}));

        const auto map = IntMap{{{2, 20}, {1, 10}, {3, 30}}};
        REQUIRE_EQUAL(map.count(), ElementCount{3});
        REQUIRE_EQUAL(map.first(), (std::pair<int, int>{1, 10}));
        REQUIRE_EQUAL(map.last(), (std::pair<int, int>{3, 30}));
        REQUIRE(map.get(2).has_value());
        REQUIRE_EQUAL(map.get(2).value(), 20);
        REQUIRE(!map.get(9).has_value());
        REQUIRE_EQUAL(map.get(9, 90), 90);
        REQUIRE_EQUAL(map.toKeyList().toStdVector(), (std::vector<int>{1, 2, 3}));
        REQUIRE_EQUAL(map.toValueList().toStdVector(), (std::vector<int>{10, 20, 30}));
        REQUIRE_EQUAL(map.toList().toStdVector(), (std::vector<std::pair<int, int>>{{1, 10}, {2, 20}, {3, 30}}));
        REQUIRE(map.toRawValue() == (std::map<int, int>{{1, 10}, {2, 20}, {3, 30}}));
        REQUIRE(map.toStdMap() == (std::map<int, int>{{1, 10}, {2, 20}, {3, 30}}));
        REQUIRE(map.toStdUnorderedMap() == (std::unordered_map<int, int>{{1, 10}, {2, 20}, {3, 30}}));
        REQUIRE(map.toKeySet().toStdSet() == (std::set<int>{1, 2, 3}));
        REQUIRE(map.toKeyHashSet().toStdUnorderedSet() == (std::unordered_set<int>{1, 2, 3}));
        REQUIRE(map.toValueSet().toStdSet() == (std::set<int>{10, 20, 30}));
        REQUIRE(map.toValueHashSet().toStdUnorderedSet() == (std::unordered_set<int>{10, 20, 30}));
        REQUIRE_EQUAL(map.toStdKeyVector(), (std::vector<int>{1, 2, 3}));
        REQUIRE_EQUAL(map.toStdVector(), (std::vector<std::pair<int, int>>{{1, 10}, {2, 20}, {3, 30}}));
    }

    void testCopyOnWriteAndChange() {
        auto first = IntMap{{{1, 10}, {2, 20}}};
        auto second = first;

        second.set(2, 22).set(3, 30);

        REQUIRE_EQUAL(first.get(2, 0), 20);
        REQUIRE(!first.contains(3));
        REQUIRE_EQUAL(second.get(2, 0), 22);
        REQUIRE(second.contains(3));
        REQUIRE(second.tryReplace(3, 33));
        REQUIRE_EQUAL(second.get(3, 0), 33);
        REQUIRE(!second.tryReplace(4, 44));
        REQUIRE(second.tryInsert(4, 40));
        REQUIRE(!second.tryInsert(4, 41));
        REQUIRE_EQUAL(second.get(4, 0), 40);
    }

    void testMemoryRemoveAndTake() {
        auto map = IntMap{{{1, 10}, {2, 20}, {3, 30}, {4, 40}}};
        REQUIRE_EQUAL(map.capacity(), map.count());
        map.reserve(ElementCount{100}).shrinkToFit();
        REQUIRE_EQUAL(map.capacity(), map.count());

        REQUIRE_EQUAL(map.removed(2).toKeyList().toStdVector(), (std::vector<int>{1, 3, 4}));
        map.removeIfKey([](int key) -> bool { return key % 2 == 0; });
        REQUIRE_EQUAL(map.toKeyList().toStdVector(), (std::vector<int>{1, 3}));
        map.removeIfValue([](int value) -> bool { return value == 30; });
        REQUIRE_EQUAL(map.toKeyList().toStdVector(), (std::vector<int>{1}));

        map = IntMap{{{1, 10}, {2, 20}, {3, 30}}};
        REQUIRE_EQUAL(map.take(2), 20);
        REQUIRE(!map.contains(2));
        REQUIRE_EQUAL(map.take(9), 0);
        const auto taken = map.takeIf([](int key, int) -> bool { return key > 1; });
        REQUIRE_EQUAL(taken.toList().toStdVector(), (std::vector<std::pair<int, int>>{{3, 30}}));
        REQUIRE_EQUAL(map.toList().toStdVector(), (std::vector<std::pair<int, int>>{{1, 10}}));
    }

    void testAlgorithmsAndPredicates() {
        auto map = IntMap{{{1, 10}, {2, 20}, {3, 30}}};
        const auto constMap = map;
        auto keys = std::vector<int>{};
        constMap.forEachKey([&](int key) -> LoopStatus {
            keys.push_back(key);
            return key == 2 ? LoopStatus::Stop : LoopStatus::Continue;
        });
        REQUIRE_EQUAL(keys, (std::vector<int>{1, 2}));

        auto entries = std::vector<std::pair<int, int>>{};
        constMap.forEach([&](int key, int value) { entries.emplace_back(key, value); });
        REQUIRE_EQUAL(entries, (std::vector<std::pair<int, int>>{{1, 10}, {2, 20}, {3, 30}}));

        auto values = std::vector<int>{};
        constMap.forEachValue([&](int value) { values.push_back(value); });
        REQUIRE_EQUAL(values, (std::vector<int>{10, 20, 30}));

        auto reverseKeys = std::vector<int>{};
        constMap.forEachKeyReverse([&](int key) { reverseKeys.push_back(key); });
        REQUIRE_EQUAL(reverseKeys, (std::vector<int>{3, 2, 1}));

        auto reverseEntries = std::vector<std::pair<int, int>>{};
        constMap.forEachReverse([&](int key, int value) { reverseEntries.emplace_back(key, value); });
        REQUIRE_EQUAL(reverseEntries, (std::vector<std::pair<int, int>>{{3, 30}, {2, 20}, {1, 10}}));

        auto reverseValues = std::vector<int>{};
        constMap.forEachValueReverse([&](int value) { reverseValues.push_back(value); });
        REQUIRE_EQUAL(reverseValues, (std::vector<int>{30, 20, 10}));

        map.mapValue([](int key, int value) -> int { return key + value; });
        REQUIRE_EQUAL(map.toValueList().toStdVector(), (std::vector<int>{11, 22, 33}));
        REQUIRE_EQUAL(
            map.mappedValues([](int, int value) -> int { return value * 2; }).toValueList().toStdVector(),
            (std::vector<int>{22, 44, 66}));
        REQUIRE(map.allOf([](int key, int value) -> bool { return value > key; }));
        REQUIRE(map.anyOf([](int, int value) -> bool { return value == 22; }));
        REQUIRE(map.noneOf([](int, int value) -> bool { return value < 0; }));
        REQUIRE_EQUAL(map.countIfValue([](int value) -> bool { return value > 20; }), ElementCount{2});
        REQUIRE(map.compareKeys(IntMap{{{1, 1}, {2, 2}, {3, 3}}}));
        REQUIRE(map.compare(IntMap{{{1, 11}, {2, 22}, {3, 33}}}));
    }
};
