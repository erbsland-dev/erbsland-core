// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Char.hpp>
#include <erbsland/CharRange.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/List.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <initializer_list>
#include <type_traits>

using el::text::Char;
using el::text::CharRange;
using el::text::CharSet;

TESTED_TARGETS(CharSet)
class CharSetConstructionTest final : public el::UnitTest {
public:
    void testSingleCharacterOverload() {
        // An initializer-list constructor would make these brace expressions potentially throwing.
        static_assert(noexcept(CharSet{U'x'}));
        // anti-pattern: allow unnecessary_type_construction -- Verify the explicit Char overload alongside char32_t.
        static_assert(noexcept(CharSet{Char{U'x'}}));
        static_assert(!std::is_constructible_v<CharSet, std::initializer_list<Char>>);
        static_assert(!std::is_constructible_v<CharSet, CharRange, Char>);
        static_assert(!std::is_constructible_v<CharSet, Char, CharRange>);

        REQUIRE_EQUAL(collectRanges(CharSet{U'x'}), (el::List<CharRange>{CharRange{U'x'}}));
        // anti-pattern: allow unnecessary_type_construction -- Verify the explicit Char overload alongside char32_t.
        REQUIRE_EQUAL(CharSet{Char{U'x'}}, CharSet(U'x'));
        REQUIRE(CharSet{Char{0xD800U}}.isEmpty());
        REQUIRE(CharSet{Char{0x110000U}}.isEmpty());
    }

    void testTwoCharacterNormalization() {
        const auto characters = std::array<Char, 10>{
            U'a',
            U'b',
            U'z',
            Char{0U},
            Char{0xD7FFU},
            Char{0xE000U},
            Char{0x10FFFFU},
            Char{0xD800U},
            Char{0xDFFFU},
            Char{0x110000U}};
        for (const auto first : characters) {
            for (const auto second : characters) {
                runWithContext(SOURCE_LOCATION(), [&]() -> void {
                    auto expected = CharSet{};
                    expected.add(first);
                    expected.add(second);
                    REQUIRE_EQUAL(collectRanges(CharSet{first, second}), collectRanges(expected));
                });
            }
        }
        REQUIRE_EQUAL((CharSet{U'x', Char{U'y'}}), (CharSet{CharRange{U'x', U'y'}}));
        REQUIRE_EQUAL((CharSet{U'x', U'x'}), CharSet{U'x'});
        REQUIRE_FALSE((CharSet{U'0', U'9'}.contains(U'5')));
    }

    void testMultipleCharactersAndSharedStorage() {
        const auto sparse = CharSet{U'z', Char{U'a'}, U'm', U'a', Char{0xD800U}, U'c'};
        REQUIRE_EQUAL(
            collectRanges(sparse),
            (el::List<CharRange>{CharRange{U'a'}, CharRange{U'c'}, CharRange{U'm'}, CharRange{U'z'}}));
        auto copy = sparse;
        copy.add(U'b');
        REQUIRE_FALSE(sparse.contains(U'b'));
        REQUIRE(copy.contains(U'b'));

        const auto merged = CharSet{U'c', U'a', U'b', U'e', U'd', U'a'};
        REQUIRE_EQUAL(collectRanges(merged), (el::List<CharRange>{CharRange{U'a', U'e'}}));
        REQUIRE_EQUAL((CharSet{Char{0x110000U}, Char{0xD800U}, U'x'}), CharSet{U'x'});
    }

    void testInitializerListFactory() {
        REQUIRE(CharSet::fromCharacters({}).isEmpty());
        REQUIRE_EQUAL(CharSet::fromCharacters({U'x'}), CharSet{U'x'});
        REQUIRE_EQUAL(CharSet::fromCharacters({U'y', U'x'}), (CharSet{U'x', U'y'}));
        const auto characters = std::initializer_list<Char>{U'z', U'a', U'm', U'a', Char{0xD800U}, U'c'};
        REQUIRE_EQUAL(CharSet::fromCharacters(characters), (CharSet{U'a', U'c', U'm', U'z'}));
    }

    void testRangeOverloads() {
        const auto ranges = std::array<CharRange, 5>{
            CharRange{}, CharRange{U'a'}, CharRange{U'b', U'd'}, CharRange{U'c', U'f'}, CharRange{U'z'}};
        for (const auto first : ranges) {
            REQUIRE_EQUAL(collectRanges(CharSet{first}), collectRanges(CharSet(first)));
            for (const auto second : ranges) {
                runWithContext(SOURCE_LOCATION(), [&]() -> void {
                    auto expected = CharSet{};
                    expected.add(first);
                    expected.add(second);
                    REQUIRE_EQUAL(collectRanges(CharSet{first, second}), collectRanges(expected));
                });
            }
        }
    }

private:
    static auto collectRanges(const CharSet &set) -> el::List<CharRange> {
        auto result = el::List<CharRange>{};
        set.forEach([&](const CharRange range) -> void { result.append(range); });
        return result;
    }
};
