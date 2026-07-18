// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/Match.hpp>
#include <erbsland/re/Match16.hpp>
#include <erbsland/re/Match32.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/text/EncodingError.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Unicode)
class RegExNullCharacterTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
private:
    template <typename StringType>
    [[nodiscard]] static auto exactText() -> StringType {
        StringType result;
        result.append(el::text::Char{U'A'});
        result.append(el::text::Char{U'\0'});
        result.append(el::text::Char{U'B'});
        return result;
    }

    template <typename StringType>
    [[nodiscard]] static auto searchText() -> StringType {
        StringType result;
        result.append(el::text::Char{U'X'});
        result.append(el::text::Char{U'A'});
        result.append(el::text::Char{U'\0'});
        result.append(el::text::Char{U'B'});
        result.append(el::text::Char{U'Y'});
        return result;
    }

    template <typename StringType>
    void requireAllMatchingApis(const RegExPtr &regEx) {
        const auto exact = exactText<StringType>();
        const auto searchable = searchText<StringType>();
        const auto match = regEx->match(exact);
        REQUIRE(match != nullptr);
        REQUIRE_EQUAL(match->begin(), 0U);
        REQUIRE_EQUAL(match->end(), 3U);
        REQUIRE_EQUAL(match->begin(1), 1U);
        REQUIRE_EQUAL(match->end(1), 2U);

        const auto fullMatch = regEx->fullMatch(exact);
        REQUIRE(fullMatch != nullptr);
        REQUIRE_EQUAL(fullMatch->begin(), 0U);
        REQUIRE_EQUAL(fullMatch->end(), 3U);

        const auto findFirst = regEx->findFirst(searchable);
        REQUIRE(findFirst != nullptr);
        REQUIRE_EQUAL(findFirst->begin(), 1U);
        REQUIRE_EQUAL(findFirst->end(), 4U);
        REQUIRE_EQUAL(findFirst->begin(1), 2U);
        REQUIRE_EQUAL(findFirst->end(1), 3U);

        REQUIRE(regEx->fullMatch(StringType{}) == nullptr);
    }

    template <typename StringType>
    void requireFullMatch(const RegExPtr &regEx) {
        REQUIRE(regEx->fullMatch(exactText<StringType>()) != nullptr);
    }

    void requireFullMatchAllWidths(const RegExPtr &regEx) {
        WITH_CONTEXT(requireFullMatch<StringEditor>(regEx));
        WITH_CONTEXT(requireFullMatch<el::text::U16StringEditor>(regEx));
        WITH_CONTEXT(requireFullMatch<el::text::U32StringEditor>(regEx));
    }

    template <typename StringType>
    void requireSingleNullFullMatch(const RegExPtr &regEx) {
        const auto nullText = StringType::fromCharacter(el::text::Char{U'\0'});
        REQUIRE(regEx->fullMatch(nullText) != nullptr);
        REQUIRE(regEx->fullMatch(StringType{}) == nullptr);
    }

public:
    void testNullPatternsAreRejectedByDefault() {
        const auto nullPattern = String::fromCharacter(el::text::Char{U'\0'});
        REQUIRE_THROWS_AS(RegExError, RegEx::compile(String{nullPattern}));
        REQUIRE_THROWS_AS(RegExError, RegEx::compile("\\x00"_el));
        REQUIRE_THROWS_AS(RegExError, RegEx::compile("\\u0000"_el));
        REQUIRE_THROWS_AS(RegExError, RegEx::compile("[\\x{0}]"_el));
    }

    void testNullPatternsCanBeExplicitlyEnabled() {
        auto settings = Settings{};
        settings.enableFeature(Feature::AcceptNullInPattern);
        const auto nullPattern = String::fromCharacter(el::text::Char{U'\0'});
        const auto raw = RegEx::compile(String{nullPattern}, {}, settings);
        const auto xEscape = RegEx::compile("\\x00"_el, {}, settings);
        const auto uEscape = RegEx::compile("\\u0000"_el, {}, settings);
        WITH_CONTEXT(requireSingleNullFullMatch<StringEditor>(raw));
        WITH_CONTEXT(requireSingleNullFullMatch<el::text::U16StringEditor>(raw));
        WITH_CONTEXT(requireSingleNullFullMatch<el::text::U32StringEditor>(raw));
        WITH_CONTEXT(requireSingleNullFullMatch<StringEditor>(xEscape));
        WITH_CONTEXT(requireSingleNullFullMatch<el::text::U16StringEditor>(xEscape));
        WITH_CONTEXT(requireSingleNullFullMatch<el::text::U32StringEditor>(xEscape));
        WITH_CONTEXT(requireSingleNullFullMatch<StringEditor>(uEscape));
        WITH_CONTEXT(requireSingleNullFullMatch<el::text::U16StringEditor>(uEscape));
        WITH_CONTEXT(requireSingleNullFullMatch<el::text::U32StringEditor>(uEscape));
    }

    void testMatchCaptureFullMatchFindAndEmptyInput() {
        auto settings = Settings{};
        settings.enableFeature(Feature::AcceptNullInPattern);
        const auto regEx = RegEx::compile("A(\\x00)B"_el, {}, settings);
        WITH_CONTEXT(requireAllMatchingApis<StringEditor>(regEx));
        WITH_CONTEXT(requireAllMatchingApis<el::text::U16StringEditor>(regEx));
        WITH_CONTEXT(requireAllMatchingApis<el::text::U32StringEditor>(regEx));
    }

    void testDotClassCategoryAndAnchors() {
        auto settings = Settings{};
        settings.enableFeature(Feature::AcceptNullInPattern);
        for (
            const auto pattern : {
                String{"^A.B$"_el},
                String{"^A[\\x00]B$"_el},
                String{"^A\\WB$"_el},
                String{"^A\\SB$"_el},
                String{"^A\\DB$"_el},
            }) {
            WITH_CONTEXT(requireFullMatchAllWidths(RegEx::compile(pattern, {}, settings)));
        }
    }

    void testNullInputCanBeExplicitlyRejected() {
        auto settings = Settings{};
        settings.enableFeature(Feature::AcceptNullInPattern);
        settings.disableFeature(Feature::AcceptNullInInput);
        const auto regEx = RegEx::compile("\\x00"_el, {}, settings);

        REQUIRE_THROWS_AS(el::text::EncodingError, regEx->fullMatch(String::fromCharacter(el::text::Char{U'\0'})));
    }
};
