// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MoveAwareTestValue.hpp"
#include "MoveAwareTestValueHash.hpp"

#include <erbsland/unit/ItemCount.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/HashMap.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using el::unit::ItemCount;

class HashMapTestHash final {
public:
    HashMapTestHash() : _identifier{++_nextIdentifier} {}

public:
    [[nodiscard]] auto identifier() const noexcept -> int { return _identifier; }

public: // operators
    [[nodiscard]] auto operator()(const int value) const noexcept -> std::size_t {
        return static_cast<std::size_t>(value);
    }

private:
    int _identifier;
    inline static int _nextIdentifier{0};
};

TESTED_TARGETS(HashMap)
class HashMapTest final : public el::UnitTest {
public:
    using IntHashMap = el::util::HashMap<int, int>;
    using MoveHashMap = el::util::HashMap<
        erbsland::test::MoveAwareTestValue,
        erbsland::test::MoveAwareTestValue,
        erbsland::test::MoveAwareTestValueHash>;
    using MoveValue = erbsland::test::MoveAwareTestValue;
    using StatefulHashMap = el::util::HashMap<int, int, HashMapTestHash>;

    void testSharedDefaultStorage() {
        auto first = IntHashMap{};
        const auto second = IntHashMap{};

        REQUIRE_EQUAL(&first.toRawValue(), &second.toRawValue());

        first.set(1, 10);

        REQUIRE_NOT_EQUAL(&first.toRawValue(), &second.toRawValue());
        REQUIRE_EQUAL(first.get(1, 0), 10);
        REQUIRE(second.count().isZero());
    }

    void testStatefulPolicyUsesUniqueDefaultStorage() {
        auto first = StatefulHashMap{};
        const auto second = StatefulHashMap{};

        REQUIRE_NOT_EQUAL(&first.toRawValue(), &second.toRawValue());
        REQUIRE_NOT_EQUAL(
            first.toRawValue().hash_function().identifier(), second.toRawValue().hash_function().identifier());

        first.set(1, 10);
        REQUIRE_EQUAL(first.get(1, 0), 10);
        REQUIRE(second.count().isZero());
    }

    void testConstructionAndElements() {
        const auto map = IntHashMap{{{1, 10}, {2, 20}, {3, 30}}};

        REQUIRE_EQUAL(map.count(), ItemCount{3});
        REQUIRE(map.get(2).has_value());
        REQUIRE_EQUAL(map.get(2).value(), 20);
        REQUIRE_EQUAL(map.get(9, 90), 90);
        REQUIRE(map.contains(1));
        REQUIRE(map.compareKeys(IntHashMap{{{3, 0}, {2, 0}, {1, 0}}}));
        REQUIRE(map.compare(IntHashMap{{{3, 30}, {2, 20}, {1, 10}}}));
        REQUIRE_NOT_EQUAL(map.first().second, 0);
        REQUIRE_EQUAL(map.toRawValue(), (std::unordered_map<int, int>{{1, 10}, {2, 20}, {3, 30}}));
        REQUIRE_EQUAL(map.toStdMap(), (std::map<int, int>{{1, 10}, {2, 20}, {3, 30}}));
        REQUIRE_EQUAL(map.toStdUnorderedMap(), (std::unordered_map<int, int>{{1, 10}, {2, 20}, {3, 30}}));
        REQUIRE_EQUAL(map.toKeySet().toStdSet(), (std::set<int>{1, 2, 3}));
        REQUIRE_EQUAL(map.toKeyHashSet().toStdUnorderedSet(), (std::unordered_set<int>{1, 2, 3}));
        REQUIRE_EQUAL(map.toValueSet().toStdSet(), (std::set<int>{10, 20, 30}));
        REQUIRE_EQUAL(map.toValueHashSet().toStdUnorderedSet(), (std::unordered_set<int>{10, 20, 30}));

        auto keys = map.toStdKeyVector();
        std::sort(keys.begin(), keys.end());
        REQUIRE_EQUAL(keys, (std::vector<int>{1, 2, 3}));

        auto entries = map.toStdVector();
        std::sort(entries.begin(), entries.end());
        REQUIRE_EQUAL(entries, (std::vector<std::pair<int, int>>{{1, 10}, {2, 20}, {3, 30}}));
    }

