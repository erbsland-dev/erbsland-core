// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CacheTestKey.hpp"

#include <erbsland/FifoCache.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(FifoCache OrderedCache)
class FifoCacheTest final : public el::UnitTest {
    using Cache = el::util::FifoCache<int, int>;

public:
    void testEvictionAndLookupOrder() {
        auto cache = Cache{el::ItemCount{2}};
        cache.set(1, 10).set(2, 20);
        REQUIRE_EQUAL(*cache.get(1), 10);
        REQUIRE(cache.contains(2));
        REQUIRE_EQUAL(*cache.peek(2), 20);
        cache.set(3, 30);
        REQUIRE_FALSE(cache.contains(1));
        REQUIRE(cache.contains(2));
        REQUIRE_EQUAL(cache.count().toSizeT(), 2U);
        REQUIRE_FALSE(cache.get(99).has_value());
    }
    void testReplacingExistingKey() {
        auto cache = Cache{el::ItemCount{2}};
        cache.set(1, 10).set(2, 20).set(1, 11);
        REQUIRE_EQUAL(*cache.peek(1), 11);
        cache.set(3, 30);
        REQUIRE_FALSE(cache.contains(1));
        REQUIRE(cache.contains(2));
    }
    void testResizeClearAndRemove() {
        auto cache = Cache{el::ItemCount{3}};
        cache.set(1, 10).set(2, 20).set(3, 30).setCapacity(el::ItemCount{1});
        REQUIRE_EQUAL(cache.count().toSizeT(), 1U);
        REQUIRE(cache.contains(3));
        cache.remove(3).remove(99);
        REQUIRE(cache.isEmpty());
        cache.set(4, 40).clear();
        REQUIRE(cache.isEmpty());
        REQUIRE_EQUAL(cache.capacity().toSizeT(), 1U);
        cache.setCapacity(el::ItemCount{0}).set(5, 50);
        REQUIRE(cache.isEmpty());
        cache.setCapacity(el::ItemCount{2}).set(6, 60);
        REQUIRE(cache.contains(6));
        REQUIRE_THROWS_AS(el::ParameterFailure, cache.setCapacity(el::ItemCount::infinite()));
        REQUIRE_EQUAL(cache.capacity().toSizeT(), 2U);
        REQUIRE_THROWS_AS(el::ParameterFailure, Cache(el::ItemCount::infinite()));
    }
    void testIndependentCopyAndMove() {
        auto original = Cache{el::ItemCount{2}};
        original.set(1, 10).set(2, 20);
        auto copy = original;
        copy.set(3, 30);
        REQUIRE(original.contains(1));
        REQUIRE_FALSE(copy.contains(1));
        auto assigned = Cache{el::ItemCount{0}};
        assigned = original;
        original.clear();
        REQUIRE(assigned.contains(1));
        auto moved = std::move(assigned);
        REQUIRE(moved.contains(1));
        assigned.set(9, 90);
        REQUIRE(assigned.contains(9));
        copy = std::move(moved);
        REQUIRE(copy.contains(1));
        moved.set(8, 80);
        REQUIRE(moved.contains(8));
        copy = copy;
        REQUIRE(copy.contains(1));
    }
    void testCollisionsAndFailedInsertionPreserveEntries() {
        auto cache = el::util::FifoCache<CacheTestKey, int, CacheTestKey::Hash>{el::ItemCount{2}};
        cache.set(CacheTestKey{1}, 10).set(CacheTestKey{2}, 20);
        REQUIRE_THROWS_AS(std::runtime_error, cache.set(CacheTestKey{3, true}, 30));
        REQUIRE_EQUAL(cache.count().toSizeT(), 2U);
        REQUIRE_EQUAL(*cache.peek(CacheTestKey{1}), 10);
        REQUIRE_EQUAL(*cache.peek(CacheTestKey{2}), 20);
        REQUIRE_FALSE(cache.contains(CacheTestKey{3}));
        cache.set(CacheTestKey{3}, 30);
        REQUIRE_FALSE(cache.contains(CacheTestKey{1}));
        REQUIRE(cache.contains(CacheTestKey{2}));
        REQUIRE(cache.contains(CacheTestKey{3}));
    }
};
