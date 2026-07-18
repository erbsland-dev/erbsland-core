// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/List.hpp>
#include <erbsland/util/LoopResult.hpp>
#include <erbsland/util/LoopStatus.hpp>
#include <erbsland/util/Set.hpp>

#include <set>
#include <string>
#include <vector>

using el::text::AsciiCategory;
using el::text::CharSet;
using el::text::U8String;
using el::text::U8StringEditor;
using el::text::UnicodeCategory;
using el::util::LoopResult;
using el::util::LoopStatus;
using namespace el::text::literals;

TESTED_TARGETS(CharSet)
class CharSetTest final : public el::UnitTest {
public:
    void testConstructionAndNormalization() {
        using Char = el::text::Char;
        using CharRange = el::text::CharRange;
        using CharSet = el::text::CharSet;
        using namespace el::text::literals;

        const auto empty = CharSet{};
        const auto single = CharSet{Char{U'A'}};
        const auto range = CharSet::fromRange(Char{U'A'}, Char{U'C'});
        const auto fromView = CharSet{u8"A¢€A"_el};
        const auto fromSet = CharSet{el::util::Set<Char>{Char{U'A'}, Char{U'A'}, Char{0x110000U}, Char{U'B'}}};
        const auto fromList = CharSet{el::util::List<Char>{Char{U'C'}, Char{U'A'}, Char{U'C'}, Char{0xD800U}}};
        const auto normalized = CharSet::fromRange(Char{U'A'}, Char{U'C'}) | CharSet::fromRange(Char{U'D'}, Char{U'F'});

        REQUIRE(empty.isEmpty());
        REQUIRE(single.contains(Char{U'A'}));
        REQUIRE_FALSE(single.contains(Char{U'B'}));
        REQUIRE(range.contains(Char{U'B'}));
        REQUIRE(fromView.contains(Char{U'A'}));
        REQUIRE(fromView.contains(Char{0x00A2U}));
        REQUIRE(fromView.contains(Char{0x20ACU}));
        REQUIRE(fromSet.contains(Char{U'A'}));
        REQUIRE(fromSet.contains(Char{U'B'}));
        REQUIRE_FALSE(fromSet.contains(Char{0x110000U}));
        REQUIRE_EQUAL(fromList.toList().toStdVector(), std::vector<Char>({Char{U'A'}, Char{U'C'}}));
        REQUIRE_EQUAL(normalized.ranges().size(), std::size_t{1});
        REQUIRE(normalized.contains(Char{U'F'}));
    }

    void testCopyOnWriteMutation() {
        using Char = el::text::Char;
        using CharSet = el::text::CharSet;

        const auto original = CharSet{Char{U'A'}, Char{U'B'}};
        auto copy = original;

        copy.add(Char{U'C'});
        copy.remove(Char{U'A'});

        REQUIRE(original.contains(Char{U'A'}));
        REQUIRE(original.contains(Char{U'B'}));
        REQUIRE_FALSE(original.contains(Char{U'C'}));
        REQUIRE_FALSE(copy.contains(Char{U'A'}));
        REQUIRE(copy.contains(Char{U'B'}));
        REQUIRE(copy.contains(Char{U'C'}));
    }

    void testSetOperations() {
        using Char = el::text::Char;
        using CharSet = el::text::CharSet;

        const auto left = CharSet::fromRange(Char{U'A'}, Char{U'C'});
        const auto right = CharSet::fromRange(Char{U'B'}, Char{U'D'});

        REQUIRE_EQUAL(
            (left | right).toList().toStdVector(), std::vector<Char>({Char{U'A'}, Char{U'B'}, Char{U'C'}, Char{U'D'}}));
        REQUIRE_EQUAL((left & right).toList().toStdVector(), std::vector<Char>({Char{U'B'}, Char{U'C'}}));
        REQUIRE_EQUAL((left - right).toList().toStdVector(), std::vector<Char>({Char{U'A'}}));
        REQUIRE_EQUAL((left ^ right).toList().toStdVector(), std::vector<Char>({Char{U'A'}, Char{U'D'}}));
        REQUIRE(CharSet{Char{U'B'}} <= left);
        REQUIRE(left >= CharSet{Char{U'B'}});
        REQUIRE_FALSE(left <= CharSet{Char{U'B'}});

        auto mutated = left;
        mutated &= right;
        REQUIRE_EQUAL(mutated.toList().toStdVector(), std::vector<Char>({Char{U'B'}, Char{U'C'}}));
        mutated |= CharSet{Char{U'X'}};
        REQUIRE(mutated.contains(Char{U'X'}));
        mutated -= CharSet{Char{U'B'}};
        REQUIRE_FALSE(mutated.contains(Char{U'B'}));
        mutated ^= CharSet{Char{U'C'}, Char{U'D'}};
        REQUIRE_FALSE(mutated.contains(Char{U'C'}));
        REQUIRE(mutated.contains(Char{U'D'}));
    }

