// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/base_n/BaseNDecoder.hpp>
#include <erbsland/text/base_n/BaseNEncoder.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace el::text;
using namespace el::text::base_n;
using namespace el::text::literals;

TESTED_TARGETS(BaseNEncoder BaseNDecoder)
class BaseNCodecTest final : public el::UnitTest {
public:
    struct Vector {
        std::string_view plain;
        std::string_view encoded;
    };

    void testRfc4648Base64() {
        requireVectors(
            BaseNFormat::base64(),
            {
                {"", ""},
                {"f", "Zg=="},
                {"fo", "Zm8="},
                {"foo", "Zm9v"},
                {"foob", "Zm9vYg=="},
                {"fooba", "Zm9vYmE="},
                {"foobar", "Zm9vYmFy"},
            });
    }

    void testRfc4648Base32() {
        requireVectors(
            BaseNFormat::base32(),
            {
                {"", ""},
                {"f", "MY======"},
                {"fo", "MZXQ===="},
                {"foo", "MZXW6==="},
                {"foob", "MZXW6YQ="},
                {"fooba", "MZXW6YTB"},
                {"foobar", "MZXW6YTBOI======"},
            });
    }

    void testRfc4648Base32Hex() {
        requireVectors(
            BaseNFormat::base32Hex(),
            {
                {"", ""},
                {"f", "CO======"},
                {"fo", "CPNG===="},
                {"foo", "CPNMU==="},
                {"foob", "CPNMUOG="},
                {"fooba", "CPNMUOJ1"},
                {"foobar", "CPNMUOJ1E8======"},
            });
    }

    void testRfc4648Base16() {
        requireVectors(
            BaseNFormat::base16(),
            {
                {"", ""},
                {"f", "66"},
                {"fo", "666F"},
                {"foo", "666F6F"},
                {"foob", "666F6F62"},
                {"fooba", "666F6F6261"},
                {"foobar", "666F6F626172"},
            });
    }

