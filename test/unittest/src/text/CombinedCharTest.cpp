// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CombinedChar.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <functional>
#include <stdexcept>

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(CombinedChar)
class CombinedCharTest final : public el::UnitTest {
public:
    void testDefaultAndSingleCharacter() {
        const auto empty = CombinedChar{};
        const auto symbol = CombinedChar{U'界'};

        REQUIRE(empty.isEmpty());
        REQUIRE_EQUAL(empty.first(), Char{});
        REQUIRE_EQUAL(empty.singleOrNull(), Char{});
        REQUIRE_EQUAL(empty.characterCount(), CpLength::zero());
        REQUIRE_EQUAL(empty.byteCount(), ByteLength::zero());
        REQUIRE_EQUAL(empty.displayWidth(), 0);

        REQUIRE_FALSE(symbol.isEmpty());
        REQUIRE_EQUAL(symbol.first(), U'界');
        REQUIRE_EQUAL(symbol.singleOrNull(), U'界');
        REQUIRE_EQUAL(symbol.characters(), (CombinedChar::Storage{U'界', 0, 0}));
        REQUIRE_EQUAL(symbol.characterCount(), CpLength{1});
        REQUIRE_EQUAL(symbol.byteCount(), ByteLength{3});
        REQUIRE_EQUAL(symbol.displayWidth(), 2);
        REQUIRE_EQUAL(symbol.toU32String(), U"界"_el);
    }

    void testConstructionFromUtf8AndUtf32Text() {
        const auto utf8 = StringEditor{std::string_view{th::stdStringFromHex("65 CC 81")}};
        const auto fromUtf8 = CombinedChar{utf8};
        const auto fromUtf32 = CombinedChar{U"e\u0301"_el};

        REQUIRE_EQUAL(fromUtf8, fromUtf32);
        REQUIRE_EQUAL(fromUtf8.first(), U'e');
        REQUIRE_EQUAL(fromUtf8.singleOrNull(), Char{});
        REQUIRE_EQUAL(fromUtf8.characters(), (CombinedChar::Storage{U'e', U'\u0301', 0}));
        REQUIRE_EQUAL(fromUtf8.characterCount(), CpLength{2});
        REQUIRE_EQUAL(fromUtf8.byteCount(), ByteLength{3});
        REQUIRE_EQUAL(fromUtf8.displayWidth(), 1);
        REQUIRE_EQUAL(fromUtf8.toString(), utf8);
        REQUIRE_EQUAL(fromUtf8.toU32String(), U"e\u0301"_el);
    }

    void testWithCombiningAppendsZeroWidthCharacters() {
        const auto combined = CombinedChar{U'e'}.withCombining(U'\u0301').withCombining(U'\u0302');

        REQUIRE_EQUAL(combined.characters(), (CombinedChar::Storage{U'e', U'\u0301', U'\u0302'}));
        REQUIRE_EQUAL(combined.characterCount(), CpLength{3});
        REQUIRE_EQUAL(combined.singleOrNull(), Char{});
        REQUIRE_EQUAL(combined.displayWidth(), 1);
    }

    void testWithCombiningRejectsUnsupportedCharacters() {
        const auto full = CombinedChar{U"e\u0301\u0302"_el};

        REQUIRE_EQUAL(CombinedChar{}.withCombining(U'\u0301'), CombinedChar{});
        REQUIRE_EQUAL(CombinedChar{U'e'}.withCombining(U'x'), CombinedChar{U'e'});
        REQUIRE_EQUAL(CombinedChar{U'e'}.withCombining(U'\n'), CombinedChar{U'e'});
        REQUIRE_EQUAL(CombinedChar{U'e'}.withCombining(Char{0x110000U}), CombinedChar{U'e'});
        REQUIRE_EQUAL(full.withCombining(U'\u0303'), full);
    }

    void testFromStringNormalizesUnsupportedText() {
        constexpr auto invalidUtf32 = std::array<char32_t, 1>{0x110000U};

        REQUIRE_EQUAL(CombinedChar::fromString(""_el).first(), Char::replacement());
        REQUIRE_EQUAL(CombinedChar::fromString(U""_el).first(), Char::replacement());
        REQUIRE_EQUAL(CombinedChar::fromString(U"\u0301"_el).first(), Char::replacement());
        REQUIRE_EQUAL(CombinedChar::fromString(U"ab"_el).first(), Char::replacement());
        REQUIRE_EQUAL(CombinedChar::fromString("\n"_el).first(), Char::replacement());
        REQUIRE_EQUAL(
            CombinedChar::fromString(U32StringEditor{std::u32string_view{invalidUtf32.data(), invalidUtf32.size()}})
                .first(),
            Char::replacement());
        REQUIRE_EQUAL(
            CombinedChar::fromString(StringEditor{std::string_view{th::stdStringFromHex("C3")}}).first(),
            Char::replacement());
        REQUIRE_EQUAL(
            CombinedChar::fromString(U"a\u0301\u0302\u0303"_el).characters(),
            (CombinedChar::Storage{U'a', U'\u0301', U'\u0302'}));
    }

    void testHashAndEqualityUseAllStoredCharacters() {
        const auto base = CombinedChar{U'e'};
        const auto combined = CombinedChar{U"e\u0301"_el};
        const auto equalCombined = CombinedChar::fromString(U"e\u0301"_el);

        REQUIRE(base == U'e');
        REQUIRE_FALSE(combined == U'e');
        REQUIRE(combined == equalCombined);
        REQUIRE_NOT_EQUAL(base.hash(), combined.hash());
        REQUIRE_EQUAL(combined.hash(), std::hash<CombinedChar>{}(combined));
    }
};