    void testCaseConversionAndCaseInsensitiveComparison() {
        using Char = el::text::Char;
        using CharSet = el::text::CharSet;

        const auto mixed = CharSet{Char{U'A'}, Char{U'a'}, Char{0x00C4U}};
        const auto folded = mixed.caseFolded();
        const auto lower = mixed.toLowercase();
        const auto upper = mixed.toUppercase();
        const auto digits = CharSet{Char{U'0'}, Char{U'9'}};

        REQUIRE_EQUAL(folded.toList().toStdVector(), std::vector<Char>({Char{U'a'}, Char{0x00E4U}}));
        REQUIRE_EQUAL(lower.toList().toStdVector(), std::vector<Char>({Char{U'a'}, Char{0x00E4U}}));
        REQUIRE_EQUAL(upper.toList().toStdVector(), std::vector<Char>({Char{U'A'}, Char{0x00C4U}}));
        REQUIRE(mixed.containsCaseFoldableCharacters());
        REQUIRE(mixed.containsLowercaseMappableCharacters());
        REQUIRE(mixed.containsUppercaseMappableCharacters());
        REQUIRE_FALSE(digits.containsCaseFoldableCharacters());
        REQUIRE_FALSE(digits.containsLowercaseMappableCharacters());
        REQUIRE_FALSE(digits.containsUppercaseMappableCharacters());
        REQUIRE_EQUAL(digits.caseFolded(), digits);
        REQUIRE_EQUAL(digits.toLowercase(), digits);
        REQUIRE_EQUAL(digits.toUppercase(), digits);
        REQUIRE(CharSet{Char{U'A'}}.isEqualToCI(CharSet{Char{U'a'}}));
        REQUIRE(CharSet{Char{0x212AU}}.isEqualToCI(CharSet{Char{U'k'}}));
        REQUIRE(CharSet{Char{U'A'}}.isSubsetOfCI(CharSet{Char{U'a'}, Char{U'B'}}));
    }

    void testFactoryMethods() {
        using Char = el::text::Char;
        using CharSet = el::text::CharSet;
        using namespace el::text::literals;

        const auto asciiHex = CharSet::from(AsciiCategory::HexDigit);
        REQUIRE(asciiHex.contains(Char{U'0'}));
        REQUIRE(asciiHex.contains(Char{U'F'}));
        REQUIRE(asciiHex.contains(Char{U'f'}));
        REQUIRE_FALSE(asciiHex.contains(Char{U'g'}));

        const auto asciiLetters = CharSet::fromRange(Char{U'a'}, Char{U'z'});
        REQUIRE(asciiLetters.contains(Char{U'm'}));
        REQUIRE_FALSE(asciiLetters.contains(Char{U'A'}));

        const auto decimalNumbers = CharSet::from(UnicodeCategory::DecimalNumber);
        REQUIRE(decimalNumbers.contains(Char{U'0'}));
        REQUIRE_FALSE(decimalNumbers.contains(Char{U'A'}));

        const auto u8Pattern = CharSet::fromPattern(u8"-a-f_0-9=/"_el);
        REQUIRE(u8Pattern.contains(Char{U'-'}));
        REQUIRE(u8Pattern.contains(Char{U'_'}));
        REQUIRE(u8Pattern.contains(Char{U'='}));
        REQUIRE(u8Pattern.contains(Char{U'/'}));
        REQUIRE(u8Pattern.contains(Char{U'c'}));
        REQUIRE(u8Pattern.contains(Char{U'5'}));
        REQUIRE_FALSE(u8Pattern.contains(Char{U'g'}));
        REQUIRE_FALSE(u8Pattern.contains(Char{U'A'}));

        REQUIRE(CharSet::fromPattern(u"a-c-"_el).contains(Char{U'-'}));
        REQUIRE(CharSet::fromPattern(U"\U0001F600-\U0001F603"_el).contains(Char{U'\U0001F602'}));
        REQUIRE_THROWS(CharSet::fromPattern("z-a"_el));
    }

