// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/AnyStringBuilder.hpp>
#include <erbsland/text/FloatParseOptions.hpp>
#include <erbsland/text/impl/FloatConversion.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

using el::err::OverflowError;
using el::err::ParseError;
using el::unit::ItemCount;
using namespace el::text;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(FloatFormat FloatParseFlag FloatParseOptions StringEditor U16StringEditor U32StringEditor)
class FloatConversionTest final : public el::UnitTest {
public:
    void testFormatDefaultsAndFactories() {
        auto format = FloatFormat{};

        REQUIRE_EQUAL(format.style(), FloatFormat::Style::Default);
        REQUIRE_FALSE(format.hasPrecision());
        REQUIRE_EQUAL(format.precision(), ItemCount{0U});
        REQUIRE_EQUAL(format.letterCase(), LetterCase::Lowercase);

        format.setStyle(FloatFormat::Style::Fixed).setPrecision(ItemCount{2U}).setLetterCase(LetterCase::Uppercase);
        REQUIRE_EQUAL(format.style(), FloatFormat::Style::Fixed);
        REQUIRE(format.hasPrecision());
        REQUIRE_EQUAL(format.precision(), ItemCount{2U});
        REQUIRE_EQUAL(format.letterCase(), LetterCase::Uppercase);

        format.clearPrecision();
        REQUIRE_FALSE(format.hasPrecision());

        REQUIRE_EQUAL(FloatFormat::fixed().style(), FloatFormat::Style::Fixed);
        REQUIRE_EQUAL(FloatFormat::scientific().style(), FloatFormat::Style::Scientific);
        REQUIRE_EQUAL(FloatFormat::general().style(), FloatFormat::Style::General);
        REQUIRE_EQUAL(FloatFormat::hexadecimal().style(), FloatFormat::Style::Hexadecimal);
    }

    void testParseOptionsDefaultsAndFlags() {
        REQUIRE_EQUAL(static_cast<std::uint8_t>(FloatParseOptions::Style::General), 0U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(FloatParseOptions::Style::Fixed), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(FloatParseOptions::Style::Scientific), 2U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(FloatParseOptions::Style::Hexadecimal), 3U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(FloatParseFlag::IgnoreTrailingChars), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(FloatParseFlag::All), 1U);

        REQUIRE(el::text::impl::AnyFloatType<float>);
        REQUIRE(el::text::impl::AnyFloatType<double>);
        REQUIRE_FALSE(el::text::impl::AnyFloatType<const float>);
        REQUIRE_FALSE(el::text::impl::AnyFloatType<const double>);
        REQUIRE_FALSE(el::text::impl::AnyFloatType<long double>);
        REQUIRE_FALSE(el::text::impl::AnyFloatType<int>);
        REQUIRE_FALSE(el::text::impl::AnyFloatType<bool>);

        auto options = FloatParseOptions{};
        REQUIRE_EQUAL(options.style(), FloatParseOptions::Style::General);
        REQUIRE(options.flags().isEmpty());
        REQUIRE_FALSE(options.hasFlag(FloatParseFlag::IgnoreTrailingChars));

        options.setStyle(FloatParseOptions::Style::Hexadecimal).setFlags(FloatParseFlag::IgnoreTrailingChars);
        REQUIRE_EQUAL(options.style(), FloatParseOptions::Style::Hexadecimal);
        REQUIRE(options.hasFlag(FloatParseFlag::IgnoreTrailingChars));

        options.clearFlags(FloatParseFlag::IgnoreTrailingChars);
        REQUIRE_FALSE(options.hasFlag(FloatParseFlag::IgnoreTrailingChars));

        options.addFlags(FloatParseFlag::IgnoreTrailingChars);
        REQUIRE(options.hasFlag(FloatParseFlag::IgnoreTrailingChars));
    }

