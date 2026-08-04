// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteArray.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/mem/impl/SecureErase.hpp>
#include <erbsland/mem/impl/UnsafeByteBufferAccess.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/LoopResult.hpp>
#include <erbsland/util/LoopStatus.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

using el::mem::Byte;
using el::mem::ByteArray;
using el::mem::ByteBuffer;
using el::mem::ConstByteSpan;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;

TESTED_TARGETS(ByteBuffer ByteBufferData UnsafeByteBufferAccess)
class ByteBufferTest final : public el::UnitTest {
private:
    template <typename T>
    inline static constexpr bool hasData = requires(T &value) { value.data(); };

    template <typename T>
    inline static constexpr bool hasSize = requires(const T &value) { value.size(); };

    template <typename T>
    inline static constexpr bool hasIterator = requires(T &value) {
        value.begin();
        value.end();
    };

    template <typename T>
    inline static constexpr bool hasIndexOperator = requires(T &value) { value[std::size_t{}]; };

    template <typename T>
    inline static constexpr bool hasWritableOwningSpan =
        requires(T &value) { requires std::same_as<decltype(value.span()), el::mem::ByteSpan>; };

    template <typename T>
    inline static constexpr bool hasCommonByteRead = requires(
        const T &value,
        const ByteIndex index,
        const ByteLength length,
        const ByteRange range,
        const ConstByteSpan source) {
        { value.isEmpty() } -> std::same_as<bool>;
        { value.length() } -> std::same_as<ByteLength>;
        { value.endIndex() } -> std::same_as<ByteIndex>;
        { value.isEqualConstTime(source) } -> std::same_as<bool>;
        { value.get(index) } -> std::same_as<Byte>;
        { value.getOrThrow(index) } -> std::same_as<Byte>;
        { value.template getInteger<uint32_t>(index) } -> std::same_as<uint32_t>;
        { value.span() } -> std::convertible_to<ConstByteSpan>;
        { value.span(range) } -> std::same_as<ConstByteSpan>;
        { value.span(index, length) } -> std::same_as<ConstByteSpan>;
        {
            value.forEach([](const Byte, const ByteIndex) {})
        } -> std::same_as<el::util::LoopResult>;
    };

    template <typename T>
    inline static constexpr bool hasCommonByteWrite = requires(
        T &value,
        const ByteIndex index,
        const ByteLength length,
        const ByteRange range,
        const ConstByteSpan source,
        const Byte byte) {
        value.set(index, byte);
        value.setOrThrow(index, byte);
        value.xorAt(index, byte);
        value.xorAtOrThrow(index, byte);
        value.template setInteger<uint32_t>(index, 0U);
        value.fill(byte);
        value.fill(range, byte);
        value.overwrite(source);
        value.overwrite(index, source);
        value.overwrite(range, source);
        value.xorWith(source);
        value.xorWith(range, source);
        value.secureErase();
    };

    template <typename T>
    inline static constexpr bool hasCommonResizableByteWrite =
        hasCommonByteWrite<T> && requires(T &value, const ByteLength length) {
            value.resize(length);
            { value.capacity() } -> std::same_as<ByteLength>;
            value.reserve(length);
            value.shrinkToFit();
            value.clear();
            value.reset();
        };

    static_assert(!hasData<ByteBuffer>);
    static_assert(!hasSize<ByteBuffer>);
    static_assert(!hasIterator<ByteBuffer>);
    static_assert(!hasIndexOperator<ByteBuffer>);
    static_assert(!hasData<ByteArray<4>>);
    static_assert(!hasSize<ByteArray<4>>);
    static_assert(!hasIterator<ByteArray<4>>);
    static_assert(!hasIndexOperator<ByteArray<4>>);
    static_assert(!hasWritableOwningSpan<ByteBuffer>);
    static_assert(!hasWritableOwningSpan<ByteArray<4>>);
    static_assert(!hasWritableOwningSpan<el::mem::ByteBlock>);
    static_assert(!hasWritableOwningSpan<el::mem::ByteBlockEditor>);
    static_assert(std::same_as<decltype(std::declval<ByteBuffer &>().span()), ConstByteSpan>);
    static_assert(hasCommonByteRead<ByteArray<4>>);
    static_assert(hasCommonByteRead<ByteBuffer>);
    static_assert(hasCommonByteRead<el::mem::ByteBlock>);
    static_assert(hasCommonByteRead<el::mem::ByteBlockEditor>);
    static_assert(hasCommonByteWrite<ByteArray<4>>);
    static_assert(hasCommonByteWrite<ByteBuffer>);
    static_assert(hasCommonByteWrite<el::mem::ByteBlockEditor>);
    static_assert(hasCommonResizableByteWrite<ByteBuffer>);
    static_assert(hasCommonResizableByteWrite<el::mem::ByteBlockEditor>);

