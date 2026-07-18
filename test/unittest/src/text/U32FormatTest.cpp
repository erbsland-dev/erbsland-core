// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/AnyStringBuilder.hpp>
#include <erbsland/text/FormatError.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u32/U32Format.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

using el::unit::ArgumentCount;
using namespace el::text;

TESTED_TARGETS(U32Format FormatError)
class U32FormatTest final : public el::UnitTest {
public:
    void testStaticTextAndAutomaticIntegers() {
        const auto isoDateTime = U32Format{U"{:02}-{:02}-{:04} {:02}:{:02}:{:02}"};

        REQUIRE_EQUAL(isoDateTime.fieldCount(), ArgumentCount{6U});
        REQUIRE_EQUAL(
            StringConverter{isoDateTime.build(17, 5, 2026, 9, 18, 21)}.toStdU32String(),
            std::u32string{U"17-05-2026 09:18:21"});
    }

    void testViewPatternAndTextArguments() {
        const auto patternText = U32StringEditor{std::u32string_view{U"{}|{}|{}"}};
        const auto format = U32Format{U32String{patternText}};

        REQUIRE_EQUAL(
            StringConverter{format.build("u8", u"u16", U"u32")}.toStdU32String(), std::u32string{U"u8|u16|u32"});
    }

    void testAppendToNonUtf32Builder() {
        const auto format = U32Format{U"[{}:{}]"};
        auto builder = AnyStringBuilder{StringKind::U8};

        format.appendTo(builder, "count", 7);

        REQUIRE_EQUAL(StringConverter{builder.toU8String()}.toStdString(), std::string{"[count:7]"});
    }

    void testSharedFormatSpecFeatures() {
        const auto format = U32Format{U"{:^7}|{:+d}|{:.2f}|{:/html}"};

        REQUIRE_EQUAL(
            StringConverter{format.build("cat", 42, 1.25, "<")}.toStdU32String(),
            std::u32string{U"  cat  |+42|1.25|&lt;"});
    }

    void testInvalidPatternSyntax() {
        REQUIRE_THROWS(U32Format{U"{"});
        REQUIRE_THROWS(U32Format{U"{:q}"});
    }
};
