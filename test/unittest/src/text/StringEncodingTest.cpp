// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StringEncoding.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <span>
#include <vector>

using el::mem::Byte;
using el::text::StringBomMode;
using el::text::StringEncoding;
using el::unit::ByteLength;

TESTED_TARGETS(StringEncoding)
class StringEncodingTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto toBytes(const std::span<const Byte> bytes) -> std::vector<uint8_t> {
        auto result = std::vector<uint8_t>{};
        result.reserve(bytes.size());
        for (const auto byte : bytes) {
            result.push_back(byte.toUInt8());
        }
        return result;
    }

public:
    void testValueClassAndFamilies() {
        constexpr auto defaultEncoding = StringEncoding{};
        constexpr auto utf16 = StringEncoding{StringEncoding::Utf16};

        static_assert(defaultEncoding == StringEncoding::Utf8);
        static_assert(utf16.toRawValue() == StringEncoding::Utf16);
        static_assert(utf16.isUtf16());
        static_assert(!utf16.isUtf8());
        static_assert(!utf16.isUtf32());
        static_assert(StringEncoding{StringEncoding::Utf32BigEndian}.isUtf32());
        REQUIRE_EQUAL(defaultEncoding, StringEncoding::Utf8);
        REQUIRE(StringEncoding{StringEncoding::Utf8}.isUtf8());
        REQUIRE(StringEncoding{StringEncoding::Utf16LittleEndian}.isUtf16());
        REQUIRE(StringEncoding{StringEncoding::Utf32LittleEndian}.isUtf32());
    }

    void testEffectiveEncoding() {
        static_assert(StringEncoding{StringEncoding::Utf16}.effectiveEncoding() == StringEncoding::Utf16LittleEndian);
        static_assert(StringEncoding{StringEncoding::Utf32}.effectiveEncoding() == StringEncoding::Utf32LittleEndian);
        static_assert(
            StringEncoding{StringEncoding::Utf16BigEndian}.effectiveEncoding() == StringEncoding::Utf16BigEndian);
        REQUIRE_EQUAL(StringEncoding{StringEncoding::Utf8}.effectiveEncoding(), StringEncoding::Utf8);
        REQUIRE_EQUAL(StringEncoding{StringEncoding::Utf8}.endianness(), el::mem::Endianness::Little);
        REQUIRE_EQUAL(StringEncoding{StringEncoding::Utf16}.endianness(), el::mem::Endianness::Little);
        REQUIRE_EQUAL(StringEncoding{StringEncoding::Utf32LittleEndian}.endianness(), el::mem::Endianness::Little);
        REQUIRE_EQUAL(StringEncoding{StringEncoding::Utf16BigEndian}.endianness(), el::mem::Endianness::Big);
        REQUIRE_EQUAL(StringEncoding{StringEncoding::Utf32BigEndian}.endianness(), el::mem::Endianness::Big);
    }

    void testBomPolicy() {
        for (
            const auto value :
            {StringEncoding::Utf8,
                StringEncoding::Utf16,
                StringEncoding::Utf16LittleEndian,
                StringEncoding::Utf16BigEndian,
                StringEncoding::Utf32,
                StringEncoding::Utf32LittleEndian,
                StringEncoding::Utf32BigEndian}) {
            const auto encoding = StringEncoding{value};
            REQUIRE_FALSE(encoding.writesBom(StringBomMode::Reject));
            REQUIRE(encoding.writesBom(StringBomMode::Require));
            REQUIRE_EQUAL(encoding.writesBom(StringBomMode::Automatic), !encoding.isUtf8());
        }
    }

    void testBomBytesAndLengths() {
        REQUIRE_EQUAL(
            toBytes(StringEncoding{StringEncoding::Utf8}.bomBytes(StringBomMode::Require)),
            std::vector<uint8_t>({0xefU, 0xbbU, 0xbfU}));
        REQUIRE_EQUAL(
            toBytes(StringEncoding{StringEncoding::Utf16}.bomBytes(StringBomMode::Automatic)),
            std::vector<uint8_t>({0xffU, 0xfeU}));
        REQUIRE_EQUAL(
            toBytes(StringEncoding{StringEncoding::Utf16BigEndian}.bomBytes(StringBomMode::Require)),
            std::vector<uint8_t>({0xfeU, 0xffU}));
        REQUIRE_EQUAL(
            toBytes(StringEncoding{StringEncoding::Utf32}.bomBytes(StringBomMode::Automatic)),
            std::vector<uint8_t>({0xffU, 0xfeU, 0x00U, 0x00U}));
        REQUIRE_EQUAL(
            toBytes(StringEncoding{StringEncoding::Utf32BigEndian}.bomBytes(StringBomMode::Require)),
            std::vector<uint8_t>({0x00U, 0x00U, 0xfeU, 0xffU}));

        REQUIRE_EQUAL(StringEncoding{StringEncoding::Utf8}.bomLength(StringBomMode::Automatic), ByteLength::zero());
        REQUIRE_EQUAL(StringEncoding{StringEncoding::Utf8}.bomLength(StringBomMode::Require), ByteLength{3U});
        REQUIRE_EQUAL(
            StringEncoding{StringEncoding::Utf16LittleEndian}.bomLength(StringBomMode::Require), ByteLength{2U});
        REQUIRE_EQUAL(
            StringEncoding{StringEncoding::Utf32LittleEndian}.bomLength(StringBomMode::Require), ByteLength{4U});
        REQUIRE(StringEncoding{StringEncoding::Utf32}.bomBytes(StringBomMode::Reject).empty());
    }
};
