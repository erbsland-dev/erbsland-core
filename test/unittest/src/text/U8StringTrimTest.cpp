// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

using namespace el::text::literals;

using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;
using namespace el::text;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(StringEditor String U8StringTrimTools)
class U8StringTrimTest final : public el::UnitTest {
public:
    void testStringInPlaceTrim() {

        auto text = StringEditor{" \tvalue\r\n"_el};
        REQUIRE_EQUAL(text.trim(), "value"_el);

        text = StringEditor{"***value**"_el};
        REQUIRE_EQUAL(text.trim(CharSet{"*"_el}), "value"_el);

        text = StringEditor{" \tvalue\r\n"_el};
        REQUIRE_EQUAL(text.trim({}, StringSide::Front), "value\r\n"_el);

        text = StringEditor{"***value**"_el};
        REQUIRE_EQUAL(text.trim(CharSet{"*"_el}, StringSide::Front), "value**"_el);

        text = StringEditor{" \tvalue\r\n"_el};
        REQUIRE_EQUAL(text.trim({}, StringSide::Back), " \tvalue"_el);

        text = StringEditor{"***value**"_el};
        REQUIRE_EQUAL(text.trim(CharSet{"*"_el}, StringSide::Back), "***value"_el);
    }

    void testStringCopyTrim() {

        const auto text = StringEditor{"xxValueXX"_el};

        REQUIRE_EQUAL(text.trimmed(CharSet{"xX"_el}), "Value"_el);
        REQUIRE_EQUAL(text.trimmed(CharSet{"xX"_el}, StringSide::Front), "ValueXX"_el);
        REQUIRE_EQUAL(text.trimmed(CharSet{"xX"_el}, StringSide::Back), "xxValue"_el);
        REQUIRE_EQUAL(text, "xxValueXX"_el);
    }

    void testViewTrim() {

        const auto text = StringEditor{" \tvalue\r\n"_el};
        const auto view = String{text};

        REQUIRE_EQUAL(view.trimmed(), "value"_el);
        REQUIRE_EQUAL(view.trimmed({}, StringSide::Front), "value\r\n"_el);
        REQUIRE_EQUAL(view.trimmed({}, StringSide::Back), " \tvalue"_el);
        REQUIRE_EQUAL(view.trimmed(CharSet{" \n\r\t"_el}), "value"_el);
        REQUIRE_EQUAL(view.trimmed(), "value"_el);
    }

    void testCodePointTrim() {

        const auto text = StringEditor{"***value**"_el};
        const auto view = text;

        REQUIRE_EQUAL(StringConverter{view.trimmed(CharSet{"*"_el})}.toStdString(), "value");
        REQUIRE_EQUAL(StringConverter{view.trimmed(CharSet{"*"_el}, StringSide::Front)}.toStdString(), "value**");
        REQUIRE_EQUAL(StringConverter{view.trimmed(CharSet{"*"_el}, StringSide::Back)}.toStdString(), "***value");
        REQUIRE_EQUAL(StringConverter{view.trimmed(CharSet{"*"_el})}.toStdString(), "value");
    }

    void testEmptyNoOpAndAllTrimmed() {

        const auto empty = StringEditor{};
        REQUIRE(empty.trimmed().isEmpty());
        REQUIRE(empty.trimmed(CharSet{"*"_el}).isEmpty());

        const auto noOp = StringEditor{"value"_el};
        REQUIRE_EQUAL(noOp.trimmed(), "value"_el);
        REQUIRE_EQUAL(noOp.trimmed(CharSet{"*"_el}), "value"_el);

        auto allWhitespace = StringEditor{" \t\r\n"_el};
        REQUIRE(allWhitespace.trim().isEmpty());

        const auto allCustom = StringEditor{"****"_el};
        REQUIRE(allCustom.trimmed(CharSet{"*"_el}).isEmpty());
        REQUIRE(allCustom.trimmed(CharSet{"*"_el}).isEmpty());
    }

    void testNestedViewTrimKeepsCorrectOrigin() {

        const auto text = StringEditor{"xx--value--yy"_el};
        const auto first = String{text}.slice(ByteRange{ByteIndex{1U}, ByteLength{11U}});
        const auto second = first.slice(ByteRange{ByteIndex{1U}, ByteLength{9U}});

        REQUIRE_EQUAL(first, "x--value--y"_el);
        REQUIRE_EQUAL(second, "--value--"_el);
        REQUIRE_EQUAL(second.trimmed(CharSet{"-"_el}), "value"_el);
        REQUIRE_EQUAL(StringConverter{second.trimmed(CharSet{"-"_el})}.toStdString(), "value");
    }

    void testMultibyteBoundaryCharacters() {

        const auto text = String{"¢€data€¢"_el};
        const auto characters = CharSet{u8"¢€"_el};

        REQUIRE_EQUAL(text.trimmed(characters), "data"_el);
        REQUIRE_EQUAL(String{text}.trimmed(characters), "data"_el);
        REQUIRE_EQUAL(StringConverter{text.trimmed(characters)}.toStdString(), "data");
    }

    void testMalformedUtf8ReplacementTrim() {
        const auto bytes = th::stdStringFromHex("C0 41 C0");
        const auto text = StringEditor{std::string_view{bytes}};
        const auto characters = CharSet{Char::replacement()};

        REQUIRE_EQUAL(text.trimmed(characters), "A"_el);
        REQUIRE_EQUAL(String{text}.trimmed(characters), "A"_el);
        REQUIRE_EQUAL(StringConverter{text.trimmed(characters)}.toStdString(), "A");
    }

    void testCaseInsensitiveTrim() {

        auto text = StringEditor{"XXvalueX"_el};
        REQUIRE_EQUAL(text.trim(CharSet{"xX"_el}), "value"_el);

        const auto unicodeText = String{"\u00C4value\u00E4"_el};
        const auto unicodeCharacters = CharSet{u8"\u00C4\u00E4"_el};
        REQUIRE_EQUAL(unicodeText.trimmed(unicodeCharacters), "value"_el);
        REQUIRE_EQUAL(String{unicodeText}.trimmed(unicodeCharacters), "value"_el);
        REQUIRE_EQUAL(StringConverter{unicodeText.trimmed(unicodeCharacters)}.toStdString(), "value");
    }
};