    void testU8StringFromFloat() {
        auto fixed = FloatFormat::fixed().setPrecision(ItemCount{2U});
        REQUIRE_EQUAL(StringConverter{String::fromFloat(12.345, fixed)}.toStdString(), std::string{"12.35"});

        auto scientific = FloatFormat::scientific().setPrecision(ItemCount{1U}).setLetterCase(LetterCase::Uppercase);
        REQUIRE_EQUAL(StringConverter{String::fromFloat(12.345, scientific)}.toStdString(), std::string{"1.2E+01"});

        auto hexadecimal = FloatFormat::hexadecimal().setPrecision(ItemCount{2U}).setLetterCase(LetterCase::Uppercase);
        REQUIRE_EQUAL(StringConverter{String::fromFloat(12.0, hexadecimal)}.toStdString(), std::string{"1.80P+3"});
    }

    void testU16AndU32StringFromFloat() {
        auto fixed = FloatFormat::fixed().setPrecision(ItemCount{1U});

        REQUIRE_EQUAL(StringConverter{U16String::fromFloat(1.25, fixed)}.toStdString(), std::string{"1.2"});
        REQUIRE_EQUAL(StringConverter{U32String::fromFloat(1.25, fixed)}.toStdString(), std::string{"1.2"});
    }

    void testStringBuilderAppendFloat() {
        auto fixed = FloatFormat::fixed().setPrecision(ItemCount{1U});

        auto builder = AnyStringBuilder::u8();
        builder.appendFloat(1.25F, fixed).append(U',').appendFloat(2.25, fixed);

        REQUIRE_EQUAL(StringConverter{builder.toString()}.toStdString(), std::string{"1.2,2.2"});
    }

    void testParseFloatAndDouble() {
        REQUIRE_EQUAL(parse<double>("12.5"), 12.5);
        REQUIRE_EQUAL(parse<double>("+12.5"), 12.5);
        REQUIRE_EQUAL(parse<double>("-12.5"), -12.5);
        REQUIRE_EQUAL(parse<float>("1.25"), 1.25F);
        REQUIRE_EQUAL(parse<double>("1.25e2"), 125.0);
    }

    void testParseAllStringKindsAndViews() {
        const auto u8 = StringEditor{"42.5"};
        const auto u16 = U16StringEditor{std::u16string_view{u"42.5"}};
        const auto u32 = U32StringEditor{std::u32string_view{U"42.5"}};

        REQUIRE_EQUAL(u8.toFloat<double>(), 42.5);
        REQUIRE_EQUAL(String{u8}.toFloat<float>(), 42.5F);
        REQUIRE_EQUAL(u16.toFloat<double>(), 42.5);
        REQUIRE_EQUAL(U16String{u16}.toFloat<float>(), 42.5F);
        REQUIRE_EQUAL(u32.toFloat<double>(), 42.5);
        REQUIRE_EQUAL(U32String{u32}.toFloat<float>(), 42.5F);

        REQUIRE_EQUAL(String{u8}.toFloat<double>(), 42.5);
        REQUIRE_EQUAL(u16.toFloat<float>(), 42.5F);
        REQUIRE_EQUAL(U16String{u16}.toFloat<double>(), 42.5);
        REQUIRE_EQUAL(u32.toFloat<float>(), 42.5F);
        REQUIRE_EQUAL(U32String{u32}.toFloat<double>(), 42.5);
    }

    void testImplDoubleCore() {
        const auto success = el::text::impl::parseDoubleCore(
            StringCharReader{StringEditor{"12.5"}}, FloatParseOptions::defaultOptions());
        REQUIRE_EQUAL(success.status, el::text::impl::FloatParseStatus::Success);
        REQUIRE_EQUAL(success.value, 12.5);

        const auto empty =
            el::text::impl::parseDoubleCore(StringCharReader{StringEditor{}}, FloatParseOptions::defaultOptions());
        REQUIRE_EQUAL(empty.status, el::text::impl::FloatParseStatus::ParseError);
        REQUIRE_EQUAL(empty.message, std::string_view{"Floating point text is empty"});

        const auto trailing = el::text::impl::parseDoubleCore(
            StringCharReader{StringEditor{"12.5m"}}, FloatParseOptions::defaultOptions());
        REQUIRE_EQUAL(trailing.status, el::text::impl::FloatParseStatus::ParseError);
        REQUIRE_EQUAL(trailing.message, std::string_view{"Floating point text has trailing characters"});

        auto ignoreTrailing = FloatParseOptions{};
        ignoreTrailing.setFlags(FloatParseFlag::IgnoreTrailingChars);
        const auto acceptedTrailing =
            el::text::impl::parseDoubleCore(StringCharReader{StringEditor{"12.5m"}}, ignoreTrailing);
        REQUIRE_EQUAL(acceptedTrailing.status, el::text::impl::FloatParseStatus::Success);
        REQUIRE_EQUAL(acceptedTrailing.value, 12.5);
    }

