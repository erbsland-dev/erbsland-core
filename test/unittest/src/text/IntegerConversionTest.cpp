// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/math/SaturatingInteger.hpp>
#include <erbsland/text/AnyStringBuilder.hpp>
#include <erbsland/text/IntegerFormat.hpp>
#include <erbsland/text/IntegerParseOptions.hpp>
#include <erbsland/text/IntegerSignMode.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

using el::err::OverflowError;
using el::err::ParseError;
using el::math::AnyIntegerType;
using el::math::SatInt16;
using el::math::SatUInt16;
using el::unit::CpLength;
using namespace el::text;
using namespace el::text::literals;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(
    IntegerBase LetterCase IntegerFormatFlag IntegerFormat IntegerParseFlag IntegerParseOptions IntegerSignMode
        IntegerType ParseError)
class IntegerConversionTest final : public el::UnitTest {
public:
    void testStableEnumValuesAndFlags() {
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerBase::Decimal), 0U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerBase::Hexadecimal), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerBase::Binary), 2U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerBase::Octal), 3U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Decimal}.baseFactor(), 10U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Hexadecimal}.baseFactor(), 16U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Octal}.baseFactor(), 8U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Binary}.digitGroupSize(), std::size_t{4U});
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Octal}.digitGroupSize(), std::size_t{3U});
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Decimal}.digitCount(std::int8_t{-128}), 3U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Decimal}.digitCount(std::uint8_t{255U}), 3U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Decimal}.digitCount(std::int16_t{-32768}), 5U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Decimal}.digitCount(std::uint16_t{65535U}), 5U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Decimal}.digitCount(std::int32_t{-2147483647 - 1}), 10U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Decimal}.digitCount(std::uint32_t{4294967295U}), 10U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Decimal}.digitCount(std::numeric_limits<std::int64_t>::min()), 19U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Decimal}.digitCount(std::numeric_limits<std::uint64_t>::max()), 20U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Hexadecimal}.digitCount(std::uint32_t{0xFFFFFFFFU}), 8U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Binary}.digitCount(std::uint8_t{0x80U}), 8U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Octal}.digitCount(std::uint16_t{0755U}), 3U);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Hexadecimal}.prefixChar(LetterCase::Uppercase), U'X');
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Binary}.prefixChar(LetterCase::Lowercase), U'b');
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Octal}.prefixChar(LetterCase::Uppercase), U'O');
        REQUIRE(IntegerBase{IntegerBase::Decimal}.prefixChar(LetterCase::Lowercase).isNull());
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Decimal}.toString(), "decimal"_el);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Hexadecimal}.toString(), "hexadecimal"_el);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Binary}.toString(), "binary"_el);
        REQUIRE_EQUAL(IntegerBase{IntegerBase::Octal}.toString(), "octal"_el);
        REQUIRE_EQUAL(IntegerBase::fromPrefixChar(U'x').value(), IntegerBase::Hexadecimal);
        REQUIRE_EQUAL(IntegerBase::fromPrefixChar(U'B').value(), IntegerBase::Binary);
        REQUIRE_EQUAL(IntegerBase::fromPrefixChar(U'o').value(), IntegerBase::Octal);
        REQUIRE_FALSE(IntegerBase::fromPrefixChar(U'd').has_value());
        REQUIRE_EQUAL(static_cast<std::uint8_t>(LetterCase::Lowercase), 0U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(LetterCase::Uppercase), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerSignMode::NegativeOnly), 0U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerSignMode::Always), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerSignMode::Space), 2U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerFormatFlag::ZeroFill), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerFormatFlag::Separator), 2U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerFormatFlag::BasePrefix), 4U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerFormatFlag::All), 7U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerParseFlag::AllowSeparator), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerParseFlag::IgnoreTrailingChars), 2U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerParseFlag::AcceptMinusSign), 4U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerParseFlag::IgnorePlusSign), 8U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerParseFlag::StopAtMaximum), 16U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(IntegerParseFlag::All), 31U);
        REQUIRE(AnyIntegerType<std::int32_t>);
        REQUIRE(AnyIntegerType<SatUInt16>);
        REQUIRE_FALSE(AnyIntegerType<bool>);

        auto formatFlags = IntegerFormatFlag::ZeroFill | IntegerFormatFlag::BasePrefix;
        REQUIRE(formatFlags.isSet(IntegerFormatFlag::ZeroFill));
        REQUIRE(formatFlags.isSet(IntegerFormatFlag::BasePrefix));
        REQUIRE_FALSE(formatFlags.isSet(IntegerFormatFlag::Separator));

        auto parseFlags = IntegerParseFlag::AllowSeparator | IntegerParseFlag::IgnoreTrailingChars;
        REQUIRE(parseFlags.isSet(IntegerParseFlag::AllowSeparator));
        REQUIRE(parseFlags.isSet(IntegerParseFlag::IgnoreTrailingChars));
    }

    void testParseOptionDefaultsAndFactories() {
        static constexpr auto constexprParserDefault = IntegerParseOptions::parserDefault();
        static_assert(!constexprParserDefault.hasFixedBase());
        static_assert(!constexprParserDefault.hasFlag(IntegerParseFlag::AcceptMinusSign));
        static_assert(constexprParserDefault.minimumDigits().isZero());
        static_assert(constexprParserDefault.maximumDigits().isInfinite());

        static constexpr auto constexprStringDefault = IntegerParseOptions::stringDefault();
        static_assert(constexprStringDefault.hasFlag(IntegerParseFlag::AcceptMinusSign));
        static_assert(constexprStringDefault.hasFlag(IntegerParseFlag::IgnorePlusSign));

        static constexpr auto constexprFixedDecimal = IntegerParseOptions::fixedDecimal(CpLength{4U});
        static_assert(constexprFixedDecimal.fixedBase() == IntegerBase::Decimal);
        static_assert(constexprFixedDecimal.minimumDigits() == CpLength{4U});
        static_assert(constexprFixedDecimal.maximumDigits() == CpLength{4U});
        static_assert(constexprFixedDecimal.hasFlag(IntegerParseFlag::StopAtMaximum));

        static constexpr auto constexprFixedHex = IntegerParseOptions::fixedHex(CpLength{2U});
        static_assert(constexprFixedHex.fixedBase() == IntegerBase::Hexadecimal);
        static_assert(constexprFixedHex.minimumDigits() == CpLength{2U});
        static_assert(constexprFixedHex.maximumDigits() == CpLength{2U});
        static_assert(constexprFixedHex.hasFlag(IntegerParseFlag::StopAtMaximum));

        const auto parserDefault = IntegerParseOptions::parserDefault();
        REQUIRE_FALSE(parserDefault.hasFlag(IntegerParseFlag::AcceptMinusSign));
        REQUIRE_FALSE(parserDefault.hasFlag(IntegerParseFlag::IgnorePlusSign));
        REQUIRE_FALSE(parserDefault.hasFlag(IntegerParseFlag::AllowSeparator));
        REQUIRE(parserDefault.maximumDigits().isInfinite());
        REQUIRE(parserDefault.minimumDigits().isZero());
        REQUIRE_EQUAL(parserDefault.separator(), U'\'');

        const auto defaultOptions = IntegerParseOptions::defaultOptions();
        REQUIRE_FALSE(defaultOptions.hasFlag(IntegerParseFlag::AcceptMinusSign));
        REQUIRE_FALSE(defaultOptions.hasFlag(IntegerParseFlag::IgnorePlusSign));

        const auto stringDefault = IntegerParseOptions::stringDefault();
        REQUIRE(stringDefault.hasFlag(IntegerParseFlag::AcceptMinusSign));
        REQUIRE(stringDefault.hasFlag(IntegerParseFlag::IgnorePlusSign));
        REQUIRE_FALSE(stringDefault.hasFlag(IntegerParseFlag::IgnoreTrailingChars));

        const auto fixedDecimal = IntegerParseOptions::fixedDecimal(CpLength{3U});
        REQUIRE_EQUAL(fixedDecimal.fixedBase().value(), IntegerBase::Decimal);
        REQUIRE_EQUAL(fixedDecimal.minimumDigits(), CpLength{3U});
        REQUIRE_EQUAL(fixedDecimal.maximumDigits(), CpLength{3U});
        REQUIRE(fixedDecimal.hasFlag(IntegerParseFlag::StopAtMaximum));

        const auto fixedHex = IntegerParseOptions::fixedHex(CpLength{2U});
        REQUIRE_EQUAL(fixedHex.fixedBase().value(), IntegerBase::Hexadecimal);
        REQUIRE_EQUAL(fixedHex.minimumDigits(), CpLength{2U});
        REQUIRE_EQUAL(fixedHex.maximumDigits(), CpLength{2U});
        REQUIRE(fixedHex.hasFlag(IntegerParseFlag::StopAtMaximum));
    }

    void testParseErrorPosition() {
        const auto withoutPosition = ParseError{"Invalid integer"};
        REQUIRE_FALSE(withoutPosition.hasPosition());
        REQUIRE(withoutPosition.position().isNoIndex());
        REQUIRE_EQUAL(StringConverter{withoutPosition.toString()}.toStdString(), std::string{"Invalid integer"});

        const auto withPosition = ParseError{"Invalid integer", el::unit::CpIndex{4U}};
        REQUIRE(withPosition.hasPosition());
        REQUIRE_EQUAL(withPosition.position(), el::unit::CpIndex{4U});
        const auto message = StringConverter{withPosition.toString()}.toStdString();
        REQUIRE(message.find("Invalid integer") != std::string::npos);
        REQUIRE(message.find("4") != std::string::npos);
    }

    void testSetFormatFromBasePrefix() {
        auto format = IntegerFormat{};

        REQUIRE(format.setFromBasePrefix(U'x'));
        REQUIRE_EQUAL(format.base(), IntegerBase::Hexadecimal);
        REQUIRE_EQUAL(format.letterCase(), LetterCase::Lowercase);

        REQUIRE(format.setFromBasePrefix(U'X'));
        REQUIRE_EQUAL(format.base(), IntegerBase::Hexadecimal);
        REQUIRE_EQUAL(format.letterCase(), LetterCase::Uppercase);

        REQUIRE(format.setFromBasePrefix(U'b'));
        REQUIRE_EQUAL(format.base(), IntegerBase::Binary);
        REQUIRE_EQUAL(format.letterCase(), LetterCase::Lowercase);

        REQUIRE(format.setFromBasePrefix(U'B'));
        REQUIRE_EQUAL(format.base(), IntegerBase::Binary);
        REQUIRE_EQUAL(format.letterCase(), LetterCase::Uppercase);

        REQUIRE(format.setFromBasePrefix(U'o'));
        REQUIRE_EQUAL(format.base(), IntegerBase::Octal);
        REQUIRE_EQUAL(format.letterCase(), LetterCase::Lowercase);

        REQUIRE(format.setFromBasePrefix(U'O'));
        REQUIRE_EQUAL(format.base(), IntegerBase::Octal);
        REQUIRE_EQUAL(format.letterCase(), LetterCase::Uppercase);

        REQUIRE_FALSE(format.setFromBasePrefix(U'd'));
        REQUIRE_EQUAL(format.base(), IntegerBase::Octal);
        REQUIRE_EQUAL(format.letterCase(), LetterCase::Uppercase);
    }

    void testFormatDefaultsAndSignedLimits() {
        REQUIRE_EQUAL(format(std::int32_t{0}), std::string{"0"});
        REQUIRE_EQUAL(format(std::int32_t{-42}), std::string{"-42"});
        REQUIRE_EQUAL(format(std::numeric_limits<std::int64_t>::min()), std::string{"-9223372036854775808"});
        REQUIRE_EQUAL(format(std::numeric_limits<std::uint64_t>::max()), std::string{"18446744073709551615"});
        REQUIRE_EQUAL(format(SatInt16{-123}), std::string{"-123"});
    }

    void testFormatBasesPrefixesCaseWidthAndSeparators() {
        auto hex = IntegerFormat::hexadecimal()
                       .setFlags(IntegerFormatFlag::BasePrefix | IntegerFormatFlag::ZeroFill)
                       .setLetterCase(LetterCase::Uppercase)
                       .setFieldWidth(CpLength{8U});
        REQUIRE_EQUAL(format(std::uint32_t{0x12AFU}, hex), std::string{"0X000012AF"});

        hex.setFlags(IntegerFormatFlag::BasePrefix | IntegerFormatFlag::Separator)
            .setLetterCase(LetterCase::Lowercase)
            .setFieldWidth(CpLength::zero());
        REQUIRE_EQUAL(format(std::numeric_limits<std::uint64_t>::max(), hex), std::string{"0xffff'ffff'ffff'ffff"});

        auto binary = IntegerFormat::binary().setFlags(IntegerFormatFlag::BasePrefix | IntegerFormatFlag::Separator);
        REQUIRE_EQUAL(format(std::uint16_t{0b10101100U}, binary), std::string{"0b1010'1100"});

        auto octal = IntegerFormat::octal()
                         .setFlags(IntegerFormatFlag::BasePrefix | IntegerFormatFlag::Separator)
                         .setLetterCase(LetterCase::Uppercase);
        REQUIRE_EQUAL(format(std::uint32_t{0755U}, octal), std::string{"0O755"});
        REQUIRE_EQUAL(
            format(std::uint32_t{01234567U}, IntegerFormat{IntegerBase::Octal}.setFlags(IntegerFormatFlag::Separator)),
            std::string{"1'234'567"});

        auto decimal = IntegerFormat::decimal().setFlags(IntegerFormatFlag::Separator);
        REQUIRE_EQUAL(format(std::int32_t{-1234567}, decimal), std::string{"-1'234'567"});

        decimal.setFlags({}).setFieldWidth(CpLength{5U});
        REQUIRE_EQUAL(format(std::int32_t{42}, decimal), std::string{"   42"});

        auto prefixedWidth =
            IntegerFormat::hexadecimal().setFlags(IntegerFormatFlag::BasePrefix).setFieldWidth(CpLength{4U});
        REQUIRE_EQUAL(format(std::uint8_t{0xffU}, prefixedWidth), std::string{"  0xff"});
        prefixedWidth.addFlags(IntegerFormatFlag::ZeroFill);
        REQUIRE_EQUAL(format(std::uint8_t{0xffU}, prefixedWidth), std::string{"0x00ff"});
    }

    void testFormatSignsAndPrecision() {
        auto signFormat = IntegerFormat::decimal().setSignMode(IntegerSignMode::Always);
        REQUIRE_EQUAL(format(std::int32_t{42}, signFormat), std::string{"+42"});
        REQUIRE_EQUAL(format(std::int32_t{-42}, signFormat), std::string{"-42"});

        signFormat.setSignMode(IntegerSignMode::Space);
        REQUIRE_EQUAL(format(std::uint32_t{42U}, signFormat), std::string{" 42"});

        auto precisionFormat = IntegerFormat::decimal().setPrecision(CpLength{5U});
        REQUIRE_EQUAL(format(std::int32_t{42}, precisionFormat), std::string{"00042"});

        precisionFormat.setSignMode(IntegerSignMode::Always);
        REQUIRE_EQUAL(format(std::int32_t{42}, precisionFormat), std::string{"+00042"});

        auto hexFormat =
            IntegerFormat::hexadecimal().setFlags(IntegerFormatFlag::BasePrefix).setPrecision(CpLength{4U});
        REQUIRE_EQUAL(format(std::uint32_t{0xffU}, hexFormat), std::string{"0x00ff"});

        auto separatedFormat =
            IntegerFormat::decimal().setFlags(IntegerFormatFlag::Separator).setPrecision(CpLength{7U});
        REQUIRE_EQUAL(format(std::uint32_t{1234U}, separatedFormat), std::string{"0'001'234"});

        auto widthAndPrecision = IntegerFormat::decimal().setFieldWidth(CpLength{6U}).setPrecision(CpLength{4U});
        REQUIRE_EQUAL(format(std::uint32_t{42U}, widthAndPrecision), std::string{"  0042"});
    }

    void testStringBuilderAppendInteger() {
        auto builder = AnyStringBuilder{StringKind::U16};
        builder.append(U'[').appendInteger(std::int32_t{-255}, IntegerFormat::hexadecimal()).append(U']');
        builder.append(char32_t{U'!'});

        REQUIRE_EQUAL(StringConverter{builder.toU8String()}.toStdString(), std::string{"[-ff]!"});
        REQUIRE_EQUAL(builder.length(), CpLength{6U});
    }

    void testParseDecimalAndDetectedBases() {
        REQUIRE_EQUAL(parse<std::int32_t>("123"), 123);
        REQUIRE_EQUAL(parse<std::int32_t>("+123"), 123);
        REQUIRE_EQUAL(parse<std::int8_t>("-128"), std::int8_t{-128});
        REQUIRE_EQUAL(parse<std::int16_t>("-32768"), std::numeric_limits<std::int16_t>::min());
        REQUIRE_EQUAL(parse<std::int32_t>("2147483647"), std::numeric_limits<std::int32_t>::max());
        REQUIRE_EQUAL(parse<std::int64_t>("-9223372036854775808"), std::numeric_limits<std::int64_t>::min());
        REQUIRE_EQUAL(parse<std::uint64_t>("18446744073709551615"), std::numeric_limits<std::uint64_t>::max());
        REQUIRE_EQUAL(parse<std::uint32_t>("0x7f"), 127U);
        REQUIRE_EQUAL(parse<std::uint32_t>("0X7F"), 127U);
        REQUIRE_EQUAL(parse<std::uint32_t>("0b1010"), 10U);
        REQUIRE_EQUAL(parse<std::uint32_t>("0o755"), 493U);
        REQUIRE_EQUAL(parse<std::uint32_t>("0O755"), 493U);

        REQUIRE_EQUAL(U8StringEditor{"-1"}.toInteger<std::uint32_t>(77U), 77U);
        REQUIRE_THROWS(U8StringEditor{"-1"}.toIntegerOrThrow<std::uint32_t>());
    }

    void testParseFixedBasesAndPrefixes() {
        auto hex = IntegerParseOptions{};
        hex.setFixedBase(IntegerBase::Hexadecimal);
        REQUIRE_EQUAL(parse<std::uint32_t>("ff", hex), 255U);
        REQUIRE_EQUAL(U8StringEditor{"0xff"}.toInteger<std::int32_t>(-1, hex), -1);

        auto binary = IntegerParseOptions{};
        binary.setFixedBase(IntegerBase::Binary);
        REQUIRE_EQUAL(parse<std::uint32_t>("1111", binary), 15U);
        REQUIRE_EQUAL(U8StringEditor{"0b1111"}.toInteger<std::int32_t>(-1, binary), -1);

        auto octal = IntegerParseOptions{};
        octal.setFixedBase(IntegerBase::Octal);
        REQUIRE_EQUAL(parse<std::uint32_t>("755", octal), 493U);
        REQUIRE_EQUAL(U8StringEditor{"0o755"}.toInteger<std::int32_t>(-1, octal), -1);

        auto decimal = IntegerParseOptions{};
        decimal.setFixedBase(IntegerBase::Decimal);
        REQUIRE_EQUAL(U8StringEditor{"0x10"}.toInteger<std::int32_t>(-1, decimal), -1);
    }

    void testParseTrailingAndSeparators() {
        REQUIRE_EQUAL(U8StringEditor{"1'234"}.toInteger<std::int32_t>(-1), -1);

        auto separators = IntegerParseOptions{};
        separators.setFlags(IntegerParseFlag::AllowSeparator);
        REQUIRE_EQUAL(parse<std::int32_t>("1'234", separators), 1234);
        REQUIRE_EQUAL(U8StringEditor{"'1234"}.toInteger<std::int32_t>(-1, separators), -1);
        REQUIRE_EQUAL(U8StringEditor{"1234'"}.toInteger<std::int32_t>(-1, separators), -1);
        REQUIRE_EQUAL(U8StringEditor{"12''34"}.toInteger<std::int32_t>(-1, separators), -1);
        REQUIRE_EQUAL(U8StringEditor{"0x'1234"}.toInteger<std::int32_t>(-1, separators), -1);

        REQUIRE_EQUAL(U8StringEditor{"123abc"}.toInteger<std::int32_t>(-1), -1);
        auto trailing = IntegerParseOptions{};
        trailing.setFlags(IntegerParseFlag::IgnoreTrailingChars);
        REQUIRE_EQUAL(parse<std::int32_t>("123abc", trailing), 123);

        auto both = IntegerParseOptions{};
        both.setFlags(IntegerParseFlag::AllowSeparator | IntegerParseFlag::IgnoreTrailingChars);
        REQUIRE_EQUAL(U8StringEditor{"123'abc"}.toInteger<std::int32_t>(-1, both), -1);

        auto customSeparator = IntegerParseOptions::stringDefault();
        customSeparator.setFlags(
            IntegerParseFlag::AllowSeparator | IntegerParseFlag::AcceptMinusSign | IntegerParseFlag::IgnorePlusSign);
        customSeparator.setSeparator(U'_');
        REQUIRE_EQUAL(parse<std::int32_t>("1_234", customSeparator), 1234);
        REQUIRE_EQUAL(U8StringEditor{"1__234"}.toInteger<std::int32_t>(-1, customSeparator), -1);
    }

    void testParseStringAndParserDefaults() {
        REQUIRE_EQUAL(U8StringEditor{"+123"}.toInteger<std::int32_t>(), 123);
        REQUIRE_EQUAL(U8StringEditor{"-123"}.toInteger<std::int32_t>(), -123);
        REQUIRE_EQUAL(U8StringEditor{"+123"}.toInteger<std::int32_t>(-1, IntegerParseOptions::parserDefault()), -1);
        REQUIRE_EQUAL(U8StringEditor{"-123"}.toInteger<std::int32_t>(-1, IntegerParseOptions::parserDefault()), -1);
    }

    void testParseOverflowAndExceptionKinds() {
        REQUIRE_EQUAL(U8StringEditor{"128"}.toInteger<std::int8_t>(std::int8_t{-1}), std::int8_t{-1});

        try {
            static_cast<void>(U8StringEditor{"128"}.toIntegerOrThrow<std::int8_t>());
            REQUIRE(false);
        } catch (const OverflowError &) {
            REQUIRE(true);
        }

        try {
            static_cast<void>(U8StringEditor{"abc"}.toIntegerOrThrow<std::int32_t>());
            REQUIRE(false);
        } catch (const ParseError &) {
            REQUIRE(true);
        }
    }

    void testAllStringKindsAndViews() {
        const auto u8 = U8StringEditor{"42"};
        const auto u16 = U16StringEditor{std::u16string_view{u"42"}};
        const auto u32 = U32StringEditor{std::u32string_view{U"42"}};

        REQUIRE_EQUAL(u8.toInteger<std::int32_t>(), 42);
        REQUIRE_EQUAL(U8String{u8}.toInteger<std::int32_t>(), 42);
        REQUIRE_EQUAL(u16.toInteger<std::int32_t>(), 42);
        REQUIRE_EQUAL(U16String{u16}.toInteger<std::int32_t>(), 42);
        REQUIRE_EQUAL(u32.toInteger<std::int32_t>(), 42);
        REQUIRE_EQUAL(U32String{u32}.toInteger<std::int32_t>(), 42);

        REQUIRE_EQUAL(StringConverter{U16String::fromInteger(std::int32_t{-42})}.toStdString(), std::string{"-42"});
        REQUIRE_EQUAL(StringConverter{U32String::fromInteger(std::uint32_t{42})}.toStdString(), std::string{"42"});
    }

    void testSmallIntegerRoundTrips() {
        for (auto value = -128; value <= 127; ++value) {
            const auto text = U8String::fromInteger(static_cast<std::int8_t>(value));
            REQUIRE_EQUAL(static_cast<int>(text.toInteger<std::int8_t>()), value);
        }
        for (auto value = 0; value <= 255; ++value) {
            const auto text = U8String::fromInteger(static_cast<std::uint8_t>(value));
            REQUIRE_EQUAL(static_cast<unsigned int>(text.toInteger<std::uint8_t>()), static_cast<unsigned int>(value));
        }
    }

    void testMalformedInputFailsAsParseError() {
        const auto invalidUtf8 = U8StringEditor{std::string_view{th::stdStringFromHex("31 C0 32")}};
        REQUIRE_EQUAL(invalidUtf8.toInteger<std::int32_t>(-1), -1);
        REQUIRE_THROWS(invalidUtf8.toIntegerOrThrow<std::int32_t>());

        const auto invalidUtf16 = U16StringEditor{std::u16string{u'1', char16_t{0xD800U}, u'2'}};
        REQUIRE_EQUAL(invalidUtf16.toInteger<std::int32_t>(-1), -1);
        REQUIRE_THROWS(invalidUtf16.toIntegerOrThrow<std::int32_t>());

        const auto invalidUtf32 = U32StringEditor{std::u32string{U'1', char32_t{0x110000U}, U'2'}};
        REQUIRE_EQUAL(invalidUtf32.toInteger<std::int32_t>(-1), -1);
        REQUIRE_THROWS(invalidUtf32.toIntegerOrThrow<std::int32_t>());
    }

private:
    template <AnyIntegerType T>
    [[nodiscard]] static auto format(T value, IntegerFormat format = IntegerFormat::defaultFormat()) -> std::string {
        return StringConverter{U8String::fromInteger(value, format)}.toStdString();
    }

    template <AnyIntegerType T>
    [[nodiscard]] static auto parse(
        std::string_view text, IntegerParseOptions options = IntegerParseOptions::stringDefault()) -> T {
        return U8StringEditor{text}.toIntegerOrThrow<T>(options);
    }
};
