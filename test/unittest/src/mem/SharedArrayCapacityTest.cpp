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
        REQUIRE_EQUAL((el::mem::impl::BestGrowth{ByteLength{64U}, ByteLength{32U}}.bestGrowth()), ByteLength{64U});
        REQUIRE_EQUAL((el::mem::impl::BestGrowth{64U, 32U}.bestGrowth<ByteData>()), 64U);
    }

    void testBestGrowthSmallAllocationIncludesOverhead() {
        const auto capacity = el::mem::impl::BestGrowth{0U, 1U}.bestGrowth<ByteData>();
        const auto requestedAllocationSize = ByteData::allocationSizeForCapacity(1U);
        const auto grownAllocationSize = ByteData::allocationSizeForCapacity(capacity);

        REQUIRE_GREATER_EQUAL(capacity, 1U);
        REQUIRE_GREATER(grownAllocationSize, requestedAllocationSize);
        REQUIRE(isPowerOfTwo(grownAllocationSize));
    }

    void testBestGrowthExactBoundaryStepsToNextPowerOfTwo() {
        constexpr auto cBoundarySize = std::size_t{128U};
        static_assert(ByteData::allocationOverhead() < cBoundarySize);
        const auto requestedCapacity = cBoundarySize - ByteData::allocationOverhead();

        const auto capacity = el::mem::impl::BestGrowth{0U, requestedCapacity}.bestGrowth<ByteData>();

        REQUIRE_EQUAL(ByteData::allocationSizeForCapacity(requestedCapacity), cBoundarySize);
        REQUIRE_EQUAL(ByteData::allocationSizeForCapacity(capacity), cBoundarySize * 2U);
    }

    void testBestGrowthLargeAllocationRoundsToBlock() {
        const auto blockSize = el::mem::impl::BestGrowth::cMaximumGrowthBlock.toSizeT();
        const auto requestedAllocationSize = blockSize + 1U;
        const auto requestedCapacity = requestedAllocationSize - ByteData::allocationOverhead();

        const auto capacity = el::mem::impl::BestGrowth{0U, requestedCapacity}.bestGrowth<ByteData>();

        REQUIRE_EQUAL(ByteData::allocationSizeForCapacity(capacity), blockSize * 2U);
    }

    void testBestGeometricGrowthUsesPortablePages() {
        using el::mem::impl::BestGrowthStrategy;
        const auto page = el::mem::impl::BestGrowth::cAllocationPageSize.toSizeT();

        REQUIRE_EQUAL(
            (el::mem::impl::BestGrowth{ByteLength{}, ByteLength{1U}}.bestGrowth(BestGrowthStrategy::Geometric)),
            ByteLength{page});
        REQUIRE_EQUAL(
            (el::mem::impl::BestGrowth{ByteLength{page}, ByteLength{page + 1U}}.bestGrowth(
                BestGrowthStrategy::Geometric)),
            ByteLength{page * 2U});
        REQUIRE_EQUAL(
            (el::mem::impl::BestGrowth{ByteLength{page + 1U}, ByteLength{page * 3U}}.bestGrowth(
                BestGrowthStrategy::Geometric)),
            ByteLength{page * 4U});
        REQUIRE_EQUAL(
            (el::mem::impl::BestGrowth{ByteLength{page}, ByteLength{page * 100U}}.bestGrowth(
                BestGrowthStrategy::Geometric)),
            ByteLength{page * 128U});
        REQUIRE_EQUAL(
            (el::mem::impl::BestGrowth{ByteLength{page}, ByteLength::maximum()}.bestGrowth(
                BestGrowthStrategy::Geometric)),
            ByteLength::maximum());
    }

    void testBestGeometricCapacityIncludesOverheadAndIsPageAligned() {
        using el::mem::impl::BestGrowthStrategy;
        const auto capacity = el::mem::impl::BestGrowth{0U, 5000U}.bestGrowth<ByteData>(BestGrowthStrategy::Geometric);
        const auto allocationSize = ByteData::allocationSizeForCapacity(capacity);

        REQUIRE_GREATER_EQUAL(capacity, 5000U);
        REQUIRE_EQUAL(allocationSize % el::mem::impl::BestGrowth::cAllocationPageSize.toSizeT(), 0U);

        using WideData = el::mem::SharedArrayData<std::uint32_t, std::uint64_t>;
        const auto wideCapacity =
            el::mem::impl::BestGrowth{0U, 5000U}.bestGrowth<WideData>(BestGrowthStrategy::Geometric);
        const auto wideAllocationSize = WideData::allocationSizeForCapacity(wideCapacity);
        const auto wideTarget =
            el::mem::impl::BestGrowth{ByteLength{}, ByteLength::fromSizeT(WideData::allocationSizeForCapacity(5000U))}
                .bestGrowth(BestGrowthStrategy::Geometric);
        REQUIRE_GREATER_EQUAL(wideCapacity, 5000U);
        REQUIRE_EQUAL(wideTarget.toSizeT() % el::mem::impl::BestGrowth::cAllocationPageSize.toSizeT(), 0U);
        const auto wideTargetSize = wideTarget.toSizeT();
        const auto unusedBytes = wideTargetSize - wideAllocationSize;
        REQUIRE_LESS_EQUAL(wideAllocationSize, wideTargetSize);
        REQUIRE_LESS(unusedBytes, sizeof(WideData::DataType));
    }

    void testBestGrowthNearMaximumStaysAllocatable() {
        const auto maxCapacity =
            (std::numeric_limits<std::size_t>::max() - ByteData::allocationOverhead()) / sizeof(ByteData::DataType);
        const auto requestedCapacity = maxCapacity - 1U;

        const auto capacity = el::mem::impl::BestGrowth{0U, requestedCapacity}.bestGrowth<ByteData>();

        REQUIRE_GREATER_EQUAL(capacity, requestedCapacity);
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
        const auto expectedCapacity = el::mem::impl::BestGrowth{2U, 24U}.bestGrowth<ByteData>();

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