    void testImplStyleRestrictions() {
        auto fixed = FloatParseOptions{};
        fixed.setStyle(FloatParseOptions::Style::Fixed);
        const auto fixedResult = el::text::impl::parseDoubleCore(StringCharReader{StringEditor{"1.25e2"}}, fixed);
        REQUIRE_EQUAL(fixedResult.status, el::text::impl::FloatParseStatus::ParseError);

        auto scientific = FloatParseOptions{};
        scientific.setStyle(FloatParseOptions::Style::Scientific);
        const auto scientificResult =
            el::text::impl::parseDoubleCore(StringCharReader{StringEditor{"12.5"}}, scientific);
        REQUIRE_EQUAL(scientificResult.status, el::text::impl::FloatParseStatus::ParseError);
    }

    void testDefaultAndThrowingParseErrors() {
        REQUIRE_EQUAL(StringEditor{"abc"}.toFloat<double>(4.5), 4.5);

        try {
            static_cast<void>(StringEditor{"abc"}.toFloatOrThrow<double>());
            REQUIRE(false);
        } catch (const ParseError &) {
            REQUIRE(true);
        }
    }

    void testTrailingCharacters() {
        REQUIRE_EQUAL(StringEditor{"12.5m"}.toFloat<double>(-1.0), -1.0);

        auto options = FloatParseOptions{};
        options.setFlags(FloatParseFlag::IgnoreTrailingChars);
        REQUIRE_EQUAL(StringEditor{"12.5m"}.toFloat<double>(-1.0, options), 12.5);
        REQUIRE_EQUAL(StringEditor{"m12.5"}.toFloat<double>(-1.0, options), -1.0);
    }

    void testParseStyles() {
        auto fixed = FloatParseOptions{};
        fixed.setStyle(FloatParseOptions::Style::Fixed);
        REQUIRE_EQUAL(parse<double>("12.5", fixed), 12.5);
        REQUIRE_EQUAL(StringEditor{"1.25e2"}.toFloat<double>(-1.0, fixed), -1.0);

        auto scientific = FloatParseOptions{};
        scientific.setStyle(FloatParseOptions::Style::Scientific);
        REQUIRE_EQUAL(parse<double>("1.25e2", scientific), 125.0);
        REQUIRE_EQUAL(StringEditor{"12.5"}.toFloat<double>(-1.0, scientific), -1.0);

        auto hexadecimal = FloatParseOptions{};
        hexadecimal.setStyle(FloatParseOptions::Style::Hexadecimal);
        const auto text = StringEditor::fromFloat(12.0, FloatFormat::hexadecimal().setPrecision(ItemCount{2U}));
        REQUIRE_EQUAL(text.toFloat<double>(-1.0, hexadecimal), 12.0);
    }

    void testParseOverflowAndExceptionKinds() {
        REQUIRE_EQUAL(StringEditor{"1e100"}.toFloat<float>(-1.0F), -1.0F);
        REQUIRE_EQUAL(StringEditor{"1e100"}.toFloat<double>(-1.0), 1e100);
        REQUIRE_EQUAL(StringEditor{"1e9999"}.toFloat<double>(-1.0), -1.0);

        try {
            static_cast<void>(StringEditor{"1e100"}.toFloatOrThrow<float>());
            REQUIRE(false);
        } catch (const OverflowError &) {
            REQUIRE(true);
        }

        try {
            static_cast<void>(StringEditor{"12.5m"}.toFloatOrThrow<double>());
            REQUIRE(false);
        } catch (const ParseError &) {
            REQUIRE(true);
        }
    }

