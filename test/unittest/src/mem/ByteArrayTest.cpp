// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/mem/ByteArray.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/mem/ByteIntegerAccess.hpp>
#include <erbsland/mem/ByteSpan.hpp>
#include <erbsland/mem/Endianness.hpp>
#include <erbsland/mem/impl/UnsafeByteArrayAccess.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

using el::mem::Byte;
using el::mem::ByteArray;
using el::mem::ByteBuffer;
using el::mem::ByteSpan;
using el::mem::ConstByteSpan;
using el::mem::Endianness;
using el::mem::FixedByteSpan;
using el::mem::FixedConstByteSpan;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;

static_assert(std::same_as<decltype(ByteArray{Byte{1U}, Byte{2U}}), ByteArray<2>>);
static_assert(ByteArray<4>{0xf0U, std::byte{0x28U}, uint8_t{0x8cU}, Byte{0x28U}}.get(ByteIndex{2U}) == Byte{0x8cU});
static_assert(std::same_as<decltype(ByteArray<3>{}.span()), FixedConstByteSpan<3>>);
static_assert(std::same_as<decltype(std::declval<const ByteArray<3> &>().span()), FixedConstByteSpan<3>>);
static_assert(ByteArray<0>::isEmpty());
static_assert(ByteArray<4>::length() == ByteLength{4U});
static_assert(ByteArray<4>::endIndex() == ByteIndex{4U});
static_assert(ByteArray{Byte{0x12U}, Byte{0x34U}}.shiftedLeft(4U).get(ByteIndex{0U}).toUInt8() == 0x23U);
static_assert([]() constexpr {
    auto bytes = ByteArray<8>{};
    bytes.set(ByteIndex{0U}, Byte{0x7fU});
    if (bytes.get(ByteIndex{0U}) != Byte{0x7fU} || bytes.get(ByteIndex::noIndex(), Byte{9U}) != Byte{9U}) {
        return false;
    }
    if (!bytes.setInteger(ByteIndex{1U}, int16_t{-2}) ||
        !bytes.setInteger(ByteIndex{3U}, uint32_t{0x12345678U}, Endianness::Big)) {
        return false;
    }
    auto value = uint32_t{};
    return bytes.getInteger<int16_t>(ByteIndex{1U}) == int16_t{-2} &&
        bytes.getIntegerInto(value, ByteIndex{3U}, Endianness::Big) && value == uint32_t{0x12345678U} &&
        bytes.span(ByteIndex{6U}, ByteLength::infinite()).size() == 2U;
}());
static_assert([]() constexpr {
    auto bytes = std::array<Byte, 8>{};
    const auto span = ByteSpan{bytes};
    if (!el::mem::setInteger(span, ByteIndex{0U}, uint32_t{0x12345678U}) ||
        !el::mem::setInteger(span, ByteIndex{4U}, int32_t{-2}, Endianness::Big)) {
        return false;
    }
    const auto constSpan = ConstByteSpan{bytes};
    auto value = int32_t{};
    return el::mem::getInteger<uint32_t>(constSpan, ByteIndex{0U}) == uint32_t{0x12345678U} &&
        el::mem::getIntegerInto(constSpan, value, ByteIndex{4U}, Endianness::Big) && value == int32_t{-2};
}());

