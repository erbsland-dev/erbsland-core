// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringView.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringCharView.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <string>
#include <vector>

using el::unit::CpIndex;
using el::unit::CpLength;
using el::unit::CpRange;
using namespace el::text;

TESTED_TARGETS(StringEncoder StringEncoderTraits)
class StringEncoderTest final : public el::UnitTest {
public:
    void testEncodeUtf8WithoutBom() {
        const auto text = U8String{std::u8string_view{u8"A¢"}};

        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf8, StringBomMode::Reject).toUInt8Vector(),
            std::vector<uint8_t>({0x41U, 0xC2U, 0xA2U}));
    }

    void testEncodeUtf16ViewWithRequiredBom() {
        const auto text = U16String{std::u16string_view{u"A¢"}};

        REQUIRE_EQUAL(
            StringEncoder{U16StringView{text}}
                .encode(StringEncoding::Utf16LittleEndian, StringBomMode::Require)
                .toUInt8Vector(),
            std::vector<uint8_t>({0xFFU, 0xFEU, 0x41U, 0x00U, 0xA2U, 0x00U}));
    }

    void testEncodeUtf32AndCharView() {
        const auto text = U32String{std::u32string_view{U"A¢"}};
        const auto charView =
            U8String{std::u8string_view{u8"xxA¢yy"}}.toCharView().slice(CpRange{CpIndex{2U}, CpLength{2U}});

        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf32LittleEndian, StringBomMode::Reject).toUInt8Vector(),
            std::vector<uint8_t>({0x41U, 0x00U, 0x00U, 0x00U, 0xA2U, 0x00U, 0x00U, 0x00U}));
        REQUIRE_EQUAL(
            StringEncoder{charView}.encode(StringEncoding::Utf8, StringBomMode::Reject).toUInt8Vector(),
            std::vector<uint8_t>({0x41U, 0xC2U, 0xA2U}));
    }
};