    void testFloatBoundaryRepresentability() {
        REQUIRE(el::text::impl::isFloatRepresentable(0.0));
        REQUIRE(el::text::impl::isFloatRepresentable(42.5));
        REQUIRE(el::text::impl::isFloatRepresentable(static_cast<double>(std::numeric_limits<float>::max())));
        REQUIRE(el::text::impl::isFloatRepresentable(static_cast<double>(std::numeric_limits<float>::denorm_min())));
        REQUIRE_FALSE(el::text::impl::isFloatRepresentable(1e100));
        REQUIRE_FALSE(
            el::text::impl::isFloatRepresentable(static_cast<double>(std::numeric_limits<float>::denorm_min()) / 2.0));
    }

    void testMalformedInputFailsAsParseError() {
        const auto invalidUtf8 = String{th::stdStringFromHex("31 C0 32")};
        REQUIRE_EQUAL(invalidUtf8.toFloat<double>(-1.0), -1.0);
        REQUIRE_THROWS(invalidUtf8.toFloatOrThrow<double>());

        const auto invalidUtf8Result =
            el::text::impl::parseDoubleCore(StringCharReader{invalidUtf8}, FloatParseOptions::defaultOptions());
        REQUIRE_EQUAL(invalidUtf8Result.status, el::text::impl::FloatParseStatus::ParseError);

        const auto invalidUtf16 = U16StringEditor{std::u16string{u'1', char16_t{0xD800U}, u'2'}};
        REQUIRE_EQUAL(invalidUtf16.toFloat<double>(-1.0), -1.0);
        REQUIRE_THROWS(invalidUtf16.toFloatOrThrow<double>());

        const auto invalidUtf16Result =
            el::text::impl::parseDoubleCore(StringCharReader{invalidUtf16}, FloatParseOptions::defaultOptions());
        REQUIRE_EQUAL(invalidUtf16Result.status, el::text::impl::FloatParseStatus::ParseError);

        const auto invalidUtf32 = U32StringEditor{std::u32string{U'1', char32_t{0x110000U}, U'2'}};
        REQUIRE_EQUAL(invalidUtf32.toFloat<double>(-1.0), -1.0);
        REQUIRE_THROWS(invalidUtf32.toFloatOrThrow<double>());
    }

    void testLibcppStrtodFallbackHelpers() {
#if defined(_LIBCPP_VERSION)
        REQUIRE(el::text::impl::hasHexFloatPrefix("0x1.8p3"));
        REQUIRE(el::text::impl::hasHexFloatPrefix("-0X1.8p3"));
        REQUIRE_FALSE(el::text::impl::hasHexFloatPrefix("1.8p3"));
        REQUIRE_EQUAL(el::text::impl::addHexFloatPrefix("1.8p3"), std::string{"0x1.8p3"});
        REQUIRE_EQUAL(el::text::impl::addHexFloatPrefix("-1.8p3"), std::string{"-0x1.8p3"});
        REQUIRE_EQUAL(el::text::impl::removeAddedHexPrefixFromLength(7U, false), 5U);
        REQUIRE_EQUAL(el::text::impl::removeAddedHexPrefixFromLength(8U, true), 6U);

        auto hexadecimal = FloatParseOptions{};
        hexadecimal.setStyle(FloatParseOptions::Style::Hexadecimal);
        const auto hexResult = el::text::impl::parseDoubleWithStrtod("1.8p3", hexadecimal);
        REQUIRE_EQUAL(hexResult.error, std::errc{});
        REQUIRE_EQUAL(hexResult.parsedLength, 5U);
        REQUIRE_EQUAL(hexResult.value, 12.0);

        auto fixed = FloatParseOptions{};
        fixed.setStyle(FloatParseOptions::Style::Fixed);
        const auto fixedResult = el::text::impl::parseDoubleWithStrtod("1.25e2", fixed);
        REQUIRE_EQUAL(fixedResult.error, std::errc{});
        REQUIRE_EQUAL(fixedResult.parsedLength, 4U);
        REQUIRE_EQUAL(fixedResult.value, 1.25);
#endif
    }

private:
    template <el::text::impl::AnyFloatType T>
    [[nodiscard]] static auto parse(
        std::string_view text, FloatParseOptions options = FloatParseOptions::defaultOptions()) -> T {
        return StringEditor{text}.toFloatOrThrow<T>(options);
    }
};
