// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteArray.hpp>
#include <erbsland/mem/ByteRingBuffer.hpp>
#include <erbsland/mem/impl/SecureErase.hpp>
#include <erbsland/mem/impl/UnsafeRingBufferAccess.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

using el::mem::Byte;
using el::unit::ByteLength;

namespace {

struct RingEraseEvent final {
    std::size_t size{};
    bool isZero{};
};

std::vector<RingEraseEvent> gRingEraseEvents;

void observeRingErase(const std::span<const std::byte> bytes) noexcept {
    gRingEraseEvents.push_back(
        {bytes.size(), std::ranges::all_of(bytes, [](const std::byte value) { return value == std::byte{}; })});
}

class RingEraseObserverGuard final {
public:
    RingEraseObserverGuard() {
        gRingEraseEvents.clear();
        el::mem::impl::setSecureEraseObserver(observeRingErase);
    }
    ~RingEraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }
};

[[nodiscard]] auto hasEraseOfSize(const std::size_t size) -> bool {
    return std::ranges::any_of(
        gRingEraseEvents, [size](const RingEraseEvent &event) { return event.size == size && event.isZero; });
}

}

TESTED_TARGETS(RingBuffer ByteRingBuffer UnsafeRingBufferAccess)
class RingBufferTest final : public el::UnitTest {
public:
    void testSafeWrapAround() {
        auto buffer = el::mem::RingBuffer{ByteLength{4U}};
        const auto first = el::mem::ByteArray{Byte{1U}, Byte{2U}, Byte{3U}};
        REQUIRE_EQUAL(buffer.write(first.span()), ByteLength{3U});

        auto read = std::array<Byte, 2>{};
        REQUIRE_EQUAL(buffer.read(el::mem::ByteSpan{read}), ByteLength{2U});
        const auto second = el::mem::ByteArray{Byte{4U}, Byte{5U}, Byte{6U}};
        REQUIRE_EQUAL(buffer.write(second.span()), ByteLength{3U});

        auto result = std::array<Byte, 4>{};
        REQUIRE_EQUAL(buffer.read(el::mem::ByteSpan{result}), ByteLength{4U});
        REQUIRE_EQUAL(
            std::vector<uint8_t>({result[0].toUInt8(), result[1].toUInt8(), result[2].toUInt8(), result[3].toUInt8()}),
            std::vector<uint8_t>({3U, 4U, 5U, 6U}));
    }

    void testGrowingWriteIsAtomic() {
        auto buffer = el::mem::RingBuffer{ByteLength{2U}, ByteLength{5U}};
        const auto first = el::mem::ByteArray{Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}};
        REQUIRE(isSuccessful(buffer.writeExact(first.span())));
        REQUIRE_GREATER_EQUAL(buffer.capacity(), ByteLength{4U});

