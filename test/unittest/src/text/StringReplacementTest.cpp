// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CharSet.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/U16String.hpp>
#include <erbsland/U16StringEditor.hpp>
#include <erbsland/U32String.hpp>
#include <erbsland/U32StringEditor.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <cstddef>
#include <string>
#include <string_view>

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

TESTED_TARGETS(String StringEditor U16String U16StringEditor U32String U32StringEditor)
class StringReplacementTest final : public el::UnitTest {
public:
    void testU8StorageReuse() {
        WITH_CONTEXT((requireVariants<String, StringEditor>(
            String{u8"a¢€😀a"_el},
            String{std::u8string_view{u8"--a¢€😀a--"}},
            String{"a"_el},
            String{"x"_el},
            String{u8"😀"_el})));
    }

    void testU16StorageReuse() {
        WITH_CONTEXT((requireVariants<U16String, U16StringEditor>(
            U16String{u"a¢€😀a"_el},
            U16String{std::u16string_view{u"--a¢€😀a--"}},
            U16String{u"a"_el},
            U16String{u"x"_el},
            U16String{u"😀"_el})));
    }

    void testU32StorageReuse() {
        WITH_CONTEXT((requireVariants<U32String, U32StringEditor>(
            U32String{U"a¢€😀a"_el},
            U32String{std::u32string_view{U"--a¢€😀a--"}},
            U32String{U"a"_el},
            U32String{U"x"_el},
            U32String{U"😀"_el})));
    }

    void testCaseFoldedMatches() {
        WITH_CONTEXT(requireCaseFolded(String{u8"KK"_el}, String{"k"_el}, String{"K"_el}));
        WITH_CONTEXT(requireCaseFolded(U16String{u"KK"_el}, U16String{u"k"_el}, U16String{u"K"_el}));
        WITH_CONTEXT(requireCaseFolded(U32String{U"KK"_el}, U32String{U"k"_el}, U32String{U"K"_el}));
    }

    void testMalformedInput() {
        const auto u8 = String{std::string_view{el::unittest::th::stdStringFromHex("61 c0 62")}};
        const auto u16Data = std::u16string{u'a', static_cast<char16_t>(0xD800U), u'b'};
        const auto u16 = U16String{std::u16string_view{u16Data}};
        const auto u32Data = std::u32string{U'a', static_cast<char32_t>(0x110000U), U'b'};
        const auto u32 = U32String{std::u32string_view{u32Data}};
        WITH_CONTEXT(requireMalformed(u8, String{"x"_el}, String{u8"�"_el}));
        WITH_CONTEXT(requireMalformed(u16, U16String{u"x"_el}, U16String{u"�"_el}));
        WITH_CONTEXT(requireMalformed(u32, U32String{U"x"_el}, U32String{U"�"_el}));
    }

    void testLargeNeedleIsSearchedOnce() {
        WITH_CONTEXT((requireSingleSearch<String, StringEditor>()));
        WITH_CONTEXT((requireSingleSearch<U16String, U16StringEditor>()));
        WITH_CONTEXT((requireSingleSearch<U32String, U32StringEditor>()));
    }

    void testStreamingOutput() {
        WITH_CONTEXT((requireStreamingOutput<String, StringEditor>()));
        WITH_CONTEXT((requireStreamingOutput<U16String, U16StringEditor>()));
        WITH_CONTEXT((requireStreamingOutput<U32String, U32StringEditor>()));
    }

    void testSensitiveU8Storage() {
        auto source = String{u8"a¢€😀a"_el};
        source.markAsSensitive();
        const auto unchanged = source.replacedAll("x"_el, "y"_el);
        const auto changed = source.replacedAll("a"_el, "x"_el);
        WITH_CONTEXT(requireUnchanged(source, unchanged));
        REQUIRE(unchanged.isSensitive());
        REQUIRE(changed.isSensitive());
        REQUIRE_NOT_EQUAL(changed.storageId(), source.storageId());
    }

private:
    static auto countingCompare(const Char left, const Char right) noexcept -> std::strong_ordering {
        ++_comparisonCount;
        return left <=> right;
    }

