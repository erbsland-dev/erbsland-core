// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/impl/SecureErase.hpp>
#include <erbsland/mem/ReferenceCounter.hpp>
#include <erbsland/mem/SharedArrayData.hpp>
#include <erbsland/mem/SharedData.hpp>
#include <erbsland/mem/SharedDataPointer.hpp>
#include <erbsland/mem/SharedVirtualData.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace el::mem;

class ObjectProbe final : public SharedData {
public:
    explicit ObjectProbe(int value = 0) : value{value} { ++constructed; }
    ObjectProbe(const ObjectProbe &other) : SharedData{other}, value{other.value} {
        ++constructed;
        ++copied;
    }
    ~ObjectProbe() { ++destroyed; }
    auto operator=(const ObjectProbe &other) -> ObjectProbe & {
        SharedData::operator=(other);
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

class VirtualProbeBase : public SharedVirtualData {
public:
    [[nodiscard]] virtual auto value() const noexcept -> int = 0;
    virtual void setValue(int value) noexcept = 0;
};

class VirtualProbe final : public VirtualProbeBase {
public:
    explicit VirtualProbe(int value = 0) : _value{value} { ++constructed; }
    VirtualProbe(const VirtualProbe &other) : VirtualProbeBase{other}, _value{other._value} {
        ++constructed;
        ++copied;
    }
    ~VirtualProbe() override { ++destroyed; }

public:
    [[nodiscard]] auto clone() const -> VirtualProbe * override { return new VirtualProbe{*this}; }
    [[nodiscard]] auto value() const noexcept -> int override { return _value; }
    void setValue(const int value) noexcept override { _value = value; }

public:
    static void resetStats() {
        constructed = 0;
        copied = 0;
        destroyed = 0;
    }

public:
    inline static std::atomic<int> constructed{0};
    inline static std::atomic<int> copied{0};
    inline static std::atomic<int> destroyed{0};

private:
    int _value;
};

struct alignas(32) ElementProbe {
    ElementProbe() : value{0} { ++constructed; }
    ElementProbe(const ElementProbe &other) : value{other.value} {
        if (copiesBeforeThrow.fetch_sub(1) == 0) {
            throw std::runtime_error{"ElementProbe copy failed."};
        }
        ++constructed;
        ++copied;
    }
    ~ElementProbe() { ++destroyed; }
    auto operator=(const ElementProbe &other) -> ElementProbe & {
        value = other.value;
        return *this;
    }

    static void resetStats() {
        constructed = 0;
        copied = 0;
        destroyed = 0;
        copiesBeforeThrow = std::numeric_limits<int>::max();
    }

    int value;
    inline static std::atomic<int> constructed{0};
    inline static std::atomic<int> copied{0};
    inline static std::atomic<int> destroyed{0};
    inline static std::atomic<int> copiesBeforeThrow{std::numeric_limits<int>::max()};
};

struct SharedArrayEraseEvent final {
    std::size_t size{};
    bool isZero{};
};

std::vector<SharedArrayEraseEvent> gSharedArrayEraseEvents;

void observeSharedArrayErase(const std::span<const std::byte> bytes) noexcept {
    gSharedArrayEraseEvents.push_back(
        {bytes.size(),
            std::ranges::all_of(bytes, [](const std::byte value) noexcept -> bool { return value == std::byte{}; })});
}

class SharedArrayEraseObserverGuard final {
public:
    SharedArrayEraseObserverGuard() {
        gSharedArrayEraseEvents.clear();
        el::mem::impl::setSecureEraseObserver(observeSharedArrayErase);
    }
    ~SharedArrayEraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }
};

TESTED_TARGETS(SharedDataPointer SharedVirtualData)
class SharedDataTest final : public el::UnitTest {
    using ObjectPointer = SharedDataPointer<ObjectProbe>;
    using ManualObjectPointer = SharedDataPointer<ObjectProbe, true>;
    using VirtualPointer = SharedDataPointer<VirtualProbeBase>;
    using ByteData = SharedArrayData<std::byte, uint32_t>;

