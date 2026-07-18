// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/AnyStringBuilder.hpp>
#include <erbsland/text/FormatError.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8Format.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

using el::unit::ArgumentCount;
using namespace el::text;

TESTED_TARGETS(U8Format FormatError)
class U8FormatTest final : public el::UnitTest {
public:
    void testStaticTextAndAutomaticIntegers() {
        const auto isoDateTime = U8Format{"{:02}-{:02}-{:04} {:02}:{:02}:{:02}"};

        REQUIRE_EQUAL(isoDateTime.fieldCount(), ArgumentCount{6U});
        REQUIRE_EQUAL(
            StringConverter{isoDateTime.build(17, 5, 2026, 9, 18, 21)}.toStdString(),
            std::string{"17-05-2026 09:18:21"});
    }

    void testAppendToNonUtf8Builder() {
        const auto format = U8Format{"[{}:{}]"};
        auto builder = AnyStringBuilder{StringKind::U16};

        format.appendTo(builder, "count", 7);

        REQUIRE_EQUAL(StringConverter{builder.toU8String()}.toStdString(), std::string{"[count:7]"});
    }

    void testEscapedBraces() {
        REQUIRE_EQUAL(StringConverter{U8Format{"{{}}"}.build()}.toStdString(), std::string{"{}"});
        REQUIRE_EQUAL(StringConverter{U8Format{"{{{}}}"}.build("value")}.toStdString(), std::string{"{value}"});
    }

    void testTextArguments() {
        const auto u8Text = U8StringEditor{std::string_view{"u8"}};
        const auto u16Text = U16StringEditor{std::u16string_view{u"u16"}};
        const auto u32Text = U32StringEditor{std::u32string_view{U"u32"}};
        const auto stdText = std::string{"std"};
        const auto stdView = std::string_view{"view"};

        const auto format = U8Format{"{}|{}|{}|{}|{}|{}|{}|{}"};

        REQUIRE_EQUAL(
            StringConverter{
                format.build(
                    u8Text, U16String{u16Text}, u32Text, stdText, stdView, "literal", u"u16 literal", U"u32 literal")}
                .toStdString(),
            std::string{"u8|u16|u32|std|view|literal|u16 literal|u32 literal"});
    }

    void testEscapedTextArguments() {
        const auto u8Text = U8StringEditor{std::string_view{"<&>"}};
        const auto u16Text = U16StringEditor{std::u16string_view{u"\"x\""}};
        const auto u32Text = U32StringEditor{std::u32string_view{U"a+b"}};

        const auto format = U8Format{"{:/html}|{:/json}|{:/regex}"};
        const auto expected = StringConverter{u8Text.toEscaped(EscapeFormat::Html)}.toStdString() + "|" +
            StringConverter{u16Text.toEscaped(EscapeFormat::Json)}.toStdString() + "|" +
            StringConverter{u32Text.toEscaped(EscapeFormat::RegEx)}.toStdString();

        REQUIRE_EQUAL(StringConverter{format.build(u8Text, u16Text, u32Text)}.toStdString(), expected);
    }

    void testEscapedTextAmountSuffixes() {
        const auto text = U8StringEditor{std::u8string_view{u8"A\né"}};
        const auto format = U8Format{"{:/json}|{:/json-}|{:/json=}|{:/json+}|{:/json*}"};

        REQUIRE_EQUAL(
            StringConverter{format.build(text, text, text, text, text)}.toStdString(),
            std::string{"A\\né|A\\né|A\\né|A\\n\\u00E9|\\u0041\\n\\u00E9"});
    }

    void testIntegerFieldTypes() {
        const auto format = U8Format{"{:d}|{:x}|{:X}|{:b}|{:B}|{:o}|{:O}"};

        REQUIRE_EQUAL(
            StringConverter{format.build(26, 26, 26, 5, 5, 493, 493)}.toStdString(),
            std::string{"26|1a|1A|101|101|755|755"});
    }

    void testTextWidthAlignmentAndPrecision() {
        const auto format = U8Format{"{:<5}|{:>5}|{:^7}|{:0>5}|{:.3s}"};

        REQUIRE_EQUAL(
            StringConverter{format.build("cat", "cat", "cat", "cat", "abcdef")}.toStdString(),
            std::string{"cat  |  cat|  cat  |00cat|abc"});

        REQUIRE_THROWS(U8Format{"{:05s}"}.build("text"));
        REQUIRE_THROWS(U8Format{"{:*>5}"}.build("text"));
    }

    void testIntegerSignsAlternatePrecisionAndLayout() {
        const auto format = U8Format{"{:+d}|{: d}|{:#x}|{:.4d}|{:8.4d}|{:0>6d}|{:04}|{:+08d}|{:#08x}"};

        REQUIRE_EQUAL(
            StringConverter{format.build(42, 42, 42, 42, 42, 42, 42, 42, 42)}.toStdString(),
            std::string{"+42| 42|0x2a|0042|    0042|000042|0042|+0000042|0x00002a"});
    }

    void testFloatingPointPrecisionAndLayout() {
        const auto format = U8Format{"{:.2f}|{:+8.1f}|{: 8.1E}"};

        REQUIRE_EQUAL(
            StringConverter{format.build(12.345, 1.25, 1.25)}.toStdString(), std::string{"12.35|    +1.2| 1.2E+00"});
    }

    void testEscapedTextWidthAlignmentAndPrecision() {
        const auto format = U8Format{"{:>12/html}|{:.2/html}|{:^14/json+}"};

        REQUIRE_EQUAL(
            StringConverter{format.build("<p>", "<p>&", "A\né")}.toStdString(),
            std::string{"   &lt;p&gt;|&lt;p|  A\\n\\u00E9   "});
    }

    void testStrictArgumentRules() {
        REQUIRE_THROWS(U8Format{"{}{}"}.build(1));
        REQUIRE_THROWS(U8Format{"{}"}.build(1, 2));
        REQUIRE_THROWS(U8Format{"{} {0}"});
        REQUIRE_THROWS(U8Format{"{1}"});

        REQUIRE_EQUAL(StringConverter{U8Format{"{0}/{0}"}.build("same")}.toStdString(), std::string{"same/same"});
    }

    void testInvalidPatternSyntax() {
        REQUIRE_THROWS(U8Format{"{"});
        REQUIRE_THROWS(U8Format{"}"});
        REQUIRE_THROWS(U8Format{"{/html}"});
        REQUIRE_THROWS(U8Format{"{:/unknown}"});
        REQUIRE_THROWS(U8Format{"{:/json!}"});
        REQUIRE_THROWS(U8Format{"{:/json--}"});
        REQUIRE_THROWS(U8Format{"{:q}"});
        REQUIRE_THROWS(U8Format{"{:é}"});
    }
};