    template <typename tString, typename tEditor>
    void requireSingleSearch() {
        auto needleEditor = tEditor{};
        needleEditor.append(U'a', CpCount{4096U});
        const auto needle = tString{needleEditor};
        auto sourceEditor = tEditor{};
        sourceEditor.append(U'p', CpCount{8192U}).append(needle).append(needle).append(U'z');
        const auto source = tString{sourceEditor};
        const auto replacement = tString{tEditor{}.append(U'x')};
        _comparisonCount = 0U;
        const auto result = source.replacedAll(needle, replacement, countingCompare);
        REQUIRE_EQUAL(_comparisonCount, std::size_t{16385U});
        auto expected = tEditor{};
        expected.append(U'p', CpCount{8192U}).append(U'x', CpCount{2U}).append(U'z');
        REQUIRE_EQUAL(result, tString{expected});
        _comparisonCount = 0U;
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(needle, needle, countingCompare)));
        REQUIRE_EQUAL(_comparisonCount, std::size_t{16385U});
        _comparisonCount = 0U;
        const auto removed = source.removedAll(needle, countingCompare);
        REQUIRE_EQUAL(_comparisonCount, std::size_t{16385U});
        const auto removedExpected = tString{tEditor{}.append(U'p', CpCount{8192U}).append(U'z')};
        REQUIRE_EQUAL(removed, removedExpected);
        const auto pruned = source.kept(CpRange{CpIndex{8192U}, CpCount{1U}});
        REQUIRE_EQUAL(StringConverter{pruned}.toStdU32String(), std::u32string{U"a"});
        REQUIRE_LESS(tEditor{pruned}.memoryUsage(), tEditor{source}.memoryUsage());
    }

    template <typename tString, typename tEditor>
    void requireStreamingOutput() {
        auto sourceEditor = tEditor{};
        sourceEditor.append(U'a').append(U'A').append(U'a').append(U'A');
        const auto source = tString{sourceEditor};
        const auto needle = tString{tEditor{}.append(U'a')};
        const auto mixed = source.replacedAll(needle, needle, Char::compareAsciiFolded);
        REQUIRE_EQUAL(StringConverter{mixed}.toStdU32String(), std::u32string{U"aaaa"});
        const auto deleted = source.replacedAll(needle, tString{});
        REQUIRE_EQUAL(StringConverter{deleted}.toStdU32String(), std::u32string{U"AA"});
        const auto allDeleted = mixed.replacedAll(needle, tString{});
        REQUIRE(allDeleted.isEmpty());
        REQUIRE_EQUAL(source.removedAll(needle), deleted);
        REQUIRE_EQUAL(source.removedAll(CharSet{U'a'}), deleted);
        REQUIRE(mixed.removedAll(needle).isEmpty());
        REQUIRE(source.replacedAll(CharSet{U'a', U'A'}, tString{}).isEmpty());
        const auto replacement = tString{tEditor{}.append(U'😀', CpCount{20U})};
        const auto grown = mixed.replacedAll(needle, replacement);
        const auto grownSet = mixed.replacedAll(CharSet{U'a'}, replacement);
        const auto expected = tString{tEditor{}.append(U'😀', CpCount{80U})};
        REQUIRE_EQUAL(grown, expected);
        REQUIRE_EQUAL(grownSet, expected);
        REQUIRE_EQUAL(StringConverter{source}.toStdU32String(), std::u32string{U"aAaA"});
    }

    template <typename T>
    void requireUnchanged(const T &source, const T &result) {
        REQUIRE_EQUAL(result, source);
        REQUIRE_EQUAL(result.storageId(), source.storageId());
    }

    template <typename tString, typename tEditor>
    void requireVariants(
        const tString &literal,
        const tString &shared,
        const tString &letter,
        const tString &missing,
        const tString &emoji) {
        const auto literalSlice = literal.slice(CpRange{CpIndex{0U}, CpCount{4U}});
        WITH_CONTEXT(requireReplacements(literalSlice, letter, missing, emoji));
        WITH_CONTEXT(requireReplacements(tEditor{literalSlice}, letter, missing, emoji));
        const auto slice = shared.slice(CpRange{CpIndex{2U}, CpCount{5U}});
        const auto pruned = shared.kept(CpRange{CpIndex{2U}, CpCount{5U}});
        REQUIRE_EQUAL(pruned, slice);
        REQUIRE_NOT_EQUAL(pruned.storageId(), shared.storageId());
        const auto prunedEditor = tEditor{shared}.kept(CpRange{CpIndex{2U}, CpCount{5U}});
        REQUIRE_EQUAL(tString{prunedEditor}, slice);
        REQUIRE_NOT_EQUAL(prunedEditor.storageId(), shared.storageId());
        WITH_CONTEXT(requireReplacements(literal, letter, missing, emoji));
        WITH_CONTEXT(requireReplacements(shared, letter, missing, emoji));
        WITH_CONTEXT(requireReplacements(slice, letter, missing, emoji));
        WITH_CONTEXT(requireReplacements(tEditor{literal}, letter, missing, emoji));
        WITH_CONTEXT(requireReplacements(tEditor{shared}, letter, missing, emoji));
        WITH_CONTEXT(requireReplacements(tEditor{slice}, letter, missing, emoji));
        WITH_CONTEXT(requireReplacements(tString{}, letter, missing, emoji));
        WITH_CONTEXT(requireReplacements(tEditor{}, letter, missing, emoji));
    }

    template <typename tString, typename tReadOnly>
    void requireReplacements(
        const tString &source, const tReadOnly &letter, const tReadOnly &missing, const tReadOnly &emoji) {
        WITH_CONTEXT(requireUnchanged(source, source.removedAll(CharSet{U'x'})));
        WITH_CONTEXT(requireUnchanged(source, source.removedAll(missing)));
        WITH_CONTEXT(requireUnchanged(source, source.removedAll(CharSet{})));
        WITH_CONTEXT(requireUnchanged(source, source.removedAll(tReadOnly{})));
        WITH_CONTEXT(requireUnchanged(source, source.removedAll(missing, Char::compareCaseFolded)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(CharSet{U'x'}, U'y')));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(CharSet{U'x'}, letter)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(missing, letter)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(CharSet{}, U'x')));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(CharSet{}, missing)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(tReadOnly{}, missing)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(CharSet{U'a'}, U'a')));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(CharSet{U'a'}, letter)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(letter, letter)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(emoji, emoji)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(CharSet{U'😀'}, emoji)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(letter, letter, Char::compareCaseFolded)));
        if (!source.isEmpty()) {
            const auto changedCharacter = source.replacedAll(CharSet{U'a'}, U'x');
            const auto changedSet = source.replacedAll(CharSet{U'a'}, missing);
            const auto changedText = source.replacedAll(letter, missing);
            REQUIRE_NOT_EQUAL(changedCharacter.storageId(), source.storageId());
            REQUIRE_NOT_EQUAL(changedSet.storageId(), source.storageId());
            REQUIRE_NOT_EQUAL(changedText.storageId(), source.storageId());
            REQUIRE_EQUAL(changedCharacter, changedSet);
            REQUIRE_EQUAL(changedSet, changedText);
        }
    }

    template <typename T>
    void requireCaseFolded(const T &source, const T &needle, const T &replacement) {
        const auto changed = source.replacedAll(needle, replacement, Char::compareCaseFolded);
        REQUIRE_EQUAL(StringConverter{changed}.toStdU32String(), std::u32string{U"KK"});
        REQUIRE_NOT_EQUAL(changed.storageId(), source.storageId());
        WITH_CONTEXT(requireUnchanged(changed, changed.replacedAll(needle, replacement, Char::compareCaseFolded)));
        const auto changedSet = source.replacedAll(CharSet{U'K', U'K'}, U'K');
        REQUIRE_EQUAL(changedSet, changed);
        REQUIRE_NOT_EQUAL(changedSet.storageId(), source.storageId());
    }

    template <typename T>
    void requireMalformed(const T &source, const T &missing, const T &replacement) {
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(missing, replacement)));
        WITH_CONTEXT(requireUnchanged(source, source.removedAll(CharSet{U'x'})));
        WITH_CONTEXT(requireUnchanged(source, source.removedAll(missing)));
        WITH_CONTEXT(requireUnchanged(source, source.removedAll(CharSet{})));
        WITH_CONTEXT(requireUnchanged(source, source.removedAll(T{})));
        WITH_CONTEXT(requireUnchanged(source, source.removedAll(missing, Char::compareCaseFolded)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedAll(CharSet{U'x'}, U'y')));
        const auto changed = source.replacedAll(CharSet{Char::replacement()}, replacement);
        REQUIRE_EQUAL(StringConverter{changed}.toStdU32String(), std::u32string{U"a�b"});
        REQUIRE_NOT_EQUAL(changed.storageId(), source.storageId());
    }

private:
    static inline std::size_t _comparisonCount{};
};