    void testBase64Url() {
        const auto bytes = el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0xfbU, 0xffU});
        const auto encoded = BaseNEncoder{bytes, BaseNFormat::base64Url()}.toString();
        REQUIRE_EQUAL(encoded, "-_8="_el);
        REQUIRE_EQUAL((BaseNDecoder{encoded, BaseNFormat::base64Url()}.toDataOrThrow()), bytes);
        requireVectors(
            BaseNFormat::base64Url(),
            {
                {"", ""},
                {"f", "Zg=="},
                {"fo", "Zm8="},
                {"foo", "Zm9v"},
                {"foob", "Zm9vYg=="},
                {"fooba", "Zm9vYmE="},
                {"foobar", "Zm9vYmFy"},
            });
    }

    void testEveryByteRoundTrips() {
        auto values = std::vector<uint8_t>{};
        for (unsigned int i = 0; i <= 0xffU; ++i) {
            values.push_back(static_cast<uint8_t>(i));
        }
        const auto data = el::mem::ByteBlock::fromVector(values);
        const auto formats = std::array{
            BaseNFormat::base16(),
            BaseNFormat::base32(),
            BaseNFormat::base32Hex(),
            BaseNFormat::base64(),
            BaseNFormat::base64Url(),
        };
        for (const auto &format : formats) {
            WITH_CONTEXT(requireRoundTrip(data, format));
            for (std::size_t length = 0; length <= 20U; ++length) {
                WITH_CONTEXT(requireRoundTrip(data.slice(el::unit::ByteIndex{}, el::unit::ByteLength{length}), format));
            }
        }
    }

    void testLargeDeterministicRoundTrips() {
        auto values = std::vector<uint8_t>(64U * 1024U);
        auto state = uint32_t{0x6d2b79f5U};
        for (auto &value : values) {
            state ^= state << 13U;
            state ^= state >> 17U;
            state ^= state << 5U;
            value = static_cast<uint8_t>(state);
        }
        const auto data = el::mem::ByteBlock::fromVector(values);
        for (
            const auto &format :
            {BaseNFormat::base16(), BaseNFormat::base32(), BaseNFormat::base64(), BaseNFormat::base64Pem()}) {
            WITH_CONTEXT(requireRoundTrip(data, format));
        }
    }

    void testStringWidthsAndUnicodeAlphabet() {
        const auto data = block("hello");
        const auto encoder = BaseNEncoder{data};
        REQUIRE_EQUAL(StringConverter{encoder.toU16String()}.toString(), encoder.toString());
        REQUIRE_EQUAL(StringConverter{encoder.toU32String()}.toString(), encoder.toString());
        REQUIRE_EQUAL((BaseNDecoder{encoder.toU16String()}.toDataOrThrow()), data);
        REQUIRE_EQUAL((BaseNDecoder{encoder.toU32String()}.toDataOrThrow()), data);

        const auto alphabet = U32String{U"😀😁😂😃😄😅😆😇😈😉😊😋😌😍😎😏"_el};
        const auto format = BaseNFormat{alphabet};
        const auto unicodeText = BaseNEncoder{data, format}.toU16String();
        REQUIRE_EQUAL((BaseNDecoder{unicodeText, format}.toDataOrThrow()), data);
    }

    void testWhitespaceAndOptionalPadding() {
        REQUIRE_EQUAL((BaseNDecoder{" \tZ\vg\f=\r=\n"_el}.toDataOrThrow()), block("f"));

        auto optional = BaseNFormat::base64();
        optional.clearFlags(BaseNFormatFlag::RequirePadding);
        REQUIRE_EQUAL((BaseNDecoder{"Zg"_el, optional}.toDataOrThrow()), block("f"));
        optional.clearFlags(BaseNFormatFlag::EmitPadding);
        REQUIRE_EQUAL((BaseNEncoder{block("f"), optional}.toString()), "Zg"_el);

        optional.setWhitespace(CharSet{U'~'});
        REQUIRE_EQUAL((BaseNDecoder{"Z~g"_el, optional}.toDataOrThrow()), block("f"));
        REQUIRE_FALSE((BaseNDecoder{"Z g"_el, optional}.toData().has_value()));
    }

    void testPemWrapping() {
        const auto data48 = el::mem::ByteBlock{el::unit::ByteLength{48U}, el::mem::Byte{0U}};
        const auto text48 = BaseNEncoder{data48, BaseNFormat::base64Pem()}.toString();
        REQUIRE_EQUAL(text48.characterLength(), el::unit::CpLength{64U});
        REQUIRE_FALSE(text48.endsWith("\n"_el));

        const auto data49 = el::mem::ByteBlock{el::unit::ByteLength{49U}, el::mem::Byte{0U}};
        const auto text49 = BaseNEncoder{data49, BaseNFormat::base64Pem()}.toString();
        REQUIRE(text49.contains("\n"_el));
        REQUIRE_FALSE(text49.endsWith("\n"_el));
        REQUIRE_EQUAL((BaseNDecoder{text49, BaseNFormat::base64Pem()}.toDataOrThrow()), data49);
    }

    void testMalformedInput() {
        requireInvalid("A");
        requireInvalid("Zg=");
        requireInvalid("Zg===");
        requireInvalid("Zh==");
        requireInvalid("Zm=8");
        requireInvalid("Z$==");
        requireInvalid("====");
        requireInvalid("A===");
        requireInvalid("Zm9v=");
        requireInvalidWithFormat("M=======", BaseNFormat::base32());
        requireInvalidWithFormat("MZ======", BaseNFormat::base32());
        requireInvalidWithFormat("MY=====", BaseNFormat::base32());
        requireInvalidWithFormat("MY=======", BaseNFormat::base32());
        REQUIRE_FALSE((BaseNDecoder{"0"_el, BaseNFormat::base16()}.toData().has_value()));
        REQUIRE_THROWS_AS(el::err::ParseError, (BaseNDecoder{"Zh=="_el}.toDataOrThrow()));
    }

    void testMaximum() {
        REQUIRE_FALSE((BaseNDecoder{"Zm9v"_el}.toData(el::unit::ByteLength{2U}).has_value()));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, BaseNDecoder{"Zm9v"_el}.toDataOrThrow(el::unit::ByteLength{2U}));
        REQUIRE_EQUAL((BaseNDecoder{"Zm9v"_el}.toDataOrThrow(el::unit::ByteLength{3U})), block("foo"));
    }

private:
    static auto block(const std::string_view text) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromSpan(std::span<const char>{text});
    }

    void requireVectors(const BaseNFormat &format, const std::initializer_list<Vector> vectors) {
        for (const auto &[plain, encoded] : vectors) {
            const auto actual = BaseNEncoder{block(plain), format}.toString();
            REQUIRE_EQUAL(StringConverter{actual}.toStdString(), std::string{encoded});
            REQUIRE_EQUAL((BaseNDecoder{actual, format}.toDataOrThrow()), block(plain));
        }
    }

    void requireInvalid(const std::string_view text) {
        REQUIRE_FALSE((BaseNDecoder{String{text}}.toData().has_value()));
    }

    void requireInvalidWithFormat(const std::string_view text, const BaseNFormat &format) {
        REQUIRE_FALSE((BaseNDecoder{String{text}, format}.toData().has_value()));
    }

    void requireRoundTrip(const el::mem::ByteBlock &data, const BaseNFormat &format) {
        const auto encoded = BaseNEncoder{data, format}.toString();
        REQUIRE_EQUAL((BaseNDecoder{encoded, format}.toDataOrThrow()), data);
    }
};