    struct EraseEvent final {
        std::size_t size{};
        bool isZero{};
    };

    inline static auto eraseEvents = std::vector<EraseEvent>{};

    static void observeErase(const std::span<const std::byte> bytes) noexcept {
        eraseEvents.push_back({bytes.size(), std::ranges::all_of(bytes, [](const std::byte value) noexcept -> bool {
                                   return value == std::byte{};
                               })});
    }

    class EraseObserverGuard final {
    public:
        EraseObserverGuard() {
            eraseEvents.clear();
            el::mem::impl::setSecureEraseObserver(observeErase);
        }
        ~EraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }
    };

public:
    void testConstructionAndDeepCopy() {
        auto empty = ByteBuffer{};
        REQUIRE(empty.isEmpty());
        REQUIRE(empty.span().empty());

        auto bytes = ByteBuffer{Byte{1U}, Byte{2U}, Byte{3U}};
        auto copy = bytes;
        bytes.set(ByteIndex{0U}, Byte{9U});
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({9U, 2U, 3U}));
        REQUIRE_EQUAL(copy.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));

        auto moved = ByteBuffer{std::move(copy)};
        REQUIRE_EQUAL(moved.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));
        const auto fixed = ByteArray{Byte{4U}, Byte{5U}};
        REQUIRE_EQUAL(ByteBuffer{fixed.span()}.toUInt8Vector(), std::vector<uint8_t>({4U, 5U}));
    }

    void testConstantTimeEquality() {
        const auto bytes = ByteBuffer{Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}};
        const auto same = ByteBuffer{Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}};
        const auto differentFirst = ByteBuffer{Byte{9U}, Byte{2U}, Byte{3U}, Byte{4U}};
        const auto differentMiddle = ByteBuffer{Byte{1U}, Byte{2U}, Byte{9U}, Byte{4U}};
        const auto differentLast = ByteBuffer{Byte{1U}, Byte{2U}, Byte{3U}, Byte{9U}};

        REQUIRE(bytes.isEqualConstTime(same));
        REQUIRE_FALSE(bytes.isEqualConstTime(differentFirst));
        REQUIRE_FALSE(bytes.isEqualConstTime(differentMiddle));
        REQUIRE_FALSE(bytes.isEqualConstTime(differentLast));
        REQUIRE(bytes.isEqualConstTime(same.span()));
        REQUIRE_FALSE(bytes.isEqualConstTime(same.span().first(3U)));
        REQUIRE(ByteBuffer{}.isEqualConstTime(ByteBuffer{}));
        REQUIRE(ByteBuffer{}.isEqualConstTime(ConstByteSpan{}));
    }

    void testStorageManagement() {
        auto bytes = ByteBuffer{ByteLength{3U}, Byte{7U}};
        bytes.reserve(ByteLength{32U});
        const auto capacity = bytes.capacity();
        REQUIRE_GREATER_EQUAL(capacity, ByteLength{32U});

        bytes.resize(ByteLength{5U});
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({7U, 7U, 7U, 0U, 0U}));
        bytes.resize(ByteLength{2U});
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({7U, 7U}));
        bytes.clear();
        REQUIRE(bytes.isEmpty());
        REQUIRE_EQUAL(bytes.capacity(), capacity);
        bytes.append(Byte{1U});
        bytes.shrinkToFit();
        REQUIRE_EQUAL(bytes.capacity(), bytes.length());
        bytes.reset();
        REQUIRE(bytes.isEmpty());
        REQUIRE(bytes.capacity().isZero());
    }

    void testUnsafeWritableAccess() {
        auto bytes = ByteBuffer{ByteLength{3U}};
        auto access = el::mem::impl::UnsafeByteBufferAccess{bytes};
        auto writable = access.writableData();

        writable[0U] = Byte{1U};
        writable[1U] = Byte{2U};
        writable[2U] = Byte{3U};
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));
    }

    void testReadWriteAndIteration() {
        auto bytes = ByteBuffer{ByteLength{12U}};
        const auto borrowed = bytes.span();
        bytes.set(ByteIndex{0U}, Byte{1U});
        bytes.setOrThrow(ByteIndex{1U}, Byte{2U});
        REQUIRE_EQUAL(borrowed[0U], Byte{1U});
        REQUIRE_EQUAL(bytes.get(ByteIndex{0U}), Byte{1U});
        REQUIRE_EQUAL(bytes.get(ByteIndex{99U}, Byte{9U}), Byte{9U});
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, bytes.getOrThrow(ByteIndex::noIndex()));

        REQUIRE(bytes.setInteger(ByteIndex{2U}, uint32_t{0x12345678U}));
        bytes.setIntegerOrThrow(ByteIndex{6U}, uint32_t{0xaabbccddU}, el::mem::Endianness::Big);
        REQUIRE_EQUAL(bytes.getInteger<uint32_t>(ByteIndex{2U}), uint32_t{0x12345678U});
        REQUIRE_EQUAL(bytes.getInteger<uint32_t>(ByteIndex{6U}, el::mem::Endianness::Big), uint32_t{0xaabbccddU});

        auto indexes = std::vector<std::size_t>{};
        const auto result = bytes.forEach([&indexes](const Byte, const ByteIndex index) {
            indexes.emplace_back(index.toSizeT());
            return index == ByteIndex{2U} ? el::util::LoopStatus::Stop : el::util::LoopStatus::Continue;
        });
        REQUIRE_EQUAL(result, el::util::LoopResult::Stopped);
        REQUIRE_EQUAL(indexes, std::vector<std::size_t>({0U, 1U, 2U}));

        const auto shortenedSpanSize = bytes.span(ByteIndex{10U}, ByteLength{9U}).size();
        REQUIRE_EQUAL(shortenedSpanSize, std::size_t{2U});
        REQUIRE(bytes.span(ByteRange::noRange()).empty());
    }

    void testOverwriteFillAndXor() {
        auto indexed = ByteBuffer{Byte{0x55U}, Byte{0xaaU}};
        indexed.xorAt(ByteIndex{0U}, Byte{0xffU});
        indexed.xorAt(ByteIndex::noIndex(), Byte{0xffU});
        indexed.xorAtOrThrow(ByteIndex{1U}, Byte{0x0fU});
        REQUIRE_EQUAL(indexed.toUInt8Vector(), std::vector<uint8_t>({0xaaU, 0xa5U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, indexed.xorAtOrThrow(ByteIndex{2U}, Byte{1U}));

        auto bytes = ByteBuffer{Byte{0U}, Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}, Byte{5U}};
        const auto shortSource = ByteArray{Byte{9U}, Byte{8U}};
        bytes.overwrite(ByteRange{ByteIndex{1U}, ByteLength{4U}}, shortSource.span());
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({0U, 9U, 8U, 3U, 4U, 5U}));

        const auto longSource = ByteArray{Byte{7U}, Byte{6U}, Byte{5U}, Byte{4U}};
        bytes.overwrite(ByteRange{ByteIndex{4U}, ByteLength::infinite()}, longSource.span());
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({0U, 9U, 8U, 3U, 7U, 6U}));
        bytes.overwrite(ByteIndex{99U}, longSource.span());
        bytes.overwrite(ByteRange::noRange(), longSource.span());
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({0U, 9U, 8U, 3U, 7U, 6U}));

        auto overlap = ByteBuffer{Byte{0U}, Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}};
        overlap.overwrite(ByteIndex{1U}, overlap.span(ByteIndex{0U}, ByteLength{4U}));
        REQUIRE_EQUAL(overlap.toUInt8Vector(), std::vector<uint8_t>({0U, 0U, 1U, 2U, 3U}));
        overlap.overwrite(ByteRange{ByteIndex{0U}, ByteLength{4U}}, overlap.span(ByteIndex{1U}, ByteLength{4U}));
        REQUIRE_EQUAL(overlap.toUInt8Vector(), std::vector<uint8_t>({0U, 1U, 2U, 3U, 3U}));

        bytes.fill(ByteRange{ByteIndex{1U}, ByteLength{3U}}, Byte{0xaaU});
        bytes.xorWith(
            ByteRange{ByteIndex{1U}, ByteLength{3U}}, ByteArray{Byte{0xffU}, Byte{0x0fU}, Byte{0xf0U}}.span());
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({0U, 0x55U, 0xa5U, 0x5aU, 7U, 6U}));

        auto xorOverlap = ByteBuffer{Byte{1U}, Byte{2U}, Byte{4U}, Byte{8U}, Byte{16U}};
        xorOverlap.xorWith(ByteRange{ByteIndex{1U}, ByteLength{4U}}, xorOverlap.span(ByteIndex{0U}, ByteLength{4U}));
        REQUIRE_EQUAL(xorOverlap.toUInt8Vector(), std::vector<uint8_t>({1U, 3U, 6U, 12U, 24U}));
        xorOverlap.xorWith(ByteRange{ByteIndex{0U}, ByteLength{4U}}, xorOverlap.span(ByteIndex{1U}, ByteLength{4U}));
        REQUIRE_EQUAL(xorOverlap.toUInt8Vector(), std::vector<uint8_t>({2U, 5U, 10U, 20U, 24U}));

        auto strict = ByteBuffer{Byte{0xf0U}, Byte{0x0fU}};
        strict.xorWithOrThrow(ByteArray{Byte{0xaaU}, Byte{0x55U}}.span());
        REQUIRE_EQUAL(strict.toUInt8Vector(), std::vector<uint8_t>({0x5aU, 0x5aU}));
        const auto strictSnapshot = strict;
        REQUIRE_THROWS_AS(el::err::ParameterError, strict.xorWithOrThrow(ByteArray{Byte{1U}}.span()));
        REQUIRE_EQUAL(strict, strictSnapshot);
    }

    void testEditOperations() {
        auto bytes = ByteBuffer{Byte{1U}, Byte{2U}, Byte{3U}};
        bytes.append(Byte{4U})
            .append(ByteArray{Byte{5U}, Byte{6U}}.span())
            .insert(ByteIndex{1U}, ByteArray{Byte{8U}, Byte{9U}}.span())
            .replace(ByteRange{ByteIndex{2U}, ByteLength{4U}}, ByteArray{Byte{7U}}.span());
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({1U, 8U, 7U, 5U, 6U}));
        bytes.remove(ByteRange{ByteIndex{1U}, ByteLength{2U}});
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({1U, 5U, 6U}));
        bytes.keep(ByteRange{ByteIndex{1U}, ByteLength::infinite()});
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({5U, 6U}));
    }

    void testSensitiveModeLifecycleAndCopies() {
        const auto observer = EraseObserverGuard{};
        auto bytes = ByteBuffer{Byte{1U}, Byte{2U}, Byte{3U}};
        bytes.reserve(ByteLength{32U});
        const auto capacity = bytes.capacity();

        bytes.setSensitive(true);
        REQUIRE(bytes.isSensitive());
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));

        auto copy = bytes;
        REQUIRE(copy.isSensitive());
        copy.set(ByteIndex{}, Byte{9U});
        REQUIRE_EQUAL(bytes.get(ByteIndex{}), Byte{1U});

        auto moved = ByteBuffer{std::move(copy)};
        REQUIRE(moved.isSensitive());
        REQUIRE_FALSE(copy.isSensitive());

        eraseEvents.clear();
        bytes.clear();
        REQUIRE(bytes.isSensitive());
        REQUIRE(bytes.isEmpty());
        REQUIRE_EQUAL(bytes.capacity(), capacity);
        REQUIRE_FALSE(eraseEvents.empty());

        bytes.resize(ByteLength{4U}).fill(Byte{0x55U});
        eraseEvents.clear();
        bytes.setSensitive(false);
        REQUIRE_FALSE(bytes.isSensitive());
        REQUIRE(bytes.isEmpty());
        REQUIRE_EQUAL(bytes.capacity(), capacity);
        REQUIRE_FALSE(eraseEvents.empty());
    }

    void testSensitiveMutationErasesDiscardedStorage() {
        const auto observer = EraseObserverGuard{};
        auto bytes = ByteBuffer{ByteLength{8U}, Byte{0xaaU}};
        bytes.setSensitive(true);

        eraseEvents.clear();
        bytes.reserve(ByteLength{64U});
        REQUIRE_FALSE(eraseEvents.empty());

        eraseEvents.clear();
        bytes.resize(ByteLength{4U});
        REQUIRE_FALSE(eraseEvents.empty());

        eraseEvents.clear();
        bytes.remove(ByteRange{ByteIndex{1U}, ByteLength{2U}});
        REQUIRE_FALSE(eraseEvents.empty());

        eraseEvents.clear();
        bytes.shrinkToFit();
        REQUIRE_FALSE(eraseEvents.empty());

        bytes.reset();
        REQUIRE(bytes.isSensitive());
        REQUIRE(bytes.capacity().isZero());
    }

    void testSensitiveAssignmentsAdoptModeAndReleaseDestinations() {
        const auto observer = EraseObserverGuard{};
        auto sensitive = ByteBuffer{Byte{1U}, Byte{2U}, Byte{3U}};
        sensitive.setSensitive(true);
        auto ordinary = ByteBuffer{Byte{8U}, Byte{9U}};

        eraseEvents.clear();
        sensitive = ordinary;
        REQUIRE_FALSE(sensitive.isSensitive());
        REQUIRE_EQUAL(sensitive.toUInt8Vector(), std::vector<uint8_t>({8U, 9U}));
        REQUIRE_FALSE(eraseEvents.empty());

        auto markedSource = ByteBuffer{Byte{4U}, Byte{5U}};
        markedSource.setSensitive(true);
        ordinary = markedSource;
        REQUIRE(ordinary.isSensitive());
        REQUIRE_EQUAL(ordinary.toUInt8Vector(), std::vector<uint8_t>({4U, 5U}));

        eraseEvents.clear();
        ordinary = ByteBuffer{Byte{7U}};
        REQUIRE_FALSE(ordinary.isSensitive());
        REQUIRE_EQUAL(ordinary.toUInt8Vector(), std::vector<uint8_t>({7U}));
        REQUIRE_FALSE(eraseEvents.empty());
    }

    void testOrdinaryModeDoesNotEraseAutomatically() {
        const auto observer = EraseObserverGuard{};
        {
            auto bytes = ByteBuffer{ByteLength{8U}, Byte{0xaaU}};
            bytes.reserve(ByteLength{64U});
            eraseEvents.clear();
            bytes.resize(ByteLength{4U});
            bytes.remove(ByteRange{ByteIndex{1U}, ByteLength{2U}});
            bytes.clear();
        }
        REQUIRE(eraseEvents.empty());
    }

    void testSecureEraseCompleteCapacity() {
        const auto observer = EraseObserverGuard{};
        auto bytes = ByteBuffer{Byte{1U}, Byte{2U}, Byte{3U}};
        bytes.reserve(ByteLength{64U});
        const auto length = bytes.length();
        const auto capacity = bytes.capacity();

        bytes.secureErase();

        REQUIRE_EQUAL(bytes.length(), length);
        REQUIRE_EQUAL(bytes.capacity(), capacity);
        REQUIRE_EQUAL(bytes.toUInt8Vector(), std::vector<uint8_t>({0U, 0U, 0U}));
        REQUIRE_EQUAL(eraseEvents.size(), std::size_t{1U});
        const auto erasedSize = eraseEvents.front().size;
        REQUIRE_EQUAL(erasedSize, capacity.toSizeT());
        REQUIRE(eraseEvents.front().isZero);
    }
};
