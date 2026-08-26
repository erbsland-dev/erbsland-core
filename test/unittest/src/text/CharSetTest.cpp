// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/List.hpp>
#include <erbsland/util/LoopResult.hpp>
#include <erbsland/util/LoopStatus.hpp>
#include <erbsland/util/Set.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <set>
#include <string>
#include <vector>

using namespace el::text::literals;

using el::text::AsciiCategory;
using el::text::CharSet;
using el::text::String;
using el::text::StringEditor;
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
        auto normalizedRanges = std::vector<CharRange>{};
        normalized.forEach([&normalizedRanges](const CharRange value) -> void { normalizedRanges.push_back(value); });
        REQUIRE_EQUAL(normalizedRanges.size(), std::size_t{1});
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

    void testInlineAndSharedStorageTransitions() {
        using Char = el::text::Char;
        using CharRange = el::text::CharRange;
        using CharSet = el::text::CharSet;

        auto value = CharSet{Char{U'a'}, Char{U'c'}};
        REQUIRE_EQUAL(collectRanges(value).size(), std::size_t{2});

        value.add(Char{U'e'});
        REQUIRE_EQUAL(collectRanges(value).size(), std::size_t{3});
        const auto sharedCopy = value;

        value.add(CharRange{U'a', U'e'});
        REQUIRE_EQUAL(collectRanges(value), std::vector<CharRange>({CharRange{U'a', U'e'}}));
        REQUIRE_EQUAL(
            collectRanges(sharedCopy), std::vector<CharRange>({CharRange{U'a'}, CharRange{U'c'}, CharRange{U'e'}}));

        auto split = CharSet::fromRange(U'a', U'z');
        split.remove(Char{U'm'});
        REQUIRE_EQUAL(collectRanges(split), std::vector<CharRange>({CharRange{U'a', U'l'}, CharRange{U'n', U'z'}}));

        auto reduced = sharedCopy;
        reduced.remove(CharRange{U'c', U'e'});
        REQUIRE_EQUAL(collectRanges(reduced), std::vector<CharRange>({CharRange{U'a'}}));
        reduced.remove(Char{U'a'});
        REQUIRE(reduced.isEmpty());
    }

    void testMoveKeepsSourceValue() {
        using Char = el::text::Char;
        using CharSet = el::text::CharSet;

        auto source = CharSet{Char{U'a'}, Char{U'c'}, Char{U'e'}};
        const auto expected = source;
        const auto constructed = std::move(source);
        REQUIRE_EQUAL(constructed, expected);
        REQUIRE_EQUAL(source, expected);

        auto assigned = CharSet{};
        assigned = std::move(source);
        REQUIRE_EQUAL(assigned, expected);
        REQUIRE_EQUAL(source, expected);
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
        const auto singleB = CharSet{Char{U'B'}};
        REQUIRE_LESS_EQUAL(singleB, left);
        REQUIRE_GREATER_EQUAL(left, singleB);
        REQUIRE_NOT_EQUAL(left, singleB);

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

    void testAsciiCategoryExactMemberships() {
        using Char = el::text::Char;

        REQUIRE_EQUAL(CharSet::from(AsciiCategory::Word), CharSet::fromPattern("_A-Za-z0-9"_el));
        REQUIRE_EQUAL(CharSet::from(AsciiCategory::WordWithHyphen), CharSet::fromPattern("-_A-Za-z0-9"_el));
        REQUIRE_EQUAL(CharSet::from(AsciiCategory::DottedName), CharSet::fromPattern("-._A-Za-z0-9"_el));
        REQUIRE_EQUAL(CharSet::from(AsciiCategory::UrlScheme), CharSet::fromPattern("-+.A-Za-z0-9"_el));
        REQUIRE_EQUAL(CharSet::from(AsciiCategory::Base64Text), CharSet::fromPattern("+/=A-Za-z0-9"_el));

        auto httpToken = CharSet::from(AsciiCategory::Alphanumeric);
        httpToken.add({
            Char{U'!'},
            Char{U'#'},
            Char{U'$'},
            Char{U'%'},
            Char{U'&'},
            Char{U'\''},
            Char{U'*'},
            Char{U'+'},
            Char{U'-'},
            Char{U'.'},
            Char{U'^'},
            Char{U'_'},
            Char{U'`'},
            Char{U'|'},
            Char{U'~'},
        });
        REQUIRE_EQUAL(CharSet::from(AsciiCategory::HttpToken), httpToken);
        REQUIRE_FALSE(CharSet::from(AsciiCategory::HttpToken).contains(Char{U'\u0080'}));
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
        const auto text = StringEditor{std::string_view{data}};
        const auto set = CharSet{String{text}};

        REQUIRE(set.contains(Char{U'A'}));
        REQUIRE(set.contains(Char::replacement()));
        REQUIRE(set.contains(Char{U'B'}));
    }

    void testAlgebraAgainstReferenceSets() {
        constexpr auto setCount = std::uint32_t{64U};
        for (auto leftMask = std::uint32_t{0}; leftMask < setCount; ++leftMask) {
            for (auto rightMask = std::uint32_t{0}; rightMask < setCount; ++rightMask) {
                runWithContext(
                    SOURCE_LOCATION(),
                    [&]() -> void { requireAlgebra(leftMask, rightMask); },
                    [leftMask, rightMask]() -> std::string {
                        return "left mask: " + std::to_string(leftMask) + ", right mask: " + std::to_string(rightMask);
                    });
            }
        }
    }

private:
    using Char = el::text::Char;
    using CharRange = el::text::CharRange;
    using CharSet = el::text::CharSet;
    using ReferenceSet = std::set<Char>;

    [[nodiscard]] static auto collectRanges(const CharSet &value) -> std::vector<CharRange> {
        auto result = std::vector<CharRange>{};
        value.forEach([&result](const CharRange range) -> void { result.push_back(range); });
        return result;
    }

    [[nodiscard]] static auto createSet(const std::uint32_t mask) -> CharSet {
        constexpr auto characters = std::array<Char, 6>{U'a', U'b', U'd', U'e', U'g', U'h'};
        auto result = CharSet{};
        for (auto index = std::size_t{0}; index < characters.size(); ++index) {
            if ((mask & (std::uint32_t{1U} << index)) != 0U) {
                result.add(characters[index]);
            }
        }
        return result;
    }

    [[nodiscard]] static auto createReferenceSet(const std::uint32_t mask) -> ReferenceSet {
        constexpr auto characters = std::array<Char, 6>{U'a', U'b', U'd', U'e', U'g', U'h'};
        auto result = ReferenceSet{};
        for (auto index = std::size_t{0}; index < characters.size(); ++index) {
            if ((mask & (std::uint32_t{1U} << index)) != 0U) {
                result.insert(characters[index]);
            }
        }
        return result;
    }

    void requireEqualToSet(const CharSet &actual, const ReferenceSet &expected) {
        REQUIRE_EQUAL(actual.toSet().toStdSet(), expected);
        for (auto codePoint = char32_t{U'`'}; codePoint <= U'i'; ++codePoint) {
            const auto character = Char{codePoint};
            REQUIRE_EQUAL(actual.contains(character), expected.contains(character));
        }
    }

    void requireAlgebra(const std::uint32_t leftMask, const std::uint32_t rightMask) {
        const auto left = createSet(leftMask);
        const auto right = createSet(rightMask);
        const auto leftReference = createReferenceSet(leftMask);
        const auto rightReference = createReferenceSet(rightMask);

        auto unionReference = leftReference;
        unionReference.insert(rightReference.begin(), rightReference.end());
        WITH_CONTEXT(requireEqualToSet(left | right, unionReference));

        auto intersectionReference = ReferenceSet{};
        std::ranges::set_intersection(
            leftReference, rightReference, std::inserter(intersectionReference, intersectionReference.end()));
        WITH_CONTEXT(requireEqualToSet(left & right, intersectionReference));

        auto subtractionReference = ReferenceSet{};
        std::ranges::set_difference(
            leftReference, rightReference, std::inserter(subtractionReference, subtractionReference.end()));
        WITH_CONTEXT(requireEqualToSet(left - right, subtractionReference));

        auto symmetricDifferenceReference = ReferenceSet{};
        std::ranges::set_symmetric_difference(
            leftReference,
            rightReference,
            std::inserter(symmetricDifferenceReference, symmetricDifferenceReference.end()));
        WITH_CONTEXT(requireEqualToSet(left ^ right, symmetricDifferenceReference));

        REQUIRE_EQUAL(left.isSubsetOf(right), std::ranges::includes(rightReference, leftReference));
        REQUIRE_EQUAL(left.isSupersetOf(right), std::ranges::includes(leftReference, rightReference));
    }
};
