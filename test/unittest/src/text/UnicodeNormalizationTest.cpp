// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/impl/StringTraits.hpp>
#include <erbsland/text/NormalizationForm.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <concepts>
#include <string>
#include <string_view>

using namespace el::text;

static_assert(std::same_as<el::text::impl::SharedStorageFor<const U8String &>, el::text::impl::U8StringSharedStorage>);
static_assert(std::same_as<el::text::impl::SharedStorageFor<U8StringEditor>, el::text::impl::U8StringSharedStorage>);
static_assert(
    std::same_as<el::text::impl::SharedStorageFor<const U16String &>, el::text::impl::U16StringSharedStorage>);
static_assert(std::same_as<el::text::impl::SharedStorageFor<U16StringEditor>, el::text::impl::U16StringSharedStorage>);
static_assert(
    std::same_as<el::text::impl::SharedStorageFor<const U32String &>, el::text::impl::U32StringSharedStorage>);
static_assert(std::same_as<el::text::impl::SharedStorageFor<U32StringEditor>, el::text::impl::U32StringSharedStorage>);
static_assert(std::same_as<el::text::impl::StringIndexTypeFor<const U8String &>, el::unit::ByteIndex>);
static_assert(std::same_as<el::text::impl::StringIndexTypeFor<U8StringEditor>, el::unit::ByteIndex>);
static_assert(std::same_as<el::text::impl::StringIndexTypeFor<const U16String &>, el::unit::U16DataIndex>);
static_assert(std::same_as<el::text::impl::StringIndexTypeFor<U16StringEditor>, el::unit::U16DataIndex>);
static_assert(std::same_as<el::text::impl::StringIndexTypeFor<const U32String &>, el::unit::CpIndex>);
static_assert(std::same_as<el::text::impl::StringIndexTypeFor<U32StringEditor>, el::unit::CpIndex>);

TESTED_TARGETS(NormalizationForm U8String U8StringEditor U16String U16StringEditor U32String U32StringEditor)
class UnicodeNormalizationTest final : public el::UnitTest {
private:
    template <typename T>
    void requireNormalized(const T &source, const NormalizationForm form, const U32String &expected) {
        const auto normalized = source.normalized(form);
        REQUIRE_EQUAL(StringConverter{normalized}.toU32String(), expected);
    }

    void requireAllWidths(
        const std::u32string_view source, const NormalizationForm form, const std::u32string_view expected) {
        requireAllWidths(U32String{source}, form, U32String{expected});
    }

    void requireAllWidths(const U32String &source, const NormalizationForm form, const U32String &expected) {
        const auto sourceU16 = StringConverter{source}.toU16String();
        const auto sourceU8 = StringConverter{source}.toU8String();
        WITH_CONTEXT(requireNormalized(source, form, expected));
        WITH_CONTEXT(requireNormalized(sourceU16, form, expected));
        WITH_CONTEXT(requireNormalized(sourceU8, form, expected));
        WITH_CONTEXT(requireNormalized(U32StringEditor{source}, form, expected));
        WITH_CONTEXT(requireNormalized(U16StringEditor{sourceU16}, form, expected));
        WITH_CONTEXT(requireNormalized(U8StringEditor{sourceU8}, form, expected));
    }

public:
    void testCanonicalDecompositionAndComposition() {
        WITH_CONTEXT(requireAllWidths(U"\u00C5", NormalizationForm::Nfd, U"A\u030A"));
        WITH_CONTEXT(requireAllWidths(U"A\u030A", NormalizationForm::Nfc, U"\u00C5"));
        WITH_CONTEXT(requireAllWidths(U"\u212B", NormalizationForm::Nfd, U"A\u030A"));
        WITH_CONTEXT(requireAllWidths(U"\u212B", NormalizationForm::Nfc, U"\u00C5"));
    }

