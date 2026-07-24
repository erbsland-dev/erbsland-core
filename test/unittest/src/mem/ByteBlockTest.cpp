// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteArray.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/mem/ByteSpan.hpp>
#include <erbsland/mem/Endianness.hpp>
#include <erbsland/mem/impl/BestGrowth.hpp>
#include <erbsland/mem/impl/ByteBlockData.hpp>
#include <erbsland/mem/impl/SecureErase.hpp>
#include <erbsland/mem/impl/UnsafeByteBlockAccess.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

using el::mem::Byte;
using el::mem::ByteArray;
using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;
using el::mem::ByteBuffer;
using el::mem::ConstByteSpan;
using el::mem::Endianness;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;

namespace {

template <typename T>
concept HasPublicBytes = requires(const T &value) { value.bytes(); };

template <typename T>
concept HasPublicToSpan = requires(T &value) { value.toSpan(); };

template <typename T>
concept HasSpanConstructor = std::constructible_from<T, ConstByteSpan>;

template <typename T>
concept HasReadSpan = requires(const T &value) {
    { value.span() } -> std::same_as<ConstByteSpan>;
};

template <typename T>
concept HasSpanSearch = requires(const T &value, const ConstByteSpan bytes) {
    value.startsWith(bytes);
    value.find(bytes);
};

template <typename T>
concept HasSpanAppend = requires(T &value, const ConstByteSpan bytes) { value.append(bytes); };

template <typename T>
concept HasSpanOverwrite =
    requires(T &value, const ConstByteSpan bytes) { value.overwrite(ByteRange::empty(), bytes); };

template <typename T>
concept HasUnmarkSensitive = requires(T &value) { value.unmarkAsSensitive(); };

static_assert(!HasPublicBytes<ByteBlock>);
static_assert(!HasPublicBytes<ByteBlockEditor>);
static_assert(!HasPublicToSpan<ByteBlock>);
static_assert(!HasPublicToSpan<ByteBlockEditor>);
static_assert(!HasSpanConstructor<ByteBlock>);
static_assert(!HasSpanConstructor<ByteBlockEditor>);
static_assert(HasReadSpan<ByteBlock>);
static_assert(HasReadSpan<ByteBlockEditor>);
static_assert(HasSpanSearch<ByteBlock>);
static_assert(HasSpanSearch<ByteBlockEditor>);
static_assert(HasSpanAppend<ByteBlockEditor>);
static_assert(HasSpanOverwrite<ByteBlockEditor>);
static_assert(!HasUnmarkSensitive<ByteBlock>);
static_assert(!HasUnmarkSensitive<ByteBlockEditor>);

struct ByteBlockEraseEvent final {
    std::size_t size{};
    bool isZero{};
};

std::vector<ByteBlockEraseEvent> gByteBlockEraseEvents;

void observeByteBlockErase(const std::span<const std::byte> bytes) noexcept {
    gByteBlockEraseEvents.push_back(
        {bytes.size(),
            std::ranges::all_of(bytes, [](const std::byte value) noexcept -> bool { return value == std::byte{}; })});
}

class ByteBlockEraseObserverGuard final {
public:
    ByteBlockEraseObserverGuard() {
        gByteBlockEraseEvents.clear();
        el::mem::impl::setSecureEraseObserver(observeByteBlockErase);
    }
    ~ByteBlockEraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }
};

}

TESTED_TARGETS(Byte ByteBlock ByteBlockEditor)
class ByteBlockTest final : public el::UnitTest {
public:
    void testByteHelpers() {
        const auto byte = Byte{0b10101100U};

        REQUIRE_EQUAL(byte.toUInt8(), uint8_t{0b10101100U});
        REQUIRE_EQUAL(byte.masked(0b11110000U), uint8_t{0b10100000U});
        REQUIRE(byte.matches(0b11100000U, 0b10100000U));
        REQUIRE_FALSE(byte.matches(0b11110000U, 0b11110000U));
    }

    void testConstructionAndConversion() {
        const auto empty = ByteBlock{};
        REQUIRE(empty.isEmpty());
        REQUIRE(empty.length().isZero());

        const auto initialized = ByteBlock({0xf0U, 0x28U, 0x8cU, 0x28U});
        REQUIRE_EQUAL(initialized.toUInt8Vector(), std::vector<uint8_t>({0xf0U, 0x28U, 0x8cU, 0x28U}));
        REQUIRE(ByteBlock({}).isEmpty());
        REQUIRE_EQUAL(ByteBlockEditor({1U, 2U}).toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));

