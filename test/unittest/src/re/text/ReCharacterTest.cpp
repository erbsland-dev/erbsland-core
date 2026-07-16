// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/impl/text/Character.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using namespace el::text::literals;
using namespace impl;
using namespace re_test::string_helper;

TESTED_TARGETS(Character)
TAGS(Text)
class ReCharacterTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    void testAppendUtf8() {
        el::text::String result;
        result.append(el::text::Char{U'A'});
        result.append(el::text::Char{0x00DFU});
        result.append(el::text::Char{0x20ACU});
        result.append(el::text::Char{0x1F600U});
        REQUIRE_EQUAL(
            result,
            el::text::String{bytesToStdString({0x41U, 0xC3U, 0x9FU, 0xE2U, 0x82U, 0xACU, 0xF0U, 0x9FU, 0x98U, 0x80U})});

        result.append(el::text::Char::endOfData());
        REQUIRE_EQUAL(result.length(), el::unit::ByteLength{10U});
    }

    void testAppendUtf8ToU8String() {
        el::text::String result;
        result.append(el::text::Char{U'A'});
        result.append(el::text::Char{0x1F600U});
        REQUIRE_EQUAL(result, el::text::String{bytesToU8String({0x41U, 0xF0U, 0x9FU, 0x98U, 0x80U})});
    }

    void testSafeString() {
        el::text::String result;
        appendToSafeString(result, el::text::Char{U'a'});
        appendToSafeString(result, el::text::Char{U'"'});
        appendToSafeString(result, el::text::Char{U'\\'});
        appendToSafeString(result, el::text::Char{U'\t'});
        REQUIRE_EQUAL(result, "a\\\"\\\\\\u0009"_el);

        appendToSafeString(result, el::text::Char::endOfData());
        REQUIRE_EQUAL(result, "a\\\"\\\\\\u0009"_el);
    }

    void testCharacterRangeString() {
        el::text::String result;
        appendToCharRangeString(result, el::text::Char{U'a'});
        appendToCharRangeString(result, el::text::Char{U' '});
        appendToCharRangeString(result, el::text::Char{U'-'});
        appendToCharRangeString(result, el::text::Char{U'\\'});
        appendToCharRangeString(result, el::text::Char{0x1F600U});
        REQUIRE_EQUAL(result, "a\\u0020\\u002D\\\\\\u{1F600}"_el);
    }

    void testRegexPredicates() {
        REQUIRE(isRepetitionStart(el::text::Char{U'{'}));
        REQUIRE(isGroupFlag(el::text::Char{U'i'}));
    }

    void testComparison() {
        REQUIRE(compareCharacters(el::text::Char{U'A'}, el::text::Char{U'a'}, false) == std::strong_ordering::less);
        REQUIRE(compareCharacters(el::text::Char{U'A'}, el::text::Char{U'a'}, true) == std::strong_ordering::equal);
        REQUIRE(
            compareCharacters(el::text::Char{0x03A3U}, el::text::Char{0x03C2U}, true) == std::strong_ordering::equal);
    }
};