    void testCompatibilityDecomposition() {
        WITH_CONTEXT(requireAllWidths(U"\uFB03", NormalizationForm::Nfc, U"\uFB03"));
        WITH_CONTEXT(requireAllWidths(U"\uFB03", NormalizationForm::Nfkc, U"ffi"));
        WITH_CONTEXT(requireAllWidths(U"\uFB03", NormalizationForm::Nfkd, U"ffi"));
        WITH_CONTEXT(requireAllWidths(
            U"\uFDFA",
            NormalizationForm::Nfkd,
            U"\u0635\u0644\u0649 \u0627\u0644\u0644\u0647 \u0639\u0644\u064A\u0647 \u0648\u0633\u0644\u0645"));
    }

    void testCanonicalOrderingAndBlocking() {
        WITH_CONTEXT(requireAllWidths(U"D\u0307\u0323", NormalizationForm::Nfd, U"D\u0323\u0307"));
        WITH_CONTEXT(requireAllWidths(U"D\u0307\u0323", NormalizationForm::Nfc, U"\u1E0C\u0307"));
        WITH_CONTEXT(requireAllWidths(U"A\u0300\u0305", NormalizationForm::Nfc, U"\u00C0\u0305"));
    }

    void testCompositionExclusion() {
        WITH_CONTEXT(requireAllWidths(U"\u0958", NormalizationForm::Nfc, U"\u0915\u093C"));
    }

    void testHangul() {
        WITH_CONTEXT(requireAllWidths(U"\uAC01", NormalizationForm::Nfd, U"\u1100\u1161\u11A8"));
        WITH_CONTEXT(requireAllWidths(U"\u1100\u1161\u11A8", NormalizationForm::Nfc, U"\uAC01"));
    }

    void testUnchangedValuesKeepStorage() {
        const auto u8 = U8String{"Already normalized"};
        const auto u16 = U16String{u"Already normalized"};
        const auto u32 = U32String{U"Already normalized"};

        REQUIRE_EQUAL(u8.normalized(NormalizationForm::Nfkc).storageId(), u8.storageId());
        REQUIRE_EQUAL(u16.normalized(NormalizationForm::Nfkc).storageId(), u16.storageId());
        REQUIRE_EQUAL(u32.normalized(NormalizationForm::Nfkc).storageId(), u32.storageId());
    }

    void testUnchangedEditorsKeepStorageAndCapacity() {
        auto u8 = U8StringEditor{"Already normalized"};
        auto u16 = U16StringEditor{u"Already normalized"};
        auto u32 = U32StringEditor{U"Already normalized"};
        u8.reserve(el::unit::ByteLength{100});
        u16.reserve(el::unit::U16DataLength{100});
        u32.reserve(el::unit::CpLength{100});
        const auto u8Id = u8.storageId();
        const auto u16Id = u16.storageId();
        const auto u32Id = u32.storageId();
        const auto u8Capacity = u8.capacity();
        const auto u16Capacity = u16.capacity();
        const auto u32Capacity = u32.capacity();
        const auto u8Alias = u8;
        const auto u16Alias = u16;
        const auto u32Alias = u32;

        REQUIRE_EQUAL(u8.normalized(NormalizationForm::Nfkc).storageId(), u8Id);
        REQUIRE_EQUAL(u16.normalized(NormalizationForm::Nfkc).storageId(), u16Id);
        REQUIRE_EQUAL(u32.normalized(NormalizationForm::Nfkc).storageId(), u32Id);
        REQUIRE_EQUAL(u8.normalize(NormalizationForm::Nfkc).storageId(), u8Id);
        REQUIRE_EQUAL(u16.normalize(NormalizationForm::Nfkc).storageId(), u16Id);
        REQUIRE_EQUAL(u32.normalize(NormalizationForm::Nfkc).storageId(), u32Id);
        REQUIRE_EQUAL(u8.capacity(), u8Capacity);
        REQUIRE_EQUAL(u16.capacity(), u16Capacity);
        REQUIRE_EQUAL(u32.capacity(), u32Capacity);
        REQUIRE_EQUAL(u8Alias.storageId(), u8Id);
        REQUIRE_EQUAL(u16Alias.storageId(), u16Id);
        REQUIRE_EQUAL(u32Alias.storageId(), u32Id);
    }