TESTED_TARGETS(ByteArray ByteSpan ConstByteSpan FixedByteSpan FixedConstByteSpan UnsafeByteArrayAccess)
class ByteArrayTest final : public el::UnitTest {
public:
    void testConstructionAndAccess() {
        auto bytes = ByteArray{Byte{1U}, Byte{2U}, Byte{3U}};
        const auto &constBytes = bytes;

        REQUIRE_FALSE(bytes.isEmpty());
        REQUIRE_EQUAL(bytes.length(), ByteLength{3U});
        REQUIRE_EQUAL(bytes.get(ByteIndex{1U}), Byte{2U});
        REQUIRE_EQUAL(constBytes.get(ByteIndex{2U}), Byte{3U});
        REQUIRE_EQUAL(bytes.get(ByteIndex{99U}, Byte{8U}), Byte{8U});
        REQUIRE_EQUAL(bytes.get(ByteIndex::noIndex(), Byte{7U}), Byte{7U});
        REQUIRE_EQUAL(bytes.getOrThrow(ByteIndex{0U}), Byte{1U});
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, bytes.getOrThrow(ByteIndex{3U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, bytes.getOrThrow(ByteIndex::noIndex()));

        bytes.set(ByteIndex{1U}, Byte{7U});
        bytes.setOrThrow(ByteIndex{0U}, Byte{6U});
        REQUIRE_EQUAL(bytes.toByteBuffer(), ByteBuffer({Byte{6U}, Byte{7U}, Byte{3U}}));
        bytes.xorAt(ByteIndex{1U}, Byte{0x0fU});
        bytes.xorAt(ByteIndex::noIndex(), Byte{0xffU});
        bytes.xorAtOrThrow(ByteIndex{0U}, Byte{0x03U});
        REQUIRE_EQUAL(bytes.toByteBuffer(), ByteBuffer({Byte{5U}, Byte{8U}, Byte{3U}}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, bytes.xorAtOrThrow(ByteIndex{3U}, Byte{1U}));

        auto sum = uint8_t{0U};
        static_cast<void>(bytes.forEach([&sum](const Byte byte) { sum = static_cast<uint8_t>(sum + byte.toUInt8()); }));
        REQUIRE_EQUAL(sum, uint8_t{16U});

        bytes.fill(Byte{0xaaU});
        REQUIRE_EQUAL(bytes.toByteBuffer(), ByteBuffer({Byte{0xaaU}, Byte{0xaaU}, Byte{0xaaU}}));
    }

    void testEmptyArray() {
        auto empty = ByteArray<0>{};

        REQUIRE(empty.isEmpty());
        REQUIRE(empty.span().empty());
        REQUIRE(empty.toByteBuffer().isEmpty());
        REQUIRE_EQUAL(empty.shiftedLeft(1U), empty);
        REQUIRE_EQUAL(empty.rotatedRight(-17), empty);
    }

    void testUnsafeWritableAccess() {
        auto bytes = ByteArray{Byte{1U}, Byte{2U}, Byte{3U}};
        auto access = el::mem::impl::UnsafeByteArrayAccess{bytes};
        auto writable = access.writableData();

        static_assert(decltype(writable)::extent == 3U);
        writable[1U] = Byte{9U};
        REQUIRE_EQUAL(bytes.toByteBuffer(), ByteBuffer({Byte{1U}, Byte{9U}, Byte{3U}}));
    }

    void testSpanAliases() {
        auto bytes = ByteArray{Byte{1U}, Byte{2U}, Byte{3U}};
        const auto fixed = bytes.span();
        const auto dynamic = ConstByteSpan{fixed};
        const auto &constBytes = bytes;
        const auto constFixed = constBytes.span();
        const auto constDynamic = ConstByteSpan{constFixed};

        static_assert(decltype(fixed)::extent == 3U);
        static_assert(decltype(constFixed)::extent == 3U);
        static_assert(ByteSpan::extent == std::dynamic_extent);
        static_assert(ConstByteSpan::extent == std::dynamic_extent);

        bytes.set(ByteIndex{1U}, Byte{9U});
        REQUIRE_EQUAL(dynamic[1], Byte{9U});
        REQUIRE_EQUAL(constDynamic[1], Byte{9U});
    }

    void testStandardSpanCompatibility() {
        auto standardBytes = std::array<std::byte, 2>{std::byte{1U}, std::byte{2U}};
        auto unsignedBytes = std::array<uint8_t, 2>{3U, 4U};
        auto characterBytes = std::array<char, 2>{'\x05', '\x06'};

        auto standardSpan = el::mem::toByteSpan(std::span{standardBytes});
        auto unsignedSpan = el::mem::toByteSpan(std::span{unsignedBytes});
        auto characterSpan = el::mem::toByteSpan(std::span{characterBytes});
        standardSpan[0] = Byte{7U};
        unsignedSpan[0] = Byte{8U};
        characterSpan[0] = Byte{9U};
        REQUIRE_EQUAL(standardBytes[0], std::byte{7U});
        REQUIRE_EQUAL(unsignedBytes[0], uint8_t{8U});
        REQUIRE_EQUAL(static_cast<unsigned char>(characterBytes[0]), static_cast<unsigned char>(9U));

        const auto &constStandardBytes = standardBytes;
        const auto &constUnsignedBytes = unsignedBytes;
        const auto &constCharacterBytes = characterBytes;
        REQUIRE_EQUAL(el::mem::toConstByteSpan(std::span{constStandardBytes})[1], Byte{2U});
        REQUIRE_EQUAL(el::mem::toConstByteSpan(std::span{constUnsignedBytes})[1], Byte{4U});
        REQUIRE_EQUAL(el::mem::toConstByteSpan(std::span{constCharacterBytes})[1], Byte{6U});
    }

    void testUnitAwareSpans() {
        auto bytes = ByteArray{Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}, Byte{5U}};

        const auto readOnlyRange = bytes.span(ByteRange{ByteIndex{1U}, ByteLength{2U}});
        REQUIRE_EQUAL(readOnlyRange.size(), std::size_t{2U});
        bytes.set(ByteIndex{1U}, Byte{9U});
        REQUIRE_EQUAL(bytes.get(ByteIndex{1U}), Byte{9U});

        const auto &constBytes = bytes;
        const auto constRange = constBytes.span(ByteIndex{2U}, ByteLength{2U});
        REQUIRE_EQUAL(constRange.size(), std::size_t{2U});
        REQUIRE_EQUAL(constRange[0], Byte{3U});
        REQUIRE_EQUAL(constRange[1], Byte{4U});

        const auto clamped = constBytes.span(ByteRange{ByteIndex{3U}, ByteLength{99U}});
        REQUIRE_EQUAL(clamped.size(), std::size_t{2U});
        REQUIRE_EQUAL(clamped[1], Byte{5U});
        REQUIRE_EQUAL(constBytes.span(ByteRange{ByteIndex{2U}, ByteLength::infinite()}).size(), std::size_t{3U});
        REQUIRE(constBytes.span(ByteRange{ByteIndex{99U}, ByteLength{2U}}).empty());
        REQUIRE(constBytes.span(ByteRange{ByteIndex::noIndex(), ByteLength{2U}}).empty());
        REQUIRE(constBytes.span(ByteRange::noRange()).empty());
        REQUIRE(constBytes.span(ByteIndex::noIndex(), ByteLength{2U}).empty());
    }

    void testBulkMutation() {
        auto bytes = ByteArray{Byte{0U}, Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}, Byte{5U}};
        bytes.fill(ByteRange{ByteIndex{1U}, ByteLength{2U}}, Byte{9U});
        bytes.overwrite(ByteRange{ByteIndex{3U}, ByteLength::infinite()}, ByteArray{Byte{8U}, Byte{7U}}.span());
        REQUIRE_EQUAL(bytes.toByteBuffer(), ByteBuffer({Byte{0U}, Byte{9U}, Byte{9U}, Byte{8U}, Byte{7U}, Byte{5U}}));

        bytes.overwrite(ByteIndex{99U}, ByteArray{Byte{1U}}.span());
        bytes.overwrite(ByteRange::noRange(), ByteArray{Byte{1U}}.span());
        bytes.overwrite(ByteRange::all(), ConstByteSpan{});
        REQUIRE_EQUAL(bytes.toByteBuffer(), ByteBuffer({Byte{0U}, Byte{9U}, Byte{9U}, Byte{8U}, Byte{7U}, Byte{5U}}));

        auto overlap = ByteArray{Byte{0U}, Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}};
        overlap.overwrite(ByteIndex{1U}, overlap.span(ByteIndex{0U}, ByteLength{4U}));
        REQUIRE_EQUAL(overlap.toByteBuffer(), ByteBuffer({Byte{0U}, Byte{0U}, Byte{1U}, Byte{2U}, Byte{3U}}));
        overlap.overwrite(ByteRange{ByteIndex{0U}, ByteLength{4U}}, overlap.span(ByteIndex{1U}, ByteLength{4U}));
        REQUIRE_EQUAL(overlap.toByteBuffer(), ByteBuffer({Byte{0U}, Byte{1U}, Byte{2U}, Byte{3U}, Byte{3U}}));