    using BytePointer = SharedDataPointer<ByteData>;
    using LargeByteData = SharedArrayData<std::byte, uint64_t>;

    using LargeBytePointer = SharedDataPointer<LargeByteData>;
    using ElementData = SharedArrayData<ElementProbe, uint32_t, SharedArrayDataConstructMethod::ValueConstruct>;
    using WideData = SharedArrayData<std::uint64_t, uint64_t>;
    using SecureByteData = SharedArrayData<
        std::byte,
        uint32_t,
        SharedArrayDataConstructMethod::None,
        SharedArrayDataCleanupMethod::SecureErase>;

    using ElementPointer = SharedDataPointer<ElementData>;
    using SecureBytePointer = SharedDataPointer<SecureByteData>;

public:
    void testReferenceCounter() {
        auto counter = ReferenceCounter{};
        REQUIRE_FALSE(counter.isReferenced());
        REQUIRE_FALSE(counter.isShared());
        REQUIRE_EQUAL(counter.useCount(), 0U);

        REQUIRE_EQUAL(counter.addReference(), ReferenceCounter::HasReferences);
        REQUIRE(counter.isReferenced());
        REQUIRE_FALSE(counter.isShared());
        REQUIRE_EQUAL(counter.useCount(), 1U);

        counter.addReference();
        REQUIRE(counter.isShared());
        REQUIRE_EQUAL(counter.useCount(), 2U);

        REQUIRE_EQUAL(counter.removeReference(), ReferenceCounter::HasReferences);
        REQUIRE_EQUAL(counter.useCount(), 1U);
        REQUIRE_EQUAL(counter.removeReference(), ReferenceCounter::NoReferences);
        REQUIRE_FALSE(counter.isReferenced());
    }

    void testObjectReferenceLifecycle() {
        ObjectProbe::resetStats();
        {
            auto pointer = ObjectPointer{new ObjectProbe{7}};
            REQUIRE_EQUAL(pointer.useCount(), 1U);
            {
                auto copy = pointer;
                REQUIRE_EQUAL(pointer.useCount(), 2U);
                REQUIRE(copy.isShared());
            }
            REQUIRE_EQUAL(pointer.useCount(), 1U);
        }
        REQUIRE_EQUAL(ObjectProbe::constructed.load(), 1);
        REQUIRE_EQUAL(ObjectProbe::destroyed.load(), 1);
    }

    void testObjectDetach() {
        ObjectProbe::resetStats();
        auto first = ObjectPointer{new ObjectProbe{11}};
        auto second = first;
        const auto *oldData = second.constGet();

        first->value = 23;

        REQUIRE_NOT_EQUAL(first.constGet(), oldData);
        REQUIRE_EQUAL(first.constGet()->value, 23);
        REQUIRE_EQUAL(second.constGet()->value, 11);
        REQUIRE_EQUAL(first.useCount(), 1U);
        REQUIRE_EQUAL(second.useCount(), 1U);
        REQUIRE_EQUAL(ObjectProbe::copied.load(), 1);
    }

    void testManualDetach() {
        ObjectProbe::resetStats();
        auto first = ManualObjectPointer{new ObjectProbe{5}};
        auto second = first;

        first.get()->value = 6;
        REQUIRE_EQUAL(second.constGet()->value, 6);
        REQUIRE_EQUAL(first.useCount(), 2U);

        first.detach();
        first.get()->value = 9;
        REQUIRE_EQUAL(first.constGet()->value, 9);
        REQUIRE_EQUAL(second.constGet()->value, 6);
        REQUIRE_EQUAL(first.useCount(), 1U);
        REQUIRE_EQUAL(second.useCount(), 1U);
    }