    void testChangedEditorNormalizesInPlace() {
        auto u8 = U8StringEditor{StringConverter{U32String{U"\uFB03"}}.toU8String()};
        auto u16 = U16StringEditor{u"\uFB03"};
        auto u32 = U32StringEditor{U"\uFB03"};

        REQUIRE_EQUAL(StringConverter{u8.normalize(NormalizationForm::Nfkc)}.toU32String(), U32String{U"ffi"});
        REQUIRE_EQUAL(StringConverter{u16.normalize(NormalizationForm::Nfkc)}.toU32String(), U32String{U"ffi"});
        REQUIRE_EQUAL(StringConverter{u32.normalize(NormalizationForm::Nfkc)}.toU32String(), U32String{U"ffi"});
    }

    void testMalformedEncodingIsReplaced() {
        const auto invalidU8 = std::array<U8String, 3>{
            U8String{el::unittest::th::stdStringFromHex("ff 41 42")},
            U8String{el::unittest::th::stdStringFromHex("41 ff 42")},
            U8String{el::unittest::th::stdStringFromHex("41 42 ff")},
        };
        const auto invalidU16Data = std::array<std::array<char16_t, 3>, 3>{
            std::array<char16_t, 3>{0xD800, u'A', u'B'},
            std::array<char16_t, 3>{u'A', 0xD800, u'B'},
            std::array<char16_t, 3>{u'A', u'B', 0xD800},
        };
        const auto invalidU32Data = std::array<std::array<char32_t, 3>, 3>{
            std::array<char32_t, 3>{0xD800, U'A', U'B'},
            std::array<char32_t, 3>{U'A', 0xD800, U'B'},
            std::array<char32_t, 3>{U'A', U'B', 0xD800},
        };
        const auto expected = std::array<U32String, 3>{
            U32String{U"\uFFFDAB"},
            U32String{U"A\uFFFDB"},
            U32String{U"AB\uFFFD"},
        };

        for (auto index = std::size_t{0}; index < expected.size(); ++index) {
            const auto invalidU16 =
                U16String{std::u16string_view{invalidU16Data[index].data(), invalidU16Data[index].size()}};
            const auto invalidU32 =
                U32String{std::u32string_view{invalidU32Data[index].data(), invalidU32Data[index].size()}};
            REQUIRE_EQUAL(
                StringConverter{invalidU8[index].normalized(NormalizationForm::Nfc)}.toU32String(), expected[index]);
            REQUIRE_EQUAL(
                StringConverter{invalidU16.normalized(NormalizationForm::Nfc)}.toU32String(), expected[index]);
            REQUIRE_EQUAL(invalidU32.normalized(NormalizationForm::Nfc), expected[index]);
        }

        const auto invalidU16 = U16String{std::u16string_view{invalidU16Data[1].data(), invalidU16Data[1].size()}};
        const auto invalidU32 = U32String{std::u32string_view{invalidU32Data[1].data(), invalidU32Data[1].size()}};
        auto invalidU8Editor = U8StringEditor{invalidU8[1]};
        auto invalidU16Editor = U16StringEditor{invalidU16};
        auto invalidU32Editor = U32StringEditor{invalidU32};
        invalidU8Editor.normalize(NormalizationForm::Nfc);
        invalidU16Editor.normalize(NormalizationForm::Nfc);
        invalidU32Editor.normalize(NormalizationForm::Nfc);
        REQUIRE(invalidU8Editor.isValidUtf8());
        REQUIRE(invalidU16Editor.isValidUtf16());
        REQUIRE(invalidU32Editor.isValidUtf32());
        REQUIRE_EQUAL(StringConverter{invalidU8Editor}.toU32String(), expected[1]);
        REQUIRE_EQUAL(StringConverter{invalidU16Editor}.toU32String(), expected[1]);
        REQUIRE_EQUAL(U32String{invalidU32Editor}, expected[1]);
    }

    void testSensitiveUtf8MarkIsPreserved() {
        auto source = U8StringEditor{StringConverter{U32String{U"\uFB03"}}.toU8String()};
        source.markAsSensitive();

        const auto copy = source.normalized(NormalizationForm::Nfkc);
        REQUIRE(copy.isSensitive());
        const auto valueCopy = U8String{source}.normalized(NormalizationForm::Nfkc);
        REQUIRE(valueCopy.isSensitive());
        source.normalize(NormalizationForm::Nfkc);
        REQUIRE(source.isSensitive());
    }