    void testForEachAndTransform() {
        using Char = el::text::Char;
        using CharRange = el::text::CharRange;
        using CharSet = el::text::CharSet;

        const auto set = CharSet::fromRange(Char{U'A'}, Char{U'C'}) | CharSet::fromRange(Char{0xD7FFU}, Char{0xE000U});
        auto ranges = std::vector<CharRange>{};
        const auto completedRanges = set.forEach([&](const CharRange &range) -> void { ranges.push_back(range); });
        REQUIRE_EQUAL(completedRanges, LoopResult::Success);
        REQUIRE_EQUAL(
            ranges,
            std::vector<CharRange>({
                CharRange{Char{U'A'}, Char{U'C'}},
                CharRange{Char{0xD7FFU}, Char{0xE000U}},
            }));

        auto characters = std::vector<Char>{};
        const auto completedCharacters = set.forEach([&](const Char character) -> LoopStatus {
            characters.push_back(character);
            return characters.size() < 4 ? LoopStatus::Continue : LoopStatus::Stop;
        });
        REQUIRE_EQUAL(completedCharacters, LoopResult::Stopped);
        REQUIRE_EQUAL(characters, std::vector<Char>({Char{U'A'}, Char{U'B'}, Char{U'C'}, Char{0xD7FFU}}));

        const auto lower =
            CharSet::fromRange(Char{U'A'}, Char{U'C'}).transform([](const Char character) noexcept -> Char {
                return character.toLowercase();
            });
        const auto expectedLower = CharSet::fromRange(Char{U'a'}, Char{U'c'});
        REQUIRE_EQUAL(lower, expectedLower);

        const auto normalized =
            CharSet::fromRange(Char{U'A'}, Char{U'B'}).transform([](const Char character) noexcept -> Char {
                return character == Char{U'A'} ? Char{U'z'} : Char{U'a'};
            });
        REQUIRE_EQUAL(normalized.toList().toStdVector(), std::vector<Char>({Char{U'a'}, Char{U'z'}}));
    }

    void testExportSkipsInvalidSurrogateCodePoints() {
        using Char = el::text::Char;
        using CharRange = el::text::CharRange;
        using CharSet = el::text::CharSet;

        const auto set = CharSet::fromRange(Char{0xD7FFU}, Char{0xE000U});

        REQUIRE_EQUAL(set.toList().toStdVector(), std::vector<Char>({Char{0xD7FFU}, Char{0xE000U}}));
        REQUIRE_FALSE(set.contains(Char{0xD800U}));
        REQUIRE_EQUAL(set.toSet().toStdSet(), std::set<Char>({Char{0xD7FFU}, Char{0xE000U}}));

        const auto letters = CharSet::fromRange(Char{U'A'}, Char{U'C'});
        REQUIRE_EQUAL(letters.toString(), "ABC"_el);
        REQUIRE_EQUAL(letters.toU8String(), "ABC"_el);
        REQUIRE_EQUAL(letters.toU16String(), u"ABC"_el);
        REQUIRE_EQUAL(letters.toU32String(), U"ABC"_el);
    }

    void testInvalidUtf8ConstructionUsesReplacementCharacter() {
        using Char = el::text::Char;
        using CharSet = el::text::CharSet;

        auto data = std::string{"A"};
        data.push_back(static_cast<char>(0xC0U));
        data.push_back('B');
        const auto text = U8StringEditor{std::string_view{data}};
        const auto set = CharSet{U8String{text}};

        REQUIRE(set.contains(Char{U'A'}));
        REQUIRE(set.contains(Char::replacement()));
        REQUIRE(set.contains(Char{U'B'}));
    }
};