        bytes.xorWith(
            ByteRange{ByteIndex{1U}, ByteLength{3U}}, ByteArray{Byte{0xffU}, Byte{0x0fU}, Byte{0xf0U}}.span());
        REQUIRE_EQUAL(
            bytes.toByteBuffer(), ByteBuffer({Byte{0U}, Byte{0xf6U}, Byte{0x06U}, Byte{0xf8U}, Byte{7U}, Byte{5U}}));
        REQUIRE_FALSE(bytes.xorWith(ByteArray{Byte{1U}}.span()));
    }

    void testBitwiseOperators() {
        const auto first = ByteArray{Byte{0xf0U}, Byte{0x0fU}};
        const auto second = ByteArray{Byte{0xaaU}, Byte{0x55U}};

        REQUIRE_EQUAL((first | second).toByteBuffer(), ByteBuffer({Byte{0xfaU}, Byte{0x5fU}}));
        REQUIRE_EQUAL((first & second).toByteBuffer(), ByteBuffer({Byte{0xa0U}, Byte{0x05U}}));
        REQUIRE_EQUAL((first ^ second).toByteBuffer(), ByteBuffer({Byte{0x5aU}, Byte{0x5aU}}));
        REQUIRE_EQUAL((~first).toByteBuffer(), ByteBuffer({Byte{0x0fU}, Byte{0xf0U}}));

        auto value = first;
        value |= second;
        REQUIRE_EQUAL(value, first | second);
        value &= first;
        REQUIRE_EQUAL(value, first);
        value ^= second;
        REQUIRE_EQUAL(value, first ^ second);
    }

    void testWholeArrayShifts() {
        const auto value = ByteArray{Byte{0x12U}, Byte{0x34U}};

        REQUIRE_EQUAL(value.shiftedLeft(4U).toByteBuffer(), ByteBuffer({Byte{0x23U}, Byte{0x40U}}));
        REQUIRE_EQUAL(value.shiftedRight(4U).toByteBuffer(), ByteBuffer({Byte{0x01U}, Byte{0x23U}}));
        REQUIRE_EQUAL(value.shiftedLeft(8U).toByteBuffer(), ByteBuffer({Byte{0x34U}, Byte{0x00U}}));
        REQUIRE_EQUAL(value.shiftedRight(8U).toByteBuffer(), ByteBuffer({Byte{0x00U}, Byte{0x12U}}));
        REQUIRE_EQUAL(value.shiftedLeft(16U), ByteArray<2>{});
        REQUIRE_EQUAL(value.shiftedRight(100U), ByteArray<2>{});

        auto shifted = value;
        shifted <<= 4U;
        REQUIRE_EQUAL(shifted, value << 4U);
        shifted = value;
        shifted >>= 4U;
        REQUIRE_EQUAL(shifted, value >> 4U);
    }

    void testWholeArrayRotations() {
        const auto value = ByteArray{Byte{0x12U}, Byte{0x34U}};

        REQUIRE_EQUAL(value.rotatedLeft(4).toByteBuffer(), ByteBuffer({Byte{0x23U}, Byte{0x41U}}));
        REQUIRE_EQUAL(value.rotatedRight(4).toByteBuffer(), ByteBuffer({Byte{0x41U}, Byte{0x23U}}));
        REQUIRE_EQUAL(value.rotatedLeft(20), value.rotatedLeft(4));
        REQUIRE_EQUAL(value.rotatedLeft(-4), value.rotatedRight(4));
        REQUIRE_EQUAL(value.rotatedRight(-4), value.rotatedLeft(4));

        auto rotated = value;
        rotated.rotateLeft(8);
        REQUIRE_EQUAL(rotated.toByteBuffer(), ByteBuffer({Byte{0x34U}, Byte{0x12U}}));
        rotated.rotateRight(8);
        REQUIRE_EQUAL(rotated, value);
    }

    void testPerByteBitOperations() {
        const auto value = ByteArray{Byte{0x81U}, Byte{0x01U}};

        REQUIRE_EQUAL(value.eachByteShiftedLeft(1U).toByteBuffer(), ByteBuffer({Byte{0x02U}, Byte{0x02U}}));
        REQUIRE_EQUAL(value.eachByteShiftedRight(1U).toByteBuffer(), ByteBuffer({Byte{0x40U}, Byte{0x00U}}));
        REQUIRE_EQUAL(value.eachByteRotatedLeft(1).toByteBuffer(), ByteBuffer({Byte{0x03U}, Byte{0x02U}}));
        REQUIRE_EQUAL(value.eachByteRotatedRight(1).toByteBuffer(), ByteBuffer({Byte{0xc0U}, Byte{0x80U}}));
        const auto carryValue = ByteArray{Byte{0x80U}, Byte{0x80U}};
        REQUIRE_NOT_EQUAL(carryValue.eachByteShiftedLeft(1U), carryValue.shiftedLeft(1U));

        auto changed = value;
        changed.shiftEachByteLeft(1U);
        changed.rotateEachByteRight(1);
        REQUIRE_EQUAL(changed.toByteBuffer(), ByteBuffer({Byte{0x01U}, Byte{0x01U}}));
    }

    void testIntegerAccess() {
        auto bytes = ByteArray<12>{};

        REQUIRE(bytes.setInteger(ByteIndex{1U}, uint16_t{0x1234U}));
        REQUIRE(bytes.setInteger(ByteIndex{3U}, uint16_t{0xabcdU}, Endianness::Big));
        REQUIRE(bytes.setInteger(ByteIndex{5U}, int16_t{-2}));
        REQUIRE_EQUAL(bytes.getInteger<uint16_t>(ByteIndex{1U}), uint16_t{0x1234U});
        REQUIRE_EQUAL(bytes.getInteger<uint16_t>(ByteIndex{3U}, Endianness::Big), uint16_t{0xabcdU});
        REQUIRE_EQUAL(bytes.getInteger<int16_t>(ByteIndex{5U}), int16_t{-2});
        REQUIRE_EQUAL(
            bytes.getInteger<uint32_t>(ByteIndex{10U}, Endianness::Little, 0xfeedbeefU), uint32_t{0xfeedbeefU});

        auto output = uint32_t{0x11223344U};
        REQUIRE_FALSE(bytes.getIntegerInto(output, ByteIndex{10U}));
        REQUIRE_EQUAL(output, uint32_t{0x11223344U});

        const auto before = bytes;
        REQUIRE_FALSE(bytes.setInteger(ByteIndex{11U}, uint16_t{0xffffU}));
        REQUIRE_EQUAL(bytes, before);
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, bytes.getIntegerOrThrow<uint32_t>(ByteIndex{10U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, bytes.setIntegerOrThrow(ByteIndex{12U}, uint16_t{1U}));
    }

    void testUnitAwareIntegerAccess() {
        auto bytes = ByteArray<12>{};

        REQUIRE(bytes.setInteger(ByteIndex{0U}, uint32_t{0x12345678U}));
        bytes.setIntegerOrThrow(ByteIndex{4U}, int32_t{-0x1234567}, Endianness::Big);
        REQUIRE_EQUAL(bytes.getInteger<uint32_t>(ByteIndex{0U}), uint32_t{0x12345678U});
        REQUIRE_EQUAL(bytes.getIntegerOrThrow<int32_t>(ByteIndex{4U}, Endianness::Big), int32_t{-0x1234567});
        REQUIRE_EQUAL(
            bytes.getInteger<uint64_t>(ByteIndex{8U}, Endianness::Little, uint64_t{0xfeedbeefU}),
            uint64_t{0xfeedbeefU});
        REQUIRE_EQUAL(
            bytes.getInteger<uint16_t>(ByteIndex::noIndex(), Endianness::Little, uint16_t{0x55aaU}), uint16_t{0x55aaU});

        auto output = int32_t{77};
        REQUIRE_FALSE(bytes.getIntegerInto(output, ByteIndex{10U}, Endianness::Big));
        REQUIRE_EQUAL(output, int32_t{77});
        REQUIRE_FALSE(bytes.getIntegerInto(output, ByteIndex::noIndex()));
        REQUIRE_EQUAL(output, int32_t{77});

        const auto before = bytes;
        REQUIRE_FALSE(bytes.setInteger(ByteIndex{11U}, uint16_t{0xffffU}));
        REQUIRE_FALSE(bytes.setInteger(ByteIndex::noIndex(), uint16_t{0xffffU}));
        REQUIRE_EQUAL(bytes, before);
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, bytes.getIntegerOrThrow<uint32_t>(ByteIndex{10U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, bytes.getIntegerOrThrow<uint32_t>(ByteIndex::noIndex()));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, bytes.setIntegerOrThrow(ByteIndex{12U}, uint16_t{1U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, bytes.setIntegerOrThrow(ByteIndex::noIndex(), uint16_t{1U}));
    }

    void testSpanIntegerAccess() {
        auto bytes = std::array<Byte, 12>{};
        auto span = ByteSpan{bytes};

        REQUIRE(el::mem::setInteger(span, ByteIndex{0U}, uint32_t{0x12345678U}));
        el::mem::setIntegerOrThrow(span, ByteIndex{4U}, int32_t{-0x1234567}, Endianness::Big);
        const auto constSpan = ConstByteSpan{span};
        REQUIRE_EQUAL(el::mem::getInteger<uint32_t>(constSpan, ByteIndex{0U}), uint32_t{0x12345678U});
        REQUIRE_EQUAL(
            el::mem::getIntegerOrThrow<int32_t>(constSpan, ByteIndex{4U}, Endianness::Big), int32_t{-0x1234567});
        REQUIRE_EQUAL(
            el::mem::getInteger<uint64_t>(constSpan, ByteIndex{8U}, Endianness::Little, uint64_t{0xfeedbeefcafebabeU}),
            uint64_t{0xfeedbeefcafebabeU});

        auto output = int32_t{77};
        REQUIRE_FALSE(el::mem::getIntegerInto(constSpan, output, ByteIndex{10U}));
        REQUIRE_EQUAL(output, int32_t{77});
        const auto before = bytes;
        REQUIRE_FALSE(el::mem::setInteger(span, ByteIndex{11U}, uint16_t{0xffffU}));
        REQUIRE_EQUAL(bytes, before);
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, el::mem::getIntegerOrThrow<uint32_t>(constSpan, ByteIndex{10U}));
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError, el::mem::setIntegerOrThrow(span, ByteIndex::noIndex(), uint16_t{1U}));
    }
};
