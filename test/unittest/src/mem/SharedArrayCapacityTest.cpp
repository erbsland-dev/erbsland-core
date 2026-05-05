// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/impl/BestGrowth.hpp>
#include <erbsland/mem/impl/SharedArrayCapacity.hpp>
#include <erbsland/mem/SharedArrayData.hpp>
#include <erbsland/mem/SharedDataPointer.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

using el::mem::Byte;
using el::unit::ByteLength;

TESTED_TARGETS(BestGrowth SharedArrayCapacity SharedArrayData)
class SharedArrayCapacityTest final : public el::UnitTest {
    using ByteData = el::mem::SharedArrayData<Byte, uint64_t>;
    using ByteDataPtr = el::mem::SharedDataPointer<ByteData>;

public:
    void testBestGrowthNoGrowth() {
        REQUIRE_EQUAL(el::mem::impl::bestGrowth(ByteLength{64U}, ByteLength{32U}), ByteLength{64U});
        REQUIRE_EQUAL(el::mem::impl::bestGrowthCapacity<ByteData>(64U, 32U), 64U);
    }

    void testBestGrowthSmallAllocationIncludesOverhead() {
        const auto capacity = el::mem::impl::bestGrowthCapacity<ByteData>(0U, 1U);
        const auto requestedAllocationSize = ByteData::allocationSizeForCapacity(1U);
        const auto grownAllocationSize = ByteData::allocationSizeForCapacity(capacity);

        REQUIRE(capacity >= 1U);
        REQUIRE(grownAllocationSize > requestedAllocationSize);
        REQUIRE(isPowerOfTwo(grownAllocationSize));
    }

    void testBestGrowthExactBoundaryStepsToNextPowerOfTwo() {
        constexpr auto cBoundarySize = std::size_t{128U};
        static_assert(ByteData::allocationOverhead() < cBoundarySize);
        const auto requestedCapacity = cBoundarySize - ByteData::allocationOverhead();

        const auto capacity = el::mem::impl::bestGrowthCapacity<ByteData>(0U, requestedCapacity);

        REQUIRE_EQUAL(ByteData::allocationSizeForCapacity(requestedCapacity), cBoundarySize);
        REQUIRE_EQUAL(ByteData::allocationSizeForCapacity(capacity), cBoundarySize * 2U);
    }

    void testBestGrowthLargeAllocationRoundsToBlock() {
        const auto blockSize = el::mem::impl::cMaximumGrowthBlock.toSizeT();
        const auto requestedAllocationSize = blockSize + 1U;
        const auto requestedCapacity = requestedAllocationSize - ByteData::allocationOverhead();

        const auto capacity = el::mem::impl::bestGrowthCapacity<ByteData>(0U, requestedCapacity);

        REQUIRE_EQUAL(ByteData::allocationSizeForCapacity(capacity), blockSize * 2U);
    }

    void testBestGrowthNearMaximumStaysAllocatable() {
        const auto maxCapacity =
            (std::numeric_limits<std::size_t>::max() - ByteData::allocationOverhead()) / sizeof(ByteData::DataType);
        const auto requestedCapacity = maxCapacity - 1U;

        const auto capacity = el::mem::impl::bestGrowthCapacity<ByteData>(0U, requestedCapacity);

        REQUIRE(capacity >= requestedCapacity);
        REQUIRE(ByteData::canAllocateWithCapacity(capacity));
    }

    void testSharedCapacityNoOpForUniqueSufficientCapacity() {
        auto data = makeData({1U, 2U}, 16U);
        const auto storageId = data.storageId();
        auto callbackCalled = false;

        const auto didAllocate = el::mem::impl::ensureSharedArrayCapacity(
            data, 2U, 8U, false, [&](const auto *, auto *) { callbackCalled = true; });

        REQUIRE_FALSE(didAllocate);
        REQUIRE_FALSE(callbackCalled);
        REQUIRE_EQUAL(data.storageId(), storageId);
        REQUIRE_EQUAL(data.constGet()->capacity(), 16U);
    }

    void testSharedCapacityReallocatesWhenShared() {
        auto data = makeData({1U, 2U}, 16U);
        const auto sharedCopy = data;
        const auto oldStorageId = data.storageId();

        const auto didAllocate =
            el::mem::impl::ensureSharedArrayCapacity(data, 2U, 8U, false, [](const auto *oldData, auto *newData) {
                std::memcpy(newData->data(), oldData->data(), 2U * sizeof(Byte));
            });

        REQUIRE(didAllocate);
        REQUIRE_NOT_EQUAL(data.storageId(), oldStorageId);
        REQUIRE_EQUAL(data.constGet()->capacity(), 16U);
        REQUIRE_EQUAL(data.constGet()->data()[0], Byte{1U});
        REQUIRE_EQUAL(data.constGet()->data()[1], Byte{2U});
        REQUIRE_EQUAL(sharedCopy.constGet()->data()[0], Byte{1U});
    }

    void testSharedCapacityReallocatesWhenCapacityIsInsufficient() {
        auto data = makeData({1U, 2U}, 2U);
        const auto oldStorageId = data.storageId();
        const auto expectedCapacity = el::mem::impl::bestGrowthCapacity<ByteData>(2U, 24U);

        const auto didAllocate =
            el::mem::impl::ensureSharedArrayCapacity(data, 2U, 24U, false, [](const auto *oldData, auto *newData) {
                std::memcpy(newData->data(), oldData->data(), 2U * sizeof(Byte));
            });

        REQUIRE(didAllocate);
        REQUIRE_NOT_EQUAL(data.storageId(), oldStorageId);
        REQUIRE_EQUAL(data.constGet()->capacity(), expectedCapacity);
        REQUIRE_EQUAL(data.constGet()->data()[0], Byte{1U});
        REQUIRE_EQUAL(data.constGet()->data()[1], Byte{2U});
    }

    void testSharedCapacityForceReallocate() {
        auto data = makeData({1U, 2U}, 16U);
        const auto oldStorageId = data.storageId();

        const auto didAllocate =
            el::mem::impl::ensureSharedArrayCapacity(data, 2U, 8U, true, [](const auto *oldData, auto *newData) {
                std::memcpy(newData->data(), oldData->data(), 2U * sizeof(Byte));
            });

        REQUIRE(didAllocate);
        REQUIRE_NOT_EQUAL(data.storageId(), oldStorageId);
        REQUIRE_EQUAL(data.constGet()->data()[0], Byte{1U});
        REQUIRE_EQUAL(data.constGet()->data()[1], Byte{2U});
    }

private:
    [[nodiscard]] static auto isPowerOfTwo(const std::size_t value) noexcept -> bool {
        return value != 0U && (value & (value - 1U)) == 0U;
    }

    [[nodiscard]] static auto makeData(std::initializer_list<uint8_t> bytes, std::size_t capacity) -> ByteDataPtr {
        auto data = ByteDataPtr{
            ByteData::create(static_cast<ByteData::SizeType>(bytes.size()), static_cast<ByteData::SizeType>(capacity))};
        auto index = std::size_t{0};
        for (const auto byte : bytes) {
            data.get()->data()[index] = Byte{byte};
            ++index;
        }
        return data;
    }
};