    void testVirtualDataDetachUsesPolymorphicClone() {
        VirtualProbe::resetStats();
        {
            auto first = VirtualPointer{new VirtualProbe{17}};
            auto second = first;
            const auto *oldSecondData = second.constGet();

            first->setValue(29);

            REQUIRE_NOT_EQUAL(first.constGet(), oldSecondData);
            REQUIRE_EQUAL(first.constGet()->value(), 29);
            REQUIRE_EQUAL(second.constGet()->value(), 17);
            REQUIRE_EQUAL(first.useCount(), 1U);
            REQUIRE_EQUAL(second.useCount(), 1U);
            REQUIRE_EQUAL(VirtualProbe::copied.load(), 1);
        }
        REQUIRE_EQUAL(VirtualProbe::constructed.load(), 2);
        REQUIRE_EQUAL(VirtualProbe::destroyed.load(), 2);
    }

    void testByteArrayDetach() {
        auto *rawData = ByteData::create(3, 8);
        rawData->data()[0] = std::byte{1};
        rawData->data()[1] = std::byte{2};
        rawData->data()[2] = std::byte{3};
        auto first = BytePointer{rawData};
        auto second = first;

        first->data()[0] = std::byte{9};
        first->setSize(2);

        REQUIRE_EQUAL(first.constGet()->size(), 2U);
        REQUIRE_EQUAL(second.constGet()->size(), 3U);
        REQUIRE_EQUAL(std::to_integer<int>(first.constGet()->data()[0]), 9);
        REQUIRE_EQUAL(std::to_integer<int>(second.constGet()->data()[0]), 1);
        REQUIRE_EQUAL(std::to_integer<int>(second.constGet()->data()[2]), 3);
    }

    void testUInt64ArraySize() {
        auto *rawData = LargeByteData::create(2, 5);
        rawData->data()[0] = std::byte{4};
        rawData->data()[1] = std::byte{5};
        auto first = LargeBytePointer{rawData};
        auto second = first;

        first->setSize(1);
        first->data()[0] = std::byte{6};

        REQUIRE_EQUAL(first.constGet()->size(), 1ULL);
        REQUIRE_EQUAL(first.constGet()->capacity(), 5ULL);
        REQUIRE_EQUAL(second.constGet()->size(), 2ULL);
        REQUIRE_EQUAL(second.constGet()->capacity(), 5ULL);
        REQUIRE_EQUAL(std::to_integer<int>(first.constGet()->data()[0]), 6);
        REQUIRE_EQUAL(std::to_integer<int>(second.constGet()->data()[0]), 4);
        REQUIRE_EQUAL(std::to_integer<int>(second.constGet()->data()[1]), 5);
    }

    void testArrayAllocationCapacityCheck() {
        REQUIRE(ByteData::canAllocateWithCapacity(0));
        REQUIRE(ByteData::canAllocateWithCapacity(16U));
        REQUIRE_FALSE(ByteData::canAllocateWithCapacity(-1));
        REQUIRE_FALSE(ByteData::canAllocateWithCapacity(std::uint64_t{std::numeric_limits<std::uint32_t>::max()} + 1U));

        constexpr auto byteOverhead = sizeof(LargeByteData) + alignof(std::byte) - 1U;
        constexpr auto maxByteCapacity = std::numeric_limits<std::size_t>::max() - byteOverhead;
        REQUIRE(LargeByteData::canAllocateWithCapacity(maxByteCapacity));
        REQUIRE_FALSE(LargeByteData::canAllocateWithCapacity(maxByteCapacity + 1U));

        constexpr auto wideOverhead = sizeof(WideData) + alignof(std::uint64_t) - 1U;
        constexpr auto maxWideCapacity =
            (std::numeric_limits<std::size_t>::max() - wideOverhead) / sizeof(std::uint64_t);
        REQUIRE(WideData::canAllocateWithCapacity(maxWideCapacity));
        REQUIRE_FALSE(WideData::canAllocateWithCapacity(maxWideCapacity + 1U));
    }