        const auto filled = ByteBlock{ByteLength{3U}, Byte{0xabu}};
        REQUIRE_EQUAL(filled.toUInt8Vector(), std::vector<uint8_t>({0xabU, 0xabU, 0xabU}));

        const auto fromBytes = ByteBlock::fromSpan(ByteBuffer{Byte{1U}, Byte{2U}}.span());
        REQUIRE_EQUAL(fromBytes.toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));

        const auto fromUInt8 = ByteBlock::fromVector(std::vector<uint8_t>{3U, 4U});
        REQUIRE_EQUAL(fromUInt8.toByteBuffer(), ByteBuffer({Byte{3U}, Byte{4U}}));

        const auto fromChar = ByteBlock::fromVector(std::vector<char>{'\x05', '\x06'});
        REQUIRE_EQUAL(fromChar.toCharVector(), std::vector<char>({'\x05', '\x06'}));

        const auto erbslandBytes = ByteBuffer{Byte{7U}, Byte{8U}};
        const auto standardBytes = std::array<std::byte, 2>{std::byte{9U}, std::byte{10U}};
        const auto unsignedBytes = std::array<uint8_t, 2>{11U, 12U};
        const auto characterBytes = std::array<char, 2>{'\x0d', '\x0e'};
        REQUIRE_EQUAL(ByteBlock::fromSpan(erbslandBytes.span()).toUInt8Vector(), std::vector<uint8_t>({7U, 8U}));
        REQUIRE_EQUAL(ByteBlock::fromSpan(std::span{standardBytes}).toUInt8Vector(), std::vector<uint8_t>({9U, 10U}));
        REQUIRE_EQUAL(ByteBlock::fromSpan(std::span{unsignedBytes}).toUInt8Vector(), std::vector<uint8_t>({11U, 12U}));
        REQUIRE_EQUAL(ByteBlock::fromSpan(std::span{characterBytes}).toUInt8Vector(), std::vector<uint8_t>({13U, 14U}));
        REQUIRE_EQUAL(ByteBlockEditor::fromSpan(erbslandBytes.span()).toUInt8Vector(), std::vector<uint8_t>({7U, 8U}));
        REQUIRE_EQUAL(
            ByteBlockEditor::fromVector(std::vector<uint8_t>{9U, 10U}).toUInt8Vector(),
            std::vector<uint8_t>({9U, 10U}));
        REQUIRE_EQUAL(
            ByteBlockEditor::fromVector(std::vector<char>{'\x0b', '\x0c'}).toUInt8Vector(),
            std::vector<uint8_t>({11U, 12U}));
        REQUIRE_EQUAL(
            ByteBlockEditor::fromSpan(std::span{standardBytes}).toUInt8Vector(), std::vector<uint8_t>({9U, 10U}));
    }

    void testByteTypeIntegration() {
        const auto fixed = ByteArray{Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}};
        const auto vector = ByteBuffer{Byte{2U}, Byte{3U}};
        const auto block = ByteBlock{fixed};
        const auto editor = ByteBlockEditor{ByteArray{Byte{5U}, Byte{6U}}};
        const auto span = vector.span();

        REQUIRE_EQUAL(block.toByteBuffer(), ByteBuffer({Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}}));
        REQUIRE_EQUAL(editor.toByteBuffer(), ByteBuffer({Byte{5U}, Byte{6U}}));
        REQUIRE_EQUAL(ByteBuffer{block.span()}, block.toByteBuffer());
        REQUIRE_EQUAL(
            ByteBuffer{block.span(ByteIndex{1U}, ByteLength{8U})}, ByteBuffer({Byte{2U}, Byte{3U}, Byte{4U}}));
        auto visited = ByteBuffer{};
        static_cast<void>(block.forEach([&visited](const Byte byte, const ByteIndex) { visited.append(byte); }));
        REQUIRE_EQUAL(visited, block.toByteBuffer());
        const auto prefix = ByteBlock::fromSpan(fixed.span().first<2>());
        const auto suffix = ByteBlock{ByteArray{Byte{3U}, Byte{4U}}};
        const auto searched = ByteBlock::fromSpan(span);
        REQUIRE(block.startsWith(prefix));
        REQUIRE(block.endsWith(suffix));
        REQUIRE(block.contains(searched));
        REQUIRE_EQUAL(block.find(searched), ByteIndex{1U});
        REQUIRE_EQUAL(block.find(searched, ByteIndex{2U}), ByteIndex::noIndex());
        REQUIRE_EQUAL(block.findLast(searched), ByteIndex{1U});

        const auto fixedNeedleStorage = ByteArray{Byte{2U}, Byte{3U}};
        const auto fixedNeedle = fixedNeedleStorage.span();
        const auto dynamicNeedle = ConstByteSpan{fixedNeedle};
        REQUIRE(block.startsWith(fixed.span().first<2>()));
        REQUIRE(block.endsWith(fixed.span().last<2>()));
        REQUIRE(block.contains(dynamicNeedle));
        REQUIRE_EQUAL(block.find(fixedNeedle), ByteIndex{1U});
        REQUIRE_EQUAL(block.find(dynamicNeedle, ByteIndex{2U}), ByteIndex::noIndex());
        REQUIRE_EQUAL(block.findLast(fixedNeedle), ByteIndex{1U});
    }

    void testMemoryManagementAndCow() {
        auto editor = makeEditor({1U, 2U, 3U});
        editor.reserve(ByteLength{10U});
        const auto reservedCapacity = el::mem::impl::bestGrowthCapacity<el::mem::impl::ByteBlockData>(3U, 10U);
        REQUIRE_EQUAL(editor.capacity(), ByteLength::fromSizeT(reservedCapacity));
        editor.resize(ByteLength{5U});
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U, 0U, 0U}));
        editor.resize(ByteLength{3U});

        auto copy = editor;
        editor.set(ByteIndex{0U}, Byte{9U});
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({9U, 2U, 3U}));
        REQUIRE_EQUAL(copy.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));

        editor.clear();
        REQUIRE(editor.isEmpty());
        REQUIRE_EQUAL(editor.capacity(), ByteLength::fromSizeT(reservedCapacity));

        editor.reset();
        REQUIRE(editor.isEmpty());
        REQUIRE(editor.capacity().isZero());

        copy.shrinkToFit();
        REQUIRE_EQUAL(copy.capacity(), copy.length());
    }

    void testSensitivityMarkAndAliases() {
        auto editor = makeEditor({1U, 2U, 3U, 4U});
        const auto block = ByteBlock{editor};
        auto slice = block.slice(ByteIndex{1U}, ByteLength{2U});
        REQUIRE_FALSE(editor.isSensitive());
        REQUIRE_FALSE(block.isSensitive());

        slice.markAsSensitive();
        REQUIRE(editor.isSensitive());
        REQUIRE(block.isSensitive());
        REQUIRE(slice.isSensitive());

        editor.set(ByteIndex{0U}, Byte{9U});
        REQUIRE(editor.isSensitive());
        REQUIRE(block.isSensitive());
        editor.reserve(ByteLength{64U});
        REQUIRE(editor.isSensitive());
        editor.shrinkToFit();
        REQUIRE(editor.isSensitive());
        editor.secureErase();
        REQUIRE(editor.isSensitive());

        auto emptyBlock = ByteBlock{};
        auto emptyEditor = ByteBlockEditor{};
        emptyBlock.markAsSensitive();
        emptyEditor.markAsSensitive();
        REQUIRE_FALSE(emptyBlock.isSensitive());
        REQUIRE_FALSE(emptyEditor.isSensitive());
        emptyEditor.reserve(ByteLength{8U});
        emptyEditor.markAsSensitive();
        REQUIRE(emptyEditor.isSensitive());
        emptyEditor.clear();
        REQUIRE(emptyEditor.isSensitive());
        emptyEditor.reset();
        REQUIRE_FALSE(emptyEditor.isSensitive());
    }

    void testSensitivityPropagation() {
        auto sensitive = makeBlock({7U, 8U});
        sensitive.markAsSensitive();

        auto copied = ByteBlockEditor{sensitive};
        REQUIRE(copied.isSensitive());

        auto appended = makeEditor({1U});
        appended.append(sensitive);
        REQUIRE(appended.isSensitive());

        auto inserted = makeEditor({1U});
        inserted.insert(ByteIndex{}, sensitive);
        REQUIRE(inserted.isSensitive());

        auto replaced = makeEditor({1U, 2U});
        replaced.replace(ByteRange::all(), sensitive);
        REQUIRE(replaced.isSensitive());

        auto overwritten = makeEditor({1U, 2U});
        overwritten.overwrite(ByteRange::all(), sensitive);
        REQUIRE(overwritten.isSensitive());

        auto xored = makeEditor({1U, 2U});
        REQUIRE(xored.xorWith(sensitive));
        REQUIRE(xored.isSensitive());

        const auto joined = makeEditor({0U}).join({makeBlock({1U}), sensitive});
        REQUIRE(joined.isSensitive());
        const auto concatenated = ByteBlockEditor::fromJoined({makeBlock({1U}), sensitive});
        REQUIRE(concatenated.isSensitive());

        auto rawDestination = makeEditor({1U, 2U});
        rawDestination.append(sensitive.span());
        rawDestination.insert(ByteIndex{}, sensitive.span());
        rawDestination.replace(ByteRange::all(), sensitive.span());
        rawDestination.overwrite(ByteRange::all(), sensitive.span());
        REQUIRE(rawDestination.xorWith(sensitive.span()));
        REQUIRE_FALSE(rawDestination.isSensitive());
        REQUIRE_FALSE(ByteBlock::fromSpan(sensitive.span()).isSensitive());
        REQUIRE_FALSE(ByteBlockEditor::fromSpan(sensitive.span()).isSensitive());
        REQUIRE_FALSE(ByteBlock::fromVector(sensitive.toUInt8Vector()).isSensitive());
    }

    void testRejectsSizesBeyondSharedByteLimit() {
        if constexpr (sizeof(std::size_t) > sizeof(std::uint32_t)) {
            const auto tooLarge = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1U;
            REQUIRE_THROWS_AS(std::length_error, ByteBlockEditor{ByteLength::fromSizeT(tooLarge)});
            auto editor = ByteBlockEditor{};
            REQUIRE_THROWS_AS(std::length_error, editor.reserve(ByteLength::fromSizeT(tooLarge)));
        }
    }

    void testSensitiveAllocationErasure() {
        const auto guard = ByteBlockEraseObserverGuard{};
        const auto expectedCapacity =
            el::mem::impl::bestGrowthCapacity<el::mem::impl::ByteBlockData>(4U, std::size_t{128U});
        {
            auto editor = makeEditor({1U, 2U, 3U, 4U});
            editor.markAsSensitive();
            editor.resize(ByteLength{2U});
            editor.clear();
            REQUIRE(gByteBlockEraseEvents.empty());
            editor.resize(ByteLength{4U});
            editor.reserve(ByteLength{128U});
            REQUIRE_EQUAL(gByteBlockEraseEvents.size(), std::size_t{1U});
            REQUIRE(gByteBlockEraseEvents.front().isZero);
            gByteBlockEraseEvents.clear();
            REQUIRE_EQUAL(editor.capacity(), ByteLength::fromSizeT(expectedCapacity));
        }
        REQUIRE_EQUAL(gByteBlockEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(
            gByteBlockEraseEvents.front().size,
            el::mem::impl::ByteBlockData::allocationSizeForCapacity(expectedCapacity));
        REQUIRE(gByteBlockEraseEvents.front().isZero);
    }

    void testGetSetAndThrowingAccess() {
        auto editor = makeEditor({1U, 2U, 3U});

        REQUIRE_EQUAL(editor.get(ByteIndex{1U}), Byte{2U});
        REQUIRE_EQUAL(editor.get(ByteIndex{9U}, Byte{7U}), Byte{7U});
        REQUIRE_EQUAL(editor.get(ByteIndex::noIndex(), Byte{8U}), Byte{8U});
        REQUIRE_THROWS(editor.getOrThrow(ByteIndex{9U}));

        editor.set(ByteIndex{1U}, Byte{5U});
        editor.xorAt(ByteIndex{1U}, Byte{0x0fU});
        editor.xorAtOrThrow(ByteIndex{2U}, Byte{0xffU});
        editor.set(ByteIndex{9U}, Byte{6U});
        editor.xorAt(ByteIndex{9U}, Byte{0xffU});
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 0x0aU, 0xfcU}));
        REQUIRE_THROWS(editor.setOrThrow(ByteIndex::noIndex(), Byte{1U}));
        REQUIRE_THROWS(editor.xorAtOrThrow(ByteIndex::noIndex(), Byte{1U}));
    }

    void testIntegerAccess() {
        auto editor = ByteBlockEditor{ByteLength{12U}};

        REQUIRE(editor.setInteger(ByteIndex{0U}, uint32_t{0x12345678U}));
        editor.setIntegerOrThrow(ByteIndex{4U}, int32_t{-0x1234567}, Endianness::Big);
        REQUIRE(editor.setInteger(ByteIndex{8U}, uint32_t{0xaabbccddU}, Endianness::Big));
        REQUIRE_EQUAL(editor.getInteger<uint32_t>(ByteIndex{0U}), uint32_t{0x12345678U});
        REQUIRE_EQUAL(editor.getIntegerOrThrow<int32_t>(ByteIndex{4U}, Endianness::Big), int32_t{-0x1234567});
        REQUIRE_EQUAL(editor.getInteger<uint32_t>(ByteIndex{8U}, Endianness::Big), uint32_t{0xaabbccddU});
        REQUIRE_EQUAL(
            editor.getInteger<uint64_t>(ByteIndex{8U}, Endianness::Little, uint64_t{0xfeedbeefcafebabeU}),
            uint64_t{0xfeedbeefcafebabeU});
        const auto immutable = ByteBlock{editor};
        REQUIRE_EQUAL(immutable.getInteger<uint32_t>(ByteIndex{0U}), uint32_t{0x12345678U});
        REQUIRE_EQUAL(immutable.getIntegerOrThrow<int32_t>(ByteIndex{4U}, Endianness::Big), int32_t{-0x1234567});
        auto immutableOutput = uint32_t{};
        REQUIRE(immutable.getIntegerInto(immutableOutput, ByteIndex{8U}, Endianness::Big));
        REQUIRE_EQUAL(immutableOutput, uint32_t{0xaabbccddU});

        auto output = int32_t{77};
        REQUIRE(editor.getIntegerInto(output, ByteIndex{4U}, Endianness::Big));
        REQUIRE_EQUAL(output, int32_t{-0x1234567});
        REQUIRE_FALSE(editor.getIntegerInto(output, ByteIndex{10U}));
        REQUIRE_EQUAL(output, int32_t{-0x1234567});
        REQUIRE_FALSE(editor.getIntegerInto(output, ByteIndex::noIndex()));
        REQUIRE_EQUAL(output, int32_t{-0x1234567});

        const auto sharedSnapshot = ByteBlock{editor};
        const auto before = editor.toByteBuffer();
        REQUIRE_FALSE(editor.setInteger(ByteIndex{11U}, uint16_t{0xffffU}));
        REQUIRE_FALSE(editor.setInteger(ByteIndex::noIndex(), uint16_t{0xffffU}));
        REQUIRE_EQUAL(editor.toByteBuffer(), before);
        editor.set(ByteIndex{0U}, Byte{0U});
        REQUIRE_EQUAL(sharedSnapshot.toByteBuffer(), before);

        REQUIRE_THROWS_AS(el::err::OutOfRangeError, editor.getIntegerOrThrow<uint32_t>(ByteIndex{10U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, editor.getIntegerOrThrow<uint32_t>(ByteIndex::noIndex()));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, editor.setIntegerOrThrow(ByteIndex{12U}, uint16_t{1U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, editor.setIntegerOrThrow(ByteIndex::noIndex(), uint16_t{1U}));
    }

    void testSliceAndFind() {
        const auto block = makeBlock({1U, 2U, 3U, 2U, 3U, 4U});
        const auto needle = makeBlock({2U, 3U});
        const auto empty = ByteBlock{};

        REQUIRE_EQUAL(
            block.slice(ByteRange{ByteIndex{1U}, ByteLength{3U}}).toUInt8Vector(), std::vector<uint8_t>({2U, 3U, 2U}));
        REQUIRE(block.slice(ByteRange::noRange()).isEmpty());
        REQUIRE(block.startsWith(makeBlock({1U, 2U})));
        REQUIRE(block.endsWith(makeBlock({3U, 4U})));
        REQUIRE(block.contains(needle));
        REQUIRE(block.contains(empty));
        REQUIRE_EQUAL(block.find(needle), ByteIndex{1U});
        REQUIRE_EQUAL(block.find(needle, ByteIndex{2U}), ByteIndex{3U});
        REQUIRE_EQUAL(block.findLast(needle), ByteIndex{3U});
        REQUIRE_EQUAL(block.find(empty), ByteIndex::zero());
        REQUIRE_EQUAL(block.findLast(empty), block.endIndex());
        REQUIRE(block.find(makeBlock({9U})).isNoIndex());
    }

    void testModification() {
        auto editor = makeEditor({1U, 2U, 3U, 4U});

        editor.remove(ByteRange{ByteIndex{1U}, ByteLength{2U}});
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 4U}));

        editor.insert(ByteIndex{1U}, makeBlock({7U, 8U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 7U, 8U, 4U}));

        editor.replace(ByteRange{ByteIndex{1U}, ByteLength{2U}}, makeBlock({5U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 5U, 4U}));

        editor.replace(ByteRange::noRange(), makeBlock({6U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 5U, 4U}));

        editor.append(Byte{9U}).append(makeBlock({10U, 11U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 5U, 4U, 9U, 10U, 11U}));

        editor.keep(ByteRange{ByteIndex{1U}, ByteLength{3U}});
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({5U, 4U, 9U}));

        editor.keep(ByteRange::noRange());
        REQUIRE(editor.isEmpty());

        const auto fixedSource = ByteArray{Byte{7U}, Byte{8U}};
        const auto dynamicSourceStorage = ByteArray{Byte{9U}, Byte{10U}, Byte{11U}};
        const auto dynamicSource = ConstByteSpan{dynamicSourceStorage.span()};
        editor.append(fixedSource.span());
        editor.insert(ByteIndex{1U}, dynamicSource);
        editor.replace(ByteRange{ByteIndex{0U}, ByteLength{2U}}, fixedSource.span());
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({7U, 8U, 10U, 11U, 8U}));

        auto aliasing = ByteBlockEditor({1U, 2U, 3U});
        aliasing.shrinkToFit();
        const auto aliasedSpan = el::mem::impl::UnsafeByteBlockAccess{aliasing}.data().subspan(1U);
        aliasing.append(aliasedSpan);
        REQUIRE_EQUAL(aliasing.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U, 2U, 3U}));
    }

    void testAppendOverwriteAndXor() {
        auto editor = makeEditor({0U, 1U, 2U, 3U, 4U, 5U});
        const auto firstSource = ByteArray{Byte{9U}, Byte{8U}};
        const auto firstRange = ByteRange{ByteIndex{1U}, ByteLength{2U}};
        editor.overwrite(firstRange, ByteBlock{firstSource});
        editor.overwrite({ByteIndex{3U}, ByteLength{2U}}, makeBlock({7U, 6U}));
        editor.overwrite({ByteIndex{5U}, ByteIndex{6U}}, ByteBlock{ByteArray{Byte{4U}}});
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({0U, 9U, 8U, 7U, 6U, 4U}));

        const auto aliased = ByteBlock{editor}.slice(ByteIndex{1U}, ByteLength{3U});
        editor.overwrite({ByteIndex{0U}, ByteLength{3U}}, aliased);
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({9U, 8U, 7U, 7U, 6U, 4U}));

        const auto shared = ByteBlock{editor};
        const auto sharedData = el::mem::impl::UnsafeByteBlockAccess{shared}.data().data();
        editor.overwrite(ByteRange::noRange(), ByteArray{Byte{1U}, Byte{2U}}.span());
        REQUIRE(el::mem::impl::UnsafeByteBlockAccess{editor}.data().data() == sharedData);
        editor.overwrite(ByteRange::all(), ConstByteSpan{});
        REQUIRE(el::mem::impl::UnsafeByteBlockAccess{editor}.data().data() == sharedData);
        editor.fill(ByteRange::noRange(), Byte{0xffU});
        REQUIRE(el::mem::impl::UnsafeByteBlockAccess{editor}.data().data() == sharedData);
        editor.xorWith(ByteRange::noRange(), ByteArray{Byte{0xffU}}.span());
        REQUIRE(el::mem::impl::UnsafeByteBlockAccess{editor}.data().data() == sharedData);
        editor.overwrite({ByteIndex{5U}, ByteLength{2U}}, ByteArray{Byte{1U}, Byte{2U}}.span());
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({9U, 8U, 7U, 7U, 6U, 1U}));
        editor.overwrite({ByteIndex{0U}, ByteLength{2U}}, ByteArray{Byte{2U}}.span());
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({2U, 8U, 7U, 7U, 6U, 1U}));
        editor.fill({ByteIndex{1U}, ByteLength{2U}}, Byte{0xaaU});
        editor.xorWith({ByteIndex{1U}, ByteLength{2U}}, ByteArray{Byte{0xffU}, Byte{0x0fU}}.span());
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({2U, 0x55U, 0xa5U, 7U, 6U, 1U}));

        editor.append(ByteBlock{ByteArray{Byte{0xaaU}, Byte{0xbbU}}})
            .appendInteger(uint16_t{0x1234U})
            .appendInteger(uint16_t{0x5678U}, Endianness::Big);
        REQUIRE_EQUAL(
            editor.toUInt8Vector(),
            std::vector<uint8_t>({2U, 0x55U, 0xa5U, 7U, 6U, 1U, 0xaaU, 0xbbU, 0x34U, 0x12U, 0x56U, 0x78U}));
        REQUIRE_EQUAL(shared.toUInt8Vector(), std::vector<uint8_t>({9U, 8U, 7U, 7U, 6U, 4U}));

        auto xorEditor = makeEditor({0xf0U, 0x0fU});
        const auto xorSnapshot = ByteBlock{xorEditor};
        REQUIRE(xorEditor.xorWith(makeBlock({0xaaU, 0x55U})));
        REQUIRE_EQUAL(xorEditor.toUInt8Vector(), std::vector<uint8_t>({0x5aU, 0x5aU}));
        REQUIRE_EQUAL(xorSnapshot.toUInt8Vector(), std::vector<uint8_t>({0xf0U, 0x0fU}));
        const auto xorBeforeFailure = xorEditor.toUInt8Vector();
        REQUIRE_FALSE(xorEditor.xorWith(makeBlock({1U})));
        REQUIRE_EQUAL(xorEditor.toUInt8Vector(), xorBeforeFailure);
        REQUIRE_THROWS_AS(el::err::ParameterError, xorEditor.xorWithOrThrow(makeBlock({1U})));
        REQUIRE_EQUAL(xorEditor.toUInt8Vector(), xorBeforeFailure);
    }

    void testCopyVariantsJoinAndComparison() {
        const auto editor = makeEditor({1U, 2U, 3U, 4U});
        const auto removed = editor.removed(ByteRange{ByteIndex{1U}, ByteLength{2U}});
        const auto replaced = editor.replaced(ByteRange{ByteIndex{1U}, ByteLength{2U}}, makeBlock({9U}));

        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U, 4U}));
        REQUIRE_EQUAL(removed.toUInt8Vector(), std::vector<uint8_t>({1U, 4U}));
        REQUIRE_EQUAL(replaced.toUInt8Vector(), std::vector<uint8_t>({1U, 9U, 4U}));

        const auto separator = makeEditor({0U});
        const auto joined = separator.join({makeBlock({1U}), makeBlock({2U}), makeBlock({3U})});
        REQUIRE_EQUAL(joined.toUInt8Vector(), std::vector<uint8_t>({1U, 0U, 2U, 0U, 3U}));

        const auto concatenated = ByteBlockEditor::fromJoined({makeBlock({1U}), makeBlock({2U, 3U})});
        REQUIRE_EQUAL(concatenated.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));

        REQUIRE(makeBlock({1U, 2U}) < makeBlock({1U, 3U}));
        REQUIRE(makeEditor({1U, 2U}) == makeBlock({1U, 2U}));
    }

    void testEditorToReadOnlySnapshot() {
        auto editor = makeEditor({1U, 2U, 3U});
        const auto block = ByteBlock{editor};

        editor.set(ByteIndex{0U}, Byte{9U});

        REQUIRE_EQUAL(block.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({9U, 2U, 3U}));
    }

    void testEditorCopiesVisibleSlice() {
        const auto block = makeBlock({1U, 2U, 3U, 4U});
        const auto slice = block.slice(ByteIndex{1U}, ByteLength{2U});
        auto editor = ByteBlockEditor{slice};

        editor.set(ByteIndex{0U}, Byte{9U});

        REQUIRE_EQUAL(block.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U, 4U}));
        REQUIRE_EQUAL(slice.toUInt8Vector(), std::vector<uint8_t>({2U, 3U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({9U, 3U}));
    }

    void testUnsafeAccess() {
        auto editor = makeEditor({1U, 2U, 3U, 4U});
        const auto &reader = editor;
        const auto readRange = el::mem::impl::UnsafeByteBlockAccess{reader}.data<2>(ByteIndex{1U});
        static_assert(decltype(readRange)::extent == 2U);
        REQUIRE_EQUAL(readRange[0], Byte{2U});
        REQUIRE_EQUAL(readRange[1], Byte{3U});

        const auto snapshot = ByteBlock{editor};
        const auto sharedData = el::mem::impl::UnsafeByteBlockAccess{editor}.data().data();
        auto access = el::mem::impl::UnsafeByteBlockAccess{editor};
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, access.writableData<2>(ByteIndex{3U}));
        REQUIRE(el::mem::impl::UnsafeByteBlockAccess{editor}.data().data() == sharedData);

        auto writableRange = access.writableData<2>(ByteIndex{1U});
        writableRange[0] = Byte{9U};
        writableRange[1] = Byte{8U};
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({1U, 9U, 8U, 4U}));
        REQUIRE_EQUAL(snapshot.toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U, 4U}));

        const auto block = ByteBlock{editor};
        const auto blockAccess = el::mem::impl::UnsafeByteBlockAccess{block};
        REQUIRE(blockAccess.data<0>(block.endIndex()).empty());
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, blockAccess.data<2>(ByteIndex{3U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, blockAccess.data<1>(ByteIndex::noIndex()));
    }

    void testSecureErase() {
        const auto guard = ByteBlockEraseObserverGuard{};

        auto uniqueEditor = makeEditor({1U, 2U, 3U, 4U});
        uniqueEditor.reserve(ByteLength{32U});
        const auto uniqueCapacity = uniqueEditor.capacity();
        auto uniqueBlock = ByteBlock{uniqueEditor};
        uniqueEditor.reset();
        gByteBlockEraseEvents.clear();
        uniqueBlock.secureErase();
        REQUIRE_EQUAL(uniqueBlock.length(), ByteLength{4U});
        REQUIRE_EQUAL(uniqueBlock.toUInt8Vector(), std::vector<uint8_t>({0U, 0U, 0U, 0U}));
        REQUIRE_EQUAL(gByteBlockEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(gByteBlockEraseEvents.front().size, uniqueCapacity.toSizeT());
        REQUIRE(gByteBlockEraseEvents.front().isZero);

        auto sharedBlock = makeBlock({5U, 6U, 7U, 8U});
        const auto sharedAlias = sharedBlock;
        const auto sharedSlice = sharedBlock.slice(ByteIndex{1U}, ByteLength{2U});
        gByteBlockEraseEvents.clear();
        sharedBlock.secureErase();
        REQUIRE_EQUAL(sharedBlock.toUInt8Vector(), std::vector<uint8_t>({0U, 0U, 0U, 0U}));
        REQUIRE_EQUAL(sharedAlias.toUInt8Vector(), std::vector<uint8_t>({5U, 6U, 7U, 8U}));
        REQUIRE_EQUAL(sharedSlice.toUInt8Vector(), std::vector<uint8_t>({6U, 7U}));
        REQUIRE(gByteBlockEraseEvents.empty());

        auto editor = makeEditor({9U, 10U, 11U});
        editor.reserve(ByteLength{24U});
        const auto capacity = editor.capacity();
        gByteBlockEraseEvents.clear();
        editor.secureErase();
        REQUIRE_EQUAL(editor.length(), ByteLength{3U});
        REQUIRE_EQUAL(editor.capacity(), capacity);
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({0U, 0U, 0U}));
        REQUIRE_EQUAL(gByteBlockEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(gByteBlockEraseEvents.front().size, capacity.toSizeT());
        REQUIRE(gByteBlockEraseEvents.front().isZero);

        auto sharedEditor = makeEditor({12U, 13U, 14U});
        sharedEditor.reserve(ByteLength{20U});
        const auto sharedCapacity = sharedEditor.capacity();
        const auto editorAlias = ByteBlock{sharedEditor};
        gByteBlockEraseEvents.clear();
        sharedEditor.secureErase();
        REQUIRE_EQUAL(sharedEditor.length(), ByteLength{3U});
        REQUIRE_EQUAL(sharedEditor.capacity(), sharedCapacity);
        REQUIRE_EQUAL(sharedEditor.toUInt8Vector(), std::vector<uint8_t>({0U, 0U, 0U}));
        REQUIRE_EQUAL(editorAlias.toUInt8Vector(), std::vector<uint8_t>({12U, 13U, 14U}));
        REQUIRE(gByteBlockEraseEvents.empty());

        auto emptyBlock = ByteBlock{};
        auto emptyEditor = ByteBlockEditor{};
        emptyEditor.reserve(ByteLength{16U});
        const auto emptyCapacity = emptyEditor.capacity();
        emptyBlock.secureErase();
        emptyEditor.secureErase();
        REQUIRE_EQUAL(emptyEditor.capacity(), emptyCapacity);
        REQUIRE_EQUAL(gByteBlockEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(gByteBlockEraseEvents.front().size, emptyCapacity.toSizeT());
        REQUIRE(gByteBlockEraseEvents.front().isZero);
    }

private:
    [[nodiscard]] static auto makeBlock(std::initializer_list<uint8_t> bytes) -> ByteBlock {
        return ByteBlock::fromVector(std::vector<uint8_t>{bytes});
    }
    [[nodiscard]] static auto makeEditor(std::initializer_list<uint8_t> bytes) -> ByteBlockEditor {
        return ByteBlockEditor::fromVector(std::vector<uint8_t>{bytes});
    }
};
