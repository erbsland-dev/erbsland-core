// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/impl/SecureErase.hpp>
#include <erbsland/mem/impl/SharedByteDataWithFlag.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>
#include <vector>

namespace {

struct SharedByteEraseEvent final {
    std::size_t size{};
    bool isZero{};
};

std::vector<SharedByteEraseEvent> gSharedByteEraseEvents;

void observeSharedByteErase(const std::span<const std::byte> bytes) noexcept {
    gSharedByteEraseEvents.push_back(
        {bytes.size(),
            std::ranges::all_of(bytes, [](const std::byte value) noexcept -> bool { return value == std::byte{}; })});
}

class SharedByteEraseObserverGuard final {
public:
    SharedByteEraseObserverGuard() {
        gSharedByteEraseEvents.clear();
        el::mem::impl::setSecureEraseObserver(observeSharedByteErase);
    }
    ~SharedByteEraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }
};

}

TESTED_TARGETS(SharedByteDataWithFlag)
class SharedByteDataWithFlagTest final : public el::UnitTest {
public:
    void testLayoutAndAccess() {
        using CharData = el::mem::impl::SharedByteDataWithFlag<char>;
        static_assert(std::is_base_of_v<el::mem::SharedData, CharData>);
        static_assert(std::same_as<CharData::SizeType, std::uint32_t>);
        static_assert(sizeof(CharData::DataType) == 1U);

        auto *data = CharData::create(3U, 8U, std::uint8_t{0x80U});
        REQUIRE_EQUAL(data->size(), std::uint32_t{3U});
        REQUIRE_EQUAL(data->capacity(), std::uint32_t{8U});
        REQUIRE_EQUAL(data->flags(), std::uint8_t{0x80U});
        REQUIRE(
            reinterpret_cast<const std::byte *>(data->data()) ==
            reinterpret_cast<const std::byte *>(data) + CharData::allocationOverhead());
        data->setSize(5U);
        data->setFlags(std::uint8_t{0xa0U});
        REQUIRE_EQUAL(data->size(), std::uint32_t{5U});
        REQUIRE_EQUAL(data->flags(), std::uint8_t{0xa0U});
        CharData::destroy(data);
    }

    void testFlagsAndClone() {
        using ByteData = el::mem::impl::SharedByteDataWithFlag<el::mem::Byte>;
        auto *data = ByteData::create(4U, 12U, std::uint8_t{0x40U});
        for (auto i = std::size_t{}; i < 4U; ++i) {
            data->data()[i] = el::mem::Byte{static_cast<std::uint8_t>(i + 1U)};
        }
        REQUIRE_FALSE(data->isSensitive());
        data->setSensitive();
        REQUIRE(data->isSensitive());
        REQUIRE_EQUAL(data->flags(), std::uint8_t{0x41U});

        auto *clone = data->clone();
        REQUIRE(clone->isSensitive());
        REQUIRE_EQUAL(clone->flags(), std::uint8_t{0x41U});
        REQUIRE_EQUAL(clone->size(), std::uint32_t{4U});
        REQUIRE_EQUAL(clone->capacity(), std::uint32_t{12U});
        for (auto i = std::size_t{}; i < 4U; ++i) {
            REQUIRE_EQUAL(clone->data()[i], data->data()[i]);
        }
        ByteData::destroy(data);
        ByteData::destroy(clone);
    }

    void testAllocationBounds() {
        using Data = el::mem::impl::SharedByteDataWithFlag<char>;
        REQUIRE_FALSE(Data::canAllocateWithCapacity(-1));
        REQUIRE_FALSE(Data::canAllocateWithCapacity(std::numeric_limits<std::uint64_t>::max()));
        REQUIRE(Data::canAllocateWithCapacity(std::uint32_t{1024U}));
        REQUIRE_EQUAL(
            Data::allocationSizeForCapacity(std::uint32_t{1024U}), Data::allocationOverhead() + std::size_t{1024U});
    }

    void testSensitiveDestructionErasesCompleteAllocation() {
        using Data = el::mem::impl::SharedByteDataWithFlag<char>;
        const auto guard = SharedByteEraseObserverGuard{};
        auto *data = Data::create(4U, 32U);
        std::ranges::fill(std::span<char>{data->data(), data->capacity()}, '\x5a');
        data->setSensitive();
        Data::destroy(data);

        REQUIRE_EQUAL(gSharedByteEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(gSharedByteEraseEvents.front().size, Data::allocationSizeForCapacity(std::uint32_t{32U}));
        REQUIRE(gSharedByteEraseEvents.front().isZero);
    }
};