    void testCopyOnWriteMemoryAndChange() {
        auto first = IntHashMap{{{1, 10}, {2, 20}}};
        auto second = first;

        second.reserve(ItemCount{32}).set(2, 22).set(3, 30);

        REQUIRE(second.capacity().toSizeT() >= 32U);
        REQUIRE_EQUAL(first.get(2, 0), 20);
        REQUIRE(!first.contains(3));
        REQUIRE_EQUAL(second.get(2, 0), 22);
        REQUIRE(second.tryReplace(3, 33));
        REQUIRE(!second.tryReplace(4, 44));
        REQUIRE(second.tryInsert(4, 40));
        REQUIRE(!second.tryInsert(4, 41));
        second.shrinkToFit();
        REQUIRE(second.capacity().toSizeT() >= second.count().toSizeT());
    }

    void testMoveAwareChange() {
        auto map = MoveHashMap{};

        auto key = MoveValue{1};
        const auto keyCounts = key.counts();
        auto value = MoveValue{10};
        const auto valueCounts = value.counts();
        map.set(std::move(key), std::move(value));
        REQUIRE_EQUAL(keyCounts->copies, 0);
        REQUIRE_EQUAL(valueCounts->copies, 0);
        REQUIRE(map.contains(MoveValue{1}));

        const auto lvalueKey = MoveValue{2};
        auto rvalueValue = MoveValue{20};
        const auto rvalueValueCounts = rvalueValue.counts();
        map.set(lvalueKey, std::move(rvalueValue));
        REQUIRE_EQUAL(rvalueValueCounts->copies, 0);
        REQUIRE(map.contains(MoveValue{2}));

        auto replacementKey = MoveValue{1};
        const auto replacementKeyCounts = replacementKey.counts();
        auto replacementValue = MoveValue{11};
        const auto replacementValueCounts = replacementValue.counts();
        REQUIRE(map.tryReplace(std::move(replacementKey), std::move(replacementValue)));
        REQUIRE_EQUAL(replacementKeyCounts->copies, 0);
        REQUIRE_EQUAL(replacementValueCounts->copies, 0);

        auto insertedKey = MoveValue{3};
        const auto insertedKeyCounts = insertedKey.counts();
        auto insertedValue = MoveValue{30};
        const auto insertedValueCounts = insertedValue.counts();
        REQUIRE(map.tryInsert(std::move(insertedKey), std::move(insertedValue)));
        REQUIRE_EQUAL(insertedKeyCounts->copies, 0);
        REQUIRE_EQUAL(insertedValueCounts->copies, 0);
        REQUIRE(map.contains(MoveValue{3}));
    }

    void testRemoveTakeAndAlgorithms() {
        auto map = IntHashMap{{{1, 10}, {2, 20}, {3, 30}, {4, 40}}};

        map.removeIf([](int key, int value) -> bool { return key == 1 || value == 40; });
        REQUIRE(!map.contains(1));
        REQUIRE(!map.contains(4));
        REQUIRE(map.contains(2));
        REQUIRE(map.contains(3));

        const auto removed = map.removedIfValue([](int value) -> bool { return value == 20; });
        REQUIRE(!removed.contains(2));
        REQUIRE(map.contains(2));

        REQUIRE_EQUAL(map.take(2), 20);
        REQUIRE_EQUAL(map.take(9), 0);
        const auto taken = map.takeIfKey([](int key) -> bool { return key == 3; });
        REQUIRE(taken.contains(3));
        REQUIRE(map.count().isZero());

        map = IntHashMap{{{1, 10}, {2, 20}, {3, 30}}};
        const auto constMap = map;
        auto visitedEntries = std::vector<std::pair<int, int>>{};
        constMap.forEach([&](int key, int value) { visitedEntries.emplace_back(key, value); });
        std::sort(visitedEntries.begin(), visitedEntries.end());
        REQUIRE_EQUAL(visitedEntries, (std::vector<std::pair<int, int>>{{1, 10}, {2, 20}, {3, 30}}));

        auto visitedKeys = std::vector<int>{};
        constMap.forEachKey([&](int key) { visitedKeys.push_back(key); });
        std::sort(visitedKeys.begin(), visitedKeys.end());
        REQUIRE_EQUAL(visitedKeys, (std::vector<int>{1, 2, 3}));

        auto visitedValues = std::vector<int>{};
        constMap.forEachValue([&](int value) { visitedValues.push_back(value); });
        std::sort(visitedValues.begin(), visitedValues.end());
        REQUIRE_EQUAL(visitedValues, (std::vector<int>{10, 20, 30}));

        map.mapValue([](int key, int value) -> int { return key + value; });
        REQUIRE_EQUAL(map.get(1, 0), 11);
        REQUIRE_EQUAL(map.get(2, 0), 22);
        REQUIRE(map.allOf([](int key, int value) -> bool { return value > key; }));
        REQUIRE(map.anyOf([](int, int value) -> bool { return value == 33; }));
        REQUIRE(map.noneOf([](int, int value) -> bool { return value < 0; }));
    }
};
