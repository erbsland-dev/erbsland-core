// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/CowManualStorage.hpp>
#include <erbsland/mem/CowStorage.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <set>
#include <utility>

class CowProbe final {
public:
    explicit CowProbe(int value = 0) : value{value} { ++constructed; }
    CowProbe(const CowProbe &other) : value{other.value} {
        ++constructed;
        ++copied;
    }
    CowProbe(CowProbe &&other) noexcept : value{std::exchange(other.value, 0)} { ++constructed; }
    ~CowProbe() { ++destroyed; }
    auto operator=(const CowProbe &other) -> CowProbe & {
        value = other.value;
        return *this;
    }

public:
    static void resetStats() {
        constructed = 0;
        copied = 0;
        destroyed = 0;
    }

public:
    int value;
    inline static std::atomic<int> constructed{0};
    inline static std::atomic<int> copied{0};
    inline static std::atomic<int> destroyed{0};
};

TESTED_TARGETS(CowStorage CowManualStorage)
class CowStorageTest final : public el::UnitTest {
    using ManualProbeStorage = el::mem::CowManualStorage<CowProbe>;
    using ProbeStorage = el::mem::CowStorage<CowProbe>;
    using SetStorage = el::mem::CowStorage<std::set<int>>;

public:
    void testDefaultStorageHasInstance() {
        const auto storage = ProbeStorage{};

        REQUIRE_EQUAL(storage.data().value, 0);
        REQUIRE_EQUAL(storage.useCount(), 1L);
        REQUIRE_FALSE(storage.isShared());
        REQUIRE_NOT_EQUAL(storage.storageId(), 0U);
    }

    void testFromAndCreate() {
        const auto fromValue = ProbeStorage::from(CowProbe{7});
        const auto created = ProbeStorage::create(11);

        REQUIRE_EQUAL(fromValue.data().value, 7);
        REQUIRE_EQUAL(created.data().value, 11);
    }

    void testCopySharesUntilMutableAccess() {
        CowProbe::resetStats();
        auto first = ProbeStorage::create(3);
        auto second = first;
        const auto originalStorageId = first.storageId();

        REQUIRE(first.isShared());
        REQUIRE(second.isShared());
        REQUIRE_EQUAL(first.useCount(), 2L);

        first.data().value = 9;

        REQUIRE_NOT_EQUAL(first.storageId(), originalStorageId);
        REQUIRE_EQUAL(first.data().value, 9);
        REQUIRE_EQUAL(second.data().value, 3);
        REQUIRE_FALSE(first.isShared());
        REQUIRE_FALSE(second.isShared());
        REQUIRE_EQUAL(CowProbe::copied.load(), 1);
    }

    void testExplicitDetach() {
        auto first = ProbeStorage::create(5);
        auto second = first;
        const auto originalStorageId = first.storageId();

        first.detach();

        REQUIRE_NOT_EQUAL(first.storageId(), originalStorageId);
        REQUIRE_EQUAL(first.data().value, 5);
        REQUIRE_EQUAL(second.data().value, 5);
        REQUIRE_FALSE(first.isShared());
        REQUIRE_FALSE(second.isShared());
    }

    void testSetLikeUsage() {
        auto storage = SetStorage::from(std::set<int>{1, 2});
        auto copy = storage;

        storage.detach();
        storage.data().insert(3);

        REQUIRE(storage.data().contains(1));
        REQUIRE(storage.data().contains(3));
        REQUIRE_FALSE(copy.data().contains(3));
    }

    void testMoveKeepsBothStorageObjectsValid() {
        auto source = ProbeStorage::create(17);
        const auto sourceStorageId = source.storageId();

        auto destination = std::move(source);

        REQUIRE_EQUAL(destination.storageId(), sourceStorageId);
        REQUIRE_EQUAL(source.storageId(), sourceStorageId);
        REQUIRE(destination.isShared());
        const auto &constDestination = destination;
        const auto &constSource = source;
        REQUIRE_EQUAL(constDestination.data().value, 17);
        REQUIRE_EQUAL(constSource.data().value, 17);
    }

    void testAssignmentKeepsMovedFromStorageUsable() {
        auto source = ProbeStorage::create(23);
        auto destination = ProbeStorage::create(1);

        destination = std::move(source);

        REQUIRE_EQUAL(destination.data().value, 23);
        REQUIRE_EQUAL(source.data().value, 23);
        destination.data().value = 42;
        REQUIRE_EQUAL(destination.data().value, 42);
        REQUIRE_EQUAL(source.data().value, 23);
    }

    void testManualStorageDetachedData() {
        auto first = ManualProbeStorage::create(6);
        auto second = first;
        const auto originalStorageId = first.storageId();

        first.detachedData().value = 12;

        REQUIRE_NOT_EQUAL(first.storageId(), originalStorageId);
        REQUIRE_EQUAL(first.data().value, 12);
        REQUIRE_EQUAL(second.data().value, 6);
        REQUIRE_FALSE(first.isShared());
        REQUIRE_FALSE(second.isShared());
    }

    void testManualStorageSetData() {
        auto storage = ManualProbeStorage::create(1);
        auto copy = storage;

        storage.setData(CowProbe{4});

        REQUIRE_EQUAL(storage.data().value, 4);
        REQUIRE_EQUAL(copy.data().value, 1);
        REQUIRE_FALSE(storage.isShared());
        REQUIRE_FALSE(copy.isShared());
    }
};