    void testConstructedArrayDetachAndAlignment() {
        ElementProbe::resetStats();
        {
            auto *rawData = ElementData::create(2, 4);
            rawData->data()[0].value = 11;
            rawData->data()[1].value = 12;
            auto first = ElementPointer{rawData};
            auto second = first;

            const auto address = reinterpret_cast<std::uintptr_t>(first.constGet()->data());
            REQUIRE_EQUAL(address % alignof(ElementProbe), 0U);

            first->data()[0].value = 21;
            first->setSize(1);

            REQUIRE_EQUAL(first.constGet()->size(), 1U);
            REQUIRE_EQUAL(second.constGet()->size(), 2U);
            REQUIRE_EQUAL(first.constGet()->data()[0].value, 21);
            REQUIRE_EQUAL(second.constGet()->data()[0].value, 11);
            REQUIRE_EQUAL(second.constGet()->data()[1].value, 12);
            REQUIRE_EQUAL(ElementProbe::copied.load(), 2);
        }
        REQUIRE_EQUAL(ElementProbe::constructed.load(), 8);
        REQUIRE_EQUAL(ElementProbe::destroyed.load(), 8);
    }

    void testConstructedArrayCloneCleanupAfterThrow() {
        ElementProbe::resetStats();
        auto *rawData = ElementData::create(3, 3);
        rawData->data()[0].value = 1;
        rawData->data()[1].value = 2;
        rawData->data()[2].value = 3;
        ElementProbe::copiesBeforeThrow = 1;

        REQUIRE_THROWS(rawData->clone());

        ElementData::destroy(rawData);
        REQUIRE_EQUAL(ElementProbe::constructed.load(), ElementProbe::destroyed.load());
    }

    void testSecureArrayErasesCompleteAllocationOnDestructionAndDetach() {
        const auto guard = SharedArrayEraseObserverGuard{};
        constexpr auto capacity = uint32_t{17U};
        constexpr auto expectedSize = SecureByteData::allocationSizeForCapacity(capacity);
        {
            auto first = SecureBytePointer{SecureByteData::create(4U, capacity)};
            first->data()[0] = std::byte{0x31};
            first->data()[16] = std::byte{0x7f};
            auto second = first;

            first->data()[0] = std::byte{0x42};
            REQUIRE_NOT_EQUAL(first.constGet(), second.constGet());
            REQUIRE(gSharedArrayEraseEvents.empty());
        }

        REQUIRE_EQUAL(gSharedArrayEraseEvents.size(), std::size_t{2U});
        for (const auto &event : gSharedArrayEraseEvents) {
            REQUIRE_EQUAL(event.size, expectedSize);
            REQUIRE(event.isZero);
        }
    }

    void testConcurrentCopies() {
        ObjectProbe::resetStats();
        auto shared = ObjectPointer{new ObjectProbe{42}};
        auto failures = std::atomic<int>{0};
        auto threads = std::vector<std::thread>{};

        for (auto threadIndex = 0; threadIndex < 8; ++threadIndex) {
            threads.emplace_back([shared, &failures]() -> void {
                for (auto i = 0; i < 2000; ++i) {
                    auto local = shared;
                    if (local.constGet()->value != 42) {
                        ++failures;
                    }
                }
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }

        REQUIRE_EQUAL(failures.load(), 0);
        REQUIRE_EQUAL(shared.useCount(), 1U);
        REQUIRE_EQUAL(shared.constGet()->value, 42);
    }

    void testConcurrentDetach() {
        ObjectProbe::resetStats();
        auto shared = ObjectPointer{new ObjectProbe{100}};
        auto failures = std::atomic<int>{0};
        auto threads = std::vector<std::thread>{};

        for (auto threadIndex = 0; threadIndex < 8; ++threadIndex) {
            threads.emplace_back([shared, threadIndex, &failures]() mutable -> void {
                auto local = shared;
                local->value = threadIndex;
                if (local.constGet()->value != threadIndex) {
                    ++failures;
                }
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }

        REQUIRE_EQUAL(failures.load(), 0);
        REQUIRE_EQUAL(shared.constGet()->value, 100);
        REQUIRE_EQUAL(shared.useCount(), 1U);
    }
};
