// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/EscapeAmount.hpp>
#include <erbsland/text/EscapeFormat.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/U16DataIndex.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

using namespace el::text::literals;

using el::unit::CpIndex;
using el::unit::U16DataIndex;
using namespace el::text;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(EscapeFormat EscapeAmount StringEditor String U16StringEditor U16String U32StringEditor U32String)
class StringEscapingTest final : public el::UnitTest {
public:
    void testEscapeFormatConversion() {

        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeFormat::None), 0U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeFormat::Html), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeFormat::Json), 2U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeFormat::Cpp), 3U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeFormat::Xml), 4U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeFormat::RegEx), 5U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeFormat::Display), 6U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeFormat::Config), 7U);

        REQUIRE_EQUAL(EscapeFormat{EscapeFormat::None}.toString(), "none"_el);
        REQUIRE_EQUAL(EscapeFormat{EscapeFormat::Html}.toString(), "html"_el);
        REQUIRE_EQUAL(EscapeFormat{EscapeFormat::Json}.toString(), "json"_el);
        REQUIRE_EQUAL(EscapeFormat{EscapeFormat::Cpp}.toString(), "cpp"_el);
        REQUIRE_EQUAL(EscapeFormat{EscapeFormat::Xml}.toString(), "xml"_el);
        REQUIRE_EQUAL(EscapeFormat{EscapeFormat::RegEx}.toString(), "regex"_el);
        REQUIRE_EQUAL(EscapeFormat{EscapeFormat::Display}.toString(), "display"_el);
        REQUIRE_EQUAL(EscapeFormat{EscapeFormat::Config}.toString(), "config"_el);

        REQUIRE_EQUAL(EscapeFormat::fromString("html"_el).value(), EscapeFormat::Html);
        REQUIRE_EQUAL(EscapeFormat::fromString("json"_el).value(), EscapeFormat::Json);
        REQUIRE_EQUAL(EscapeFormat::fromStringOrThrow("regex"_el), EscapeFormat::RegEx);
        REQUIRE_FALSE(EscapeFormat::fromString("pcre"_el).has_value());
        REQUIRE_EQUAL(EscapeFormat::fromStringOrThrow("display"_el), EscapeFormat::Display);
        REQUIRE_EQUAL(EscapeFormat::fromStringOrThrow("config"_el), EscapeFormat::Config);
        REQUIRE_FALSE(EscapeFormat::fromString("unknown"_el).has_value());
        REQUIRE_THROWS(EscapeFormat::fromStringOrThrow("unknown"_el));
    }

    void testEscapeAmountConversion() {

        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeAmount::Nothing), 0U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeAmount::Required), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeAmount::Balanced), 2U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeAmount::NonAscii), 3U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(EscapeAmount::Everything), 4U);

        REQUIRE_EQUAL(EscapeAmount{EscapeAmount::Nothing}.toString(), "nothing"_el);
        REQUIRE_EQUAL(EscapeAmount{EscapeAmount::Required}.toString(), "required"_el);
        REQUIRE_EQUAL(EscapeAmount{EscapeAmount::Balanced}.toString(), "balanced"_el);
        REQUIRE_EQUAL(EscapeAmount{EscapeAmount::NonAscii}.toString(), "non-ascii"_el);
        REQUIRE_EQUAL(EscapeAmount{EscapeAmount::Everything}.toString(), "all"_el);

        REQUIRE_EQUAL(EscapeAmount::fromString("nothing"_el).value(), EscapeAmount::Nothing);
        REQUIRE_EQUAL(EscapeAmount::fromString("balanced"_el).value(), EscapeAmount::Balanced);
        REQUIRE_EQUAL(EscapeAmount::fromStringOrThrow("all"_el), EscapeAmount::Everything);
        REQUIRE_FALSE(EscapeAmount::fromString("everything"_el).has_value());
        REQUIRE_THROWS(EscapeAmount::fromStringOrThrow("everything"_el));

        REQUIRE_EQUAL(EscapeAmount::fromSuffix(U'-').value(), EscapeAmount::Required);
        REQUIRE_EQUAL(EscapeAmount::fromSuffix(U'=').value(), EscapeAmount::Balanced);
        REQUIRE_EQUAL(EscapeAmount::fromSuffix(U'+').value(), EscapeAmount::NonAscii);
        REQUIRE_EQUAL(EscapeAmount::fromSuffix(U'*').value(), EscapeAmount::Everything);
        REQUIRE_FALSE(EscapeAmount::fromSuffix(U'!').has_value());
    }

    void testDisplayEscapingPreservesPunctuationAndEscapesControls() {

        auto text = el::text::StringEditor{"\"quoted\" \\ path"_el};
        text.append(U'\x1b').append(U'\n');
        REQUIRE_EQUAL(text.toEscaped(EscapeFormat::Display), "\"quoted\" \\ path\\u{1b}\\n"_el);
    }

    void testHtmlAndXmlTargets() {
        const auto text = StringEditor{"<tag attr=\"x\">&'</tag>"_el};

        const auto htmlRequired = text.toEscaped(EscapeFormat::Html, EscapeAmount::Required);
        REQUIRE_EQUAL(
            StringConverter{htmlRequired}.toStdString(), std::string{"&lt;tag attr=\"x\"&gt;&amp;'&lt;/tag&gt;"});
        REQUIRE_EQUAL(htmlRequired.length(), text.escapedSize(EscapeFormat::Html, EscapeAmount::Required));

        const auto htmlBalanced = text.toEscaped(EscapeFormat::Html, EscapeAmount::Balanced);
        REQUIRE_EQUAL(
            StringConverter{htmlBalanced}.toStdString(),
            std::string{"&lt;tag attr=&quot;x&quot;&gt;&amp;'&lt;/tag&gt;"});
        REQUIRE_EQUAL(htmlBalanced.length(), text.escapedSize(EscapeFormat::Html));

        const auto xmlBalanced = text.toEscaped(EscapeFormat::Xml, EscapeAmount::Balanced);
        REQUIRE_EQUAL(
            StringConverter{xmlBalanced}.toStdString(),
            std::string{"&lt;tag attr=&quot;x&quot;&gt;&amp;&apos;&lt;/tag&gt;"});
        REQUIRE_EQUAL(xmlBalanced.length(), text.escapedSize(EscapeFormat::Xml));
    }

    void testJsonCppAndRegExTargets() {
        const auto jsonText = String{"\"\\\n😀"_el};
        const auto jsonEscaped = jsonText.toEscaped(EscapeFormat::Json, EscapeAmount::NonAscii);
        REQUIRE_EQUAL(StringConverter{jsonEscaped}.toStdString(), std::string{"\\\"\\\\\\n\\uD83D\\uDE00"});
        REQUIRE_EQUAL(jsonEscaped.length(), jsonText.escapedSize(EscapeFormat::Json, EscapeAmount::NonAscii));

        const auto cppText = String{"A\né😀"_el};
        const auto cppEscaped = cppText.toEscaped(EscapeFormat::Cpp, EscapeAmount::NonAscii);
        REQUIRE_EQUAL(StringConverter{cppEscaped}.toStdString(), std::string{"A\\n\\u00E9\\U0001F600"});
        REQUIRE_EQUAL(cppEscaped.length(), cppText.escapedSize(EscapeFormat::Cpp, EscapeAmount::NonAscii));

        const auto regExText = StringEditor{"a+b*(c)"_el};
        const auto regExEscaped = regExText.toEscaped(EscapeFormat::RegEx, EscapeAmount::Required);
        REQUIRE_EQUAL(StringConverter{regExEscaped}.toStdString(), std::string{"a\\+b\\*\\(c\\)"});
        REQUIRE_EQUAL(regExEscaped.length(), regExText.escapedSize(EscapeFormat::RegEx, EscapeAmount::Required));
    }

    void testConfigTarget() {
        const auto u8Text = String{"A\\\"$\n\r\t\u0001é"_el};
        const auto u8Escaped = u8Text.toEscaped(EscapeFormat::Config, EscapeAmount::Required);
        REQUIRE_EQUAL(StringConverter{u8Escaped}.toStdString(), std::string{"A\\\\\\\"\\$\\n\\r\\t\\u{1}é"});
        REQUIRE_EQUAL(u8Escaped.length(), u8Text.escapedSize(EscapeFormat::Config, EscapeAmount::Required));

        const auto u16Text = U16StringEditor{std::u16string_view{u"A\\\"$\n\r\t\u0001é"}};
        const auto u16Escaped = u16Text.toEscaped(EscapeFormat::Config, EscapeAmount::Required);
        REQUIRE_EQUAL(StringConverter{u16Escaped}.toStdU16String(), std::u16string{u"A\\\\\\\"\\$\\n\\r\\t\\u{1}é"});
        REQUIRE_EQUAL(u16Escaped.length(), u16Text.escapedSize(EscapeFormat::Config, EscapeAmount::Required));

        const auto u32Text = U32StringEditor{std::u32string_view{U"A\\\"$\n\r\t\u0001é"}};
        const auto u32Escaped = u32Text.toEscaped(EscapeFormat::Config, EscapeAmount::Required);
        REQUIRE_EQUAL(StringConverter{u32Escaped}.toStdU32String(), std::u32string{U"A\\\\\\\"\\$\\n\\r\\t\\u{1}é"});
        REQUIRE_EQUAL(u32Escaped.length(), u32Text.escapedSize(EscapeFormat::Config, EscapeAmount::Required));
    }

    void testEscapeAmounts() {
        const auto text = String{"A\né"_el};

        REQUIRE_EQUAL(
            StringConverter{text.toEscaped(EscapeFormat::Json, EscapeAmount::Nothing)}.toStdString(),
            std::string{"A\né"});
        REQUIRE_EQUAL(
            StringConverter{text.toEscaped(EscapeFormat::None, EscapeAmount::Everything)}.toStdString(),
            std::string{"A\né"});

        REQUIRE_EQUAL(
            StringConverter{text.toEscaped(EscapeFormat::Json, EscapeAmount::Required)}.toStdString(),
            std::string{"A\\né"});
        REQUIRE_EQUAL(
            StringConverter{text.toEscaped(EscapeFormat::Json, EscapeAmount::NonAscii)}.toStdString(),
            std::string{"A\\n\\u00E9"});
        REQUIRE_EQUAL(
            StringConverter{text.toEscaped(EscapeFormat::Json, EscapeAmount::Everything)}.toStdString(),
            std::string{"\\u0041\\n\\u00E9"});

        const auto formatText = String{"A‍B"_el};
        REQUIRE_EQUAL(
            StringConverter{formatText.toEscaped(EscapeFormat::Json)}.toStdString(), std::string{"A\\u200DB"});
    }

    void testNativeStringAndViewApis() {
        const auto u8Text = StringEditor{"<&>"_el};
        const auto u8View = String{u8Text};
        const auto u8Result = u8View.toEscaped(EscapeFormat::Html);
        REQUIRE_EQUAL(StringConverter{u8Result}.toStdString(), std::string{"&lt;&amp;&gt;"});
        REQUIRE_EQUAL(u8Result.length(), u8View.escapedSize(EscapeFormat::Html));

        const auto u16Text = U16StringEditor{std::u16string_view{u"<>&é😀"}};
        const auto u16Result = u16Text.toEscaped(EscapeFormat::Html, EscapeAmount::NonAscii);
        REQUIRE_EQUAL(StringConverter{u16Result}.toStdU16String(), std::u16string{u"&lt;&gt;&amp;&#233;&#128512;"});
        REQUIRE_EQUAL(u16Result.length(), u16Text.escapedSize(EscapeFormat::Html, EscapeAmount::NonAscii));

        const auto u16View = U16String{u16Text};
        const auto u16ViewResult = u16View.toEscaped(EscapeFormat::Xml);
        REQUIRE_EQUAL(StringConverter{u16ViewResult}.toStdU16String(), std::u16string{u"&lt;&gt;&amp;é😀"});
        REQUIRE_EQUAL(u16ViewResult.length(), u16View.escapedSize(EscapeFormat::Xml));

        const auto u32Text = U32StringEditor{std::u32string_view{U"[x]é"}};
        const auto u32Result = u32Text.toEscaped(EscapeFormat::RegEx, EscapeAmount::NonAscii);
        REQUIRE_EQUAL(StringConverter{u32Result}.toStdU32String(), std::u32string{U"\\[x\\]\\x{E9}"});
        REQUIRE_EQUAL(u32Result.length(), u32Text.escapedSize(EscapeFormat::RegEx, EscapeAmount::NonAscii));

        const auto u32View = U32String{u32Text};
        const auto u32ViewResult = u32View.toEscaped(EscapeFormat::RegEx, EscapeAmount::Required);
        REQUIRE_EQUAL(StringConverter{u32ViewResult}.toStdU32String(), std::u32string{U"\\[x\\]é"});
        REQUIRE_EQUAL(u32ViewResult.length(), u32View.escapedSize(EscapeFormat::RegEx, EscapeAmount::Required));
    }

    void testMalformedInputHandling() {
        const auto invalidUtf8 = String{th::stdStringFromHex("41 C0 42")};
        const auto escapedUtf8 = invalidUtf8.toEscaped(EscapeFormat::Json, EscapeAmount::NonAscii);
        REQUIRE_EQUAL(StringConverter{escapedUtf8}.toStdString(), std::string{"A\\uFFFDB"});

        const auto invalidUtf16 = U16StringEditor{std::u16string{u'A', char16_t{0xD800U}, u'B'}};
        const auto escapedUtf16 = invalidUtf16.toEscaped(EscapeFormat::Json, EscapeAmount::NonAscii);
        REQUIRE_EQUAL(StringConverter{escapedUtf16}.toStdU16String(), std::u16string{u"A\\uFFFDB"});

        const auto invalidUtf32 = U32StringEditor{std::u32string{U'A', char32_t{0x110000U}, U'B'}};
        const auto escapedUtf32 = invalidUtf32.toEscaped(EscapeFormat::Json, EscapeAmount::NonAscii);
        REQUIRE_EQUAL(StringConverter{escapedUtf32}.toStdU32String(), std::u32string{U"A\\uFFFDB"});

        const auto configUtf8 = invalidUtf8.toEscaped(EscapeFormat::Config, EscapeAmount::Required);
        REQUIRE_EQUAL(StringConverter{configUtf8}.toStdString(), std::string{"A\\u{fffd}B"});
        REQUIRE_EQUAL(configUtf8.length(), invalidUtf8.escapedSize(EscapeFormat::Config, EscapeAmount::Required));
        const auto configUtf16 = invalidUtf16.toEscaped(EscapeFormat::Config, EscapeAmount::Required);
        REQUIRE_EQUAL(StringConverter{configUtf16}.toStdU16String(), std::u16string{u"A\\u{fffd}B"});
        REQUIRE_EQUAL(configUtf16.length(), invalidUtf16.escapedSize(EscapeFormat::Config, EscapeAmount::Required));
        const auto configUtf32 = invalidUtf32.toEscaped(EscapeFormat::Config, EscapeAmount::Required);
        REQUIRE_EQUAL(StringConverter{configUtf32}.toStdU32String(), std::u32string{U"A\\u{fffd}B"});
        REQUIRE_EQUAL(configUtf32.length(), invalidUtf32.escapedSize(EscapeFormat::Config, EscapeAmount::Required));
    }

    void testNoEscapePreservesNativeInvalidData() {
        const auto invalidUtf8 = String{th::stdStringFromHex("41 C0 42")};
        const auto unchangedUtf8 = invalidUtf8.toEscaped(EscapeFormat::Json, EscapeAmount::Nothing);
        REQUIRE_FALSE(unchangedUtf8.isValidUtf8());
        REQUIRE_EQUAL(unchangedUtf8.length(), invalidUtf8.length());

        const auto invalidUtf16 = U16StringEditor{std::u16string{u'A', char16_t{0xD800U}, u'B'}};
        const auto unchangedUtf16 = invalidUtf16.toEscaped(EscapeFormat::Json, EscapeAmount::Nothing);
        REQUIRE_EQUAL(unchangedUtf16.length(), invalidUtf16.length());
        REQUIRE(unchangedUtf16.charAt(U16DataIndex{1U}).isReplacement());

        const auto invalidUtf32 = U32StringEditor{std::u32string{U'A', char32_t{0x110000U}, U'B'}};
        const auto unchangedUtf32 = invalidUtf32.toEscaped(EscapeFormat::None, EscapeAmount::Everything);
        REQUIRE_EQUAL(unchangedUtf32.length(), invalidUtf32.length());
        REQUIRE(unchangedUtf32.charAt(CpIndex{1U}).isReplacement());
    }
};