    void testIdempotence() {
        const auto source = U32String{U"\u212B\uFB03\uAC01D\u0307\u0323"};
        for (
            const auto form :
            {NormalizationForm::Nfc, NormalizationForm::Nfd, NormalizationForm::Nfkc, NormalizationForm::Nfkd}) {
            const auto once = source.normalized(form);
            const auto twice = once.normalized(form);
            REQUIRE_EQUAL(twice, once);
            REQUIRE_EQUAL(twice.storageId(), once.storageId());
        }
    }

    void testCombiningMarkLimit() {
        auto thirtyMarks = U32StringEditor{U"A"};
        thirtyMarks.append(Char{U'\u0301'}, el::unit::CpLength{30});
        const auto accepted = U32String{thirtyMarks};
        auto acceptedComposed = U32StringEditor{U"\u00C1"};
        acceptedComposed.append(Char{U'\u0301'}, el::unit::CpLength{29});
        WITH_CONTEXT(requireAllWidths(accepted, NormalizationForm::Nfd, accepted));
        WITH_CONTEXT(requireAllWidths(accepted, NormalizationForm::Nfkd, accepted));
        WITH_CONTEXT(requireAllWidths(accepted, NormalizationForm::Nfc, U32String{acceptedComposed}));
        WITH_CONTEXT(requireAllWidths(accepted, NormalizationForm::Nfkc, U32String{acceptedComposed}));

        auto thirtyOneMarks = thirtyMarks;
        thirtyOneMarks.append(Char{U'\u0301'});
        for (
            const auto form :
            {NormalizationForm::Nfc, NormalizationForm::Nfd, NormalizationForm::Nfkc, NormalizationForm::Nfkd}) {
            WITH_CONTEXT(requireAllWidths(U32String{thirtyOneMarks}, form, U32String{U"\uFFFD"}));
        }

        auto leadingMarks = U32StringEditor{};
        leadingMarks.append(Char{U'\u0301'}, el::unit::CpLength{31});
        WITH_CONTEXT(requireAllWidths(U32String{leadingMarks}, NormalizationForm::Nfd, U32String{U"\uFFFD"}));

        thirtyOneMarks.append(Char{U'B'});
        WITH_CONTEXT(requireAllWidths(U32String{thirtyOneMarks}, NormalizationForm::Nfd, U32String{U"\uFFFDB"}));

        auto excessiveMarks = U32StringEditor{U"A"};
        excessiveMarks.append(Char{U'\u0301'}, el::unit::CpLength{1000});
        excessiveMarks.append(Char{U'B'});
        WITH_CONTEXT(requireAllWidths(U32String{excessiveMarks}, NormalizationForm::Nfd, U32String{U"\uFFFDB"}));
    }

    void testCombiningMarkLimitCountsDecomposition() {
        auto source = U32StringEditor{U"A"};
        source.append(Char{U'\u0301'}, el::unit::CpLength{29});
        source.append(Char{U'\u0344'}); // Decomposes into two non-starters.

        WITH_CONTEXT(requireAllWidths(U32String{source}, NormalizationForm::Nfd, U32String{U"\uFFFD"}));
        WITH_CONTEXT(requireAllWidths(U32String{source}, NormalizationForm::Nfc, U32String{U"\uFFFD"}));
    }

    void testRawCopyAroundChangedWindow() {
        auto source = U32StringEditor{};
        source.append(Char{U'A'}, el::unit::CpLength{4096});
        source.append(Char{U'\uFB03'});
        source.append(Char{U'Z'}, el::unit::CpLength{4096});

        auto expected = U32StringEditor{};
        expected.append(Char{U'A'}, el::unit::CpLength{4096});
        expected.append(U32String{U"ffi"});
        expected.append(Char{U'Z'}, el::unit::CpLength{4096});
        WITH_CONTEXT(requireAllWidths(U32String{source}, NormalizationForm::Nfkc, U32String{expected}));
    }
};