        const auto tooLarge = el::mem::ByteArray{Byte{5U}, Byte{6U}};
        REQUIRE(isFailure(buffer.writeExact(tooLarge.span())));
        REQUIRE_EQUAL(buffer.length(), ByteLength{4U});
    }

    void testUnsafeNativeAccess() {
        auto buffer = el::mem::RingBuffer{ByteLength{4U}};
        {
            auto access = el::mem::impl::UnsafeRingBufferAccess{buffer};
            auto spans = access.writableSpans();
            spans[0][0] = Byte{0xaaU};
            spans[0][1] = Byte{0xbbU};
            access.commitWritten(ByteLength{2U});
            REQUIRE_THROWS(buffer.write(el::mem::ByteArray{Byte{0xccU}}.span()));
        }
        const auto result = buffer.read(ByteLength::infinite());
        REQUIRE_EQUAL(result.toUInt8Vector(), std::vector<uint8_t>({0xaaU, 0xbbU}));
    }

    void testEndianIntegerOperations() {
        auto buffer = el::mem::ByteRingBuffer{ByteLength{8U}};
        buffer.setEndianness(el::mem::Endianness::Big);
        REQUIRE(isSuccessful(buffer.writeInteger<uint32_t>(0x12345678U)));
        REQUIRE_EQUAL(buffer.readInteger<uint32_t>().value(), uint32_t{0x12345678U});
    }

    void testSensitiveConsumptionWrapAndUnsafeAccess() {
        const auto observer = RingEraseObserverGuard{};
        auto buffer = el::mem::RingBuffer{ByteLength{4U}};
        buffer.setSensitive(true);
        REQUIRE(buffer.isSensitive());
        REQUIRE_EQUAL(buffer.write(el::mem::ByteArray{Byte{1U}, Byte{2U}, Byte{3U}}.span()), ByteLength{3U});

        auto first = std::array<Byte, 2>{};
        REQUIRE_EQUAL(buffer.read(el::mem::ByteSpan{first}), ByteLength{2U});
        REQUIRE(hasEraseOfSize(2U));
        REQUIRE_EQUAL(buffer.write(el::mem::ByteArray{Byte{4U}, Byte{5U}, Byte{6U}}.span()), ByteLength{3U});

        gRingEraseEvents.clear();
        {
            auto access = el::mem::impl::UnsafeRingBufferAccess{buffer};
            access.consumeRead(ByteLength{3U});
        }
        REQUIRE(hasEraseOfSize(2U));
        REQUIRE(hasEraseOfSize(1U));
        REQUIRE_EQUAL(buffer.length(), ByteLength{1U});
    }

    void testSensitiveGrowthShrinkClearAndExplicitErase() {
        const auto observer = RingEraseObserverGuard{};
        auto buffer = el::mem::RingBuffer{ByteLength{2U}, ByteLength{8U}};
        buffer.setSensitive(true);
        REQUIRE(isSuccessful(buffer.writeExact(el::mem::ByteArray{Byte{1U}, Byte{2U}, Byte{3U}}.span())));
        REQUIRE(hasEraseOfSize(2U));

        gRingEraseEvents.clear();
        buffer.clear();
        REQUIRE(buffer.isEmpty());
        REQUIRE(hasEraseOfSize(buffer.capacity().toSizeT()));

        REQUIRE(isSuccessful(buffer.writeExact(el::mem::ByteArray{Byte{4U}, Byte{5U}, Byte{6U}}.span())));
        const auto grownCapacity = buffer.capacity();
        static_cast<void>(buffer.read(ByteLength::infinite()));
        gRingEraseEvents.clear();
        buffer.shrinkToInitial();
        REQUIRE_EQUAL(buffer.capacity(), ByteLength{2U});
        REQUIRE(hasEraseOfSize(grownCapacity.toSizeT()));

        REQUIRE_EQUAL(buffer.write(el::mem::ByteArray{Byte{7U}}.span()), ByteLength{1U});
        gRingEraseEvents.clear();
        buffer.secureErase();
        REQUIRE(buffer.isEmpty());
        REQUIRE(buffer.isSensitive());
        REQUIRE(hasEraseOfSize(2U));
    }

    void testSensitiveDisableSwapAndDestruction() {
        const auto observer = RingEraseObserverGuard{};
        {
            auto first = el::mem::RingBuffer{ByteLength{3U}};
            auto second = el::mem::RingBuffer{ByteLength{5U}};
            first.setSensitive(true);
            REQUIRE_EQUAL(first.write(el::mem::ByteArray{Byte{1U}, Byte{2U}}.span()), ByteLength{2U});
            first.swap(second);
            REQUIRE_FALSE(first.isSensitive());
            REQUIRE(second.isSensitive());
            REQUIRE_EQUAL(second.length(), ByteLength{2U});

            gRingEraseEvents.clear();
            second.setSensitive(false);
            REQUIRE_FALSE(second.isSensitive());
            REQUIRE(second.isEmpty());
            REQUIRE(hasEraseOfSize(3U));

            second.setSensitive(true);
            REQUIRE_EQUAL(second.write(el::mem::ByteArray{Byte{9U}}.span()), ByteLength{1U});
            gRingEraseEvents.clear();
        }
        REQUIRE(hasEraseOfSize(3U));
    }
};
