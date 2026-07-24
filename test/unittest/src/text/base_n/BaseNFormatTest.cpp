// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/base_n/BaseNFormat.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text;
using namespace el::text::base_n;
using namespace el::text::literals;

TESTED_TARGETS(BaseNFormat BaseNFormatFlag)
class BaseNFormatTest final : public el::UnitTest {
public:
    void testStandardFactories() {
        requireFormat(BaseNFormat::base16(), 16U, 4U, false);
        requireFormat(BaseNFormat::base32(), 32U, 5U, true);
        requireFormat(BaseNFormat::base32Hex(), 32U, 5U, true);
        requireFormat(BaseNFormat::base64(), 64U, 6U, true);
        requireFormat(BaseNFormat::base64Url(), 64U, 6U, true);
        REQUIRE_EQUAL((BaseNFormat{}.alphabet()), BaseNFormat::base64().alphabet());

        const auto pem = BaseNFormat::base64Pem();
        REQUIRE(pem.hasFlag(BaseNFormatFlag::WrapLines));
        REQUIRE_EQUAL(pem.lineLength(), el::unit::CpLength{64U});
        REQUIRE_EQUAL(pem.lineSeparator(), U"\n"_el);
    }

    void testCustomization() {
        auto format = BaseNFormat::base64();
        format.clearFlags(BaseNFormatFlag::RequirePadding | BaseNFormatFlag::EmitPadding);
        REQUIRE_FALSE(format.hasFlag(BaseNFormatFlag::RequirePadding));
        REQUIRE_FALSE(format.hasFlag(BaseNFormatFlag::EmitPadding));
        format.setWhitespace(CharSet{U'~'});
        REQUIRE(format.whitespace().contains(U'~'));
        format.setLineSeparator(U32String{U"~"_el});
        format.addFlags(BaseNFormatFlag::WrapLines);
        format.setLineLength(el::unit::CpLength{10U});
        REQUIRE_EQUAL(format.lineLength(), el::unit::CpLength{10U});
    }

    void testInvalidFormats() {
        REQUIRE_THROWS_AS(el::err::ParameterError, (BaseNFormat{U32String{U"0123456789ABCDE"_el}}));
        REQUIRE_THROWS_AS(el::err::ParameterError, (BaseNFormat{U32String{U"00123456789ABCDE"_el}}));

        auto format = BaseNFormat::base64();
        const auto originalAlphabet = format.alphabet();
        REQUIRE_THROWS_AS(el::err::ParameterError, format.setPadding(U'A'));
        REQUIRE_THROWS_AS(el::err::ParameterError, format.setWhitespace(CharSet{U'A'}));
        REQUIRE_THROWS_AS(el::err::ParameterError, format.setWhitespace(CharSet{U'='}));
        REQUIRE_THROWS_AS(el::err::ParameterError, format.setPadding(std::nullopt));
        REQUIRE_THROWS_AS(el::err::ParameterError, format.setPadding(Char::noCodePoint()));
        REQUIRE_EQUAL(format.alphabet(), originalAlphabet);
        REQUIRE_EQUAL(format.padding(), std::optional<Char>{U'='});
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            format.setLineLength(el::unit::CpLength::zero()).addFlags(BaseNFormatFlag::WrapLines));

        auto noPadding = BaseNFormat{U32String{U"0123456789ABCDEF"_el}};
        REQUIRE_THROWS_AS(el::err::ParameterError, noPadding.addFlags(BaseNFormatFlag::EmitPadding));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            noPadding.setLineSeparator(U32String{U"x"_el}).addFlags(BaseNFormatFlag::WrapLines));
    }

private:
    void requireFormat(
        const BaseNFormat &format, const std::size_t alphabetLength, const uint8_t bits, const bool padded) {
        REQUIRE_EQUAL(format.alphabet().length().toSizeT(), alphabetLength);
        REQUIRE_EQUAL(format.bitsPerCharacter(), bits);
        REQUIRE_EQUAL(format.padding().has_value(), padded);
        REQUIRE(format.whitespace().contains(U' '));
        for (std::size_t i = 0; i < alphabetLength; ++i) {
            const auto character = format.characterFor(static_cast<uint8_t>(i));
            REQUIRE(format.valueFor(character) == static_cast<uint8_t>(i));
        }
    }
};
