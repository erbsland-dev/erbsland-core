// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Alignment.hpp>
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

#include <string>
#include <string_view>
#include <type_traits>

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;
using el::geometry::Alignment;

TESTED_TARGETS(String StringEditor U16String U16StringEditor U32String U32StringEditor)
class StringUnchangedResultTest final : public el::UnitTest {
public:
    void testU8Variants() { WITH_CONTEXT((requireVariants<String, StringEditor>(String{u8"--a😀b--"_el}))); }

    void testU16Variants() { WITH_CONTEXT((requireVariants<U16String, U16StringEditor>(U16String{u"--a😀b--"_el}))); }

    void testU32Variants() { WITH_CONTEXT((requireVariants<U32String, U32StringEditor>(U32String{U"--a😀b--"_el}))); }

    void testEscapingVariants() {
        WITH_CONTEXT((requireEscapingVariants<String, StringEditor>(String{"abc"_el})));
        WITH_CONTEXT((requireEscapingVariants<U16String, U16StringEditor>(U16String{u"abc"_el})));
        WITH_CONTEXT((requireEscapingVariants<U32String, U32StringEditor>(U32String{U"abc"_el})));
    }

    void testUnchangedUnicodeEscaping() {
        WITH_CONTEXT((requireUnicodeEscaping<String, StringEditor>(String{u8"a😀b�"_el})));
        WITH_CONTEXT((requireUnicodeEscaping<U16String, U16StringEditor>(U16String{u"a😀b�"_el})));
        WITH_CONTEXT((requireUnicodeEscaping<U32String, U32StringEditor>(U32String{U"a😀b�"_el})));
    }

    void testMalformedEscaping() {
        const auto u8 = String{std::string_view{el::unittest::th::stdStringFromHex("61 c0 62")}};
        const auto u16Data = std::u16string{u'a', static_cast<char16_t>(0xD800U), u'b'};
        const auto u16 = U16String{std::u16string_view{u16Data}};
        const auto u32Data = std::u32string{U'a', static_cast<char32_t>(0x110000U), U'b'};
        const auto u32 = U32String{std::u32string_view{u32Data}};
        WITH_CONTEXT(requireMalformed(u8));
        WITH_CONTEXT(requireMalformed(StringEditor{u8}));
        WITH_CONTEXT(requireMalformed(u16));
        WITH_CONTEXT(requireMalformed(U16StringEditor{u16}));
        WITH_CONTEXT(requireMalformed(u32));
        WITH_CONTEXT(requireMalformed(U32StringEditor{u32}));
        // A range replacement compares native data, even when decoded characters are equal.
        WITH_CONTEXT(requireUnchanged(
            u8, u8.replaced(CpRange{CpIndex{1U}, CpCount::one()}, u8.slice(CpRange{CpIndex{1U}, CpCount::one()}))));
        const auto repaired = u8.replacedFirst(u8"�"_el, u8"�"_el);
        REQUIRE_NOT_EQUAL(repaired.storageId(), u8.storageId());
        REQUIRE_EQUAL(repaired, String{u8"a�b"_el});
    }

    void testSensitiveResults() {
        auto source = String{u8"a😀b"_el};
        source.markAsSensitive();
        WITH_CONTEXT((requireOperations<String, StringEditor>(source)));
        WITH_CONTEXT((requireOperations<String, StringEditor>(StringEditor{source})));
        WITH_CONTEXT(requireSensitiveEscaping(source));
        WITH_CONTEXT(requireSensitiveEscaping(StringEditor{source}));
        auto malformed = String{std::string_view{el::unittest::th::stdStringFromHex("61 c0 62")}};
        malformed.markAsSensitive();
        REQUIRE(malformed.toEscaped(EscapeFormat::Json).isSensitive());
        REQUIRE(StringEditor{malformed}.toEscaped(EscapeFormat::Json).isSensitive());
    }

    void testSharedEditorResultsRemainIndependent() {
        WITH_CONTEXT((requireIndependentEditor<String, StringEditor>()));
        WITH_CONTEXT((requireIndependentEditor<U16String, U16StringEditor>()));
        WITH_CONTEXT((requireIndependentEditor<U32String, U32StringEditor>()));
    }

private:
    template <typename T>
    void requireUnchanged(const T &source, const T &result) {
        REQUIRE_EQUAL(result, source);
        REQUIRE_EQUAL(result.storageId(), source.storageId());
    }

    template <typename tReadOnly, typename tEditor>
    void requireVariants(const tReadOnly &literal) {
        const auto shared = literal.copy();
        const auto range = CpRange{CpIndex{2U}, CpCount{3U}};
        for (const auto &source : {literal, shared, literal.slice(range), shared.slice(range), tReadOnly{}}) {
            WITH_CONTEXT((requireOperations<tReadOnly, tEditor>(source)));
            WITH_CONTEXT((requireOperations<tReadOnly, tEditor>(tEditor{source})));
        }
    }

    template <typename tReadOnly, typename tEditor, typename T>
    void requireOperations(const T &source) {
        const auto missing = tReadOnly{tEditor{}.append(U'x')};
        const auto letter = tReadOnly{tEditor{}.append(U'a')};
        const auto emptyRange = CpRange::emptyAt(CpIndex::zero());
        const auto outside = CpRange{CpIndex{99U}, CpCount::one()};
        WITH_CONTEXT(requireUnchanged(source, source.removed(emptyRange)));
        WITH_CONTEXT(requireUnchanged(source, source.removed(CpRange::noRange())));
        WITH_CONTEXT(requireUnchanged(source, source.removed(outside)));
        WITH_CONTEXT(requireUnchanged(source, source.removedFirst(missing)));
        WITH_CONTEXT(requireUnchanged(source, source.removedFirst(tReadOnly{})));
        WITH_CONTEXT(requireUnchanged(source, source.inserted(CpIndex::zero(), tReadOnly{})));
        WITH_CONTEXT(requireUnchanged(source, source.inserted(CpIndex{99U}, tReadOnly{})));
        WITH_CONTEXT(requireUnchanged(source, source.inserted(CpIndex::noIndex(), letter)));
        WITH_CONTEXT(requireUnchanged(source, source.replaced(emptyRange, tReadOnly{})));
        WITH_CONTEXT(requireUnchanged(source, source.replaced(outside, tReadOnly{})));
        WITH_CONTEXT(requireUnchanged(source, source.replaced(CpRange::noRange(), letter)));
        const auto firstRange = CpRange{CpIndex::zero(), CpCount::one()};
        const auto first = tReadOnly{source.slice(firstRange)};
        WITH_CONTEXT(requireUnchanged(source, source.replaced(firstRange, first)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedFirst(first, first)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedFirst(first, first, Char::compareCaseFolded)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedFirst(missing, letter)));
        WITH_CONTEXT(requireUnchanged(source, source.replacedFirst(tReadOnly{}, letter)));
        WITH_CONTEXT((requireNativeOperations<tReadOnly>(source, letter)));
        for (const auto mode : {TruncateMode::Begin, TruncateMode::Middle, TruncateMode::End}) {
            WITH_CONTEXT(requireUnchanged(source, source.truncated(CpCount::infinite(), mode)));
            WITH_CONTEXT(requireUnchanged(source, source.truncated(CpCount{100U}, mode)));
            WITH_CONTEXT(requireUnchanged(source, source.truncated(source.characterLength(), mode, missing)));
        }
        for (const auto alignment : {Alignment::Left, Alignment::HCenter, Alignment::Right}) {
            WITH_CONTEXT(requireUnchanged(source, source.aligned(source.characterLength(), alignment)));
            WITH_CONTEXT(requireUnchanged(source, source.aligned(CpCount::zero(), alignment)));
            WITH_CONTEXT(requireUnchanged(source, source.aligned(CpCount{99U}, alignment, Char::endOfData())));
        }
        if (source.isEmpty()) {
            WITH_CONTEXT(requireUnchanged(source, source.truncated(CpCount::zero())));
        } else {
            REQUIRE(source.truncated(CpCount::zero()).isEmpty());
            const auto kept = source.kept(CpRange{CpIndex::zero(), source.characterLength()});
            REQUIRE_EQUAL(kept, source);
            REQUIRE_NOT_EQUAL(kept.storageId(), source.storageId());
            if (source.characterLength() > CpCount::one()) {
                REQUIRE_EQUAL(source.truncated(CpCount::one()).characterLength(), CpCount::one());
                REQUIRE_NOT_EQUAL(source.truncated(CpCount::one()).storageId(), source.storageId());
            }
            REQUIRE_NOT_EQUAL(source.inserted(CpIndex::zero(), letter).storageId(), source.storageId());
            REQUIRE_NOT_EQUAL(source.removed(firstRange).storageId(), source.storageId());
            REQUIRE_NOT_EQUAL(source.removedFirst(first).storageId(), source.storageId());
            REQUIRE_NOT_EQUAL(source.replaced(firstRange, missing).storageId(), source.storageId());
            REQUIRE_NOT_EQUAL(source.replacedFirst(first, missing).storageId(), source.storageId());
            REQUIRE_NOT_EQUAL(source.aligned(CpCount{99U}, Alignment::Left).storageId(), source.storageId());
        }
    }

    template <typename tReadOnly, typename T>
    void requireNativeOperations(const T &source, const tReadOnly &letter) {
        if constexpr (!std::is_same_v<tReadOnly, U32String>) {
            using tRange = std::conditional_t<std::is_same_v<tReadOnly, String>, ByteRange, U16Range>;
            using tIndex = std::conditional_t<std::is_same_v<tReadOnly, String>, ByteIndex, U16Index>;
            using tLength = std::conditional_t<std::is_same_v<tReadOnly, String>, ByteCount, U16Count>;
            WITH_CONTEXT(requireUnchanged(source, source.removed(tRange::emptyAt(tIndex::zero()))));
            WITH_CONTEXT(requireUnchanged(source, source.removed(tRange::noRange())));
            WITH_CONTEXT(requireUnchanged(source, source.removed(tRange{tIndex{99U}, tLength::one()})));
            WITH_CONTEXT(requireUnchanged(source, source.inserted(tIndex::zero(), tReadOnly{})));
            WITH_CONTEXT(requireUnchanged(source, source.inserted(tIndex::noIndex(), letter)));
            WITH_CONTEXT(requireUnchanged(source, source.replaced(tRange::noRange(), letter)));
            WITH_CONTEXT(
                requireUnchanged(source, source.replaced(tRange{tIndex::zero(), source.length()}, tReadOnly{source})));
        }
    }

    template <typename tReadOnly, typename tEditor>
    void requireEscapingVariants(const tReadOnly &literal) {
        const auto shared = literal.copy();
        const auto range = CpRange{CpIndex{1U}, CpCount{3U}};
        for (const auto &source : {literal, shared, literal.slice(range), shared.slice(range), tReadOnly{}}) {
            WITH_CONTEXT(requireEscaping(source));
            WITH_CONTEXT(requireEscaping(tEditor{source}));
        }
        const auto needsEscape = tReadOnly{tEditor{}.append(U'a').append(U'"').append(U'\n')};
        const auto expected = std::u32string{U"a\\\"\\n"};
        REQUIRE_EQUAL(StringConverter{needsEscape.toEscaped(EscapeFormat::Json)}.toStdU32String(), expected);
        REQUIRE_EQUAL(StringConverter{tEditor{needsEscape}.toEscaped(EscapeFormat::Json)}.toStdU32String(), expected);
    }

    template <typename tReadOnly, typename tEditor>
    void requireUnicodeEscaping(const tReadOnly &literal) {
        const auto shared = literal.copy();
        const auto range = CpRange{CpIndex{1U}, CpCount{3U}};
        for (const auto &source : {literal, shared, literal.slice(range), shared.slice(range)}) {
            for (
                const auto format :
                {EscapeFormat::Html,
                    EscapeFormat::Json,
                    EscapeFormat::Cpp,
                    EscapeFormat::Xml,
                    EscapeFormat::RegEx,
                    EscapeFormat::Display,
                    EscapeFormat::Log,
                    EscapeFormat::Markdown}) {
                WITH_CONTEXT(requireUnchanged(source, source.toEscaped(format)));
                const auto editor = tEditor{source};
                WITH_CONTEXT(requireUnchanged(editor, editor.toEscaped(format)));
            }
        }
    }

    template <typename T>
    void requireEscaping(const T &source) {
        WITH_CONTEXT(requireUnchanged(source, source.toEscaped(EscapeFormat::None)));
        for (
            const auto format :
            {EscapeFormat::Html,
                EscapeFormat::Json,
                EscapeFormat::Cpp,
                EscapeFormat::Xml,
                EscapeFormat::RegEx,
                EscapeFormat::Display,
                EscapeFormat::Log,
                EscapeFormat::Config,
                EscapeFormat::ConfigTest,
                EscapeFormat::Markdown}) {
            WITH_CONTEXT(requireUnchanged(source, source.toEscaped(format)));
            WITH_CONTEXT(requireUnchanged(source, source.toEscaped(format, EscapeAmount::Nothing)));
        }
    }

    template <typename T>
    void requireMalformed(const T &source) {
        WITH_CONTEXT(requireUnchanged(source, source.toEscaped(EscapeFormat::None)));
        WITH_CONTEXT(requireUnchanged(source, source.toEscaped(EscapeFormat::Json, EscapeAmount::Nothing)));
        const auto rawRange = CpRange{CpIndex{1U}, CpCount::one()};
        WITH_CONTEXT(requireUnchanged(source, source.replaced(rawRange, source.slice(rawRange))));
        const auto repaired = source.toEscaped(EscapeFormat::Json);
        REQUIRE_NOT_EQUAL(repaired.storageId(), source.storageId());
        REQUIRE_EQUAL(StringConverter{repaired}.toStdU32String(), std::u32string{U"a�b"});
        WITH_CONTEXT(requireUnchanged(repaired, repaired.toEscaped(EscapeFormat::Json)));
    }

    template <typename T>
    void requireSensitiveEscaping(const T &source) {
        WITH_CONTEXT(requireUnchanged(source, source.toEscaped(EscapeFormat::Json)));
        REQUIRE(source.toEscaped(EscapeFormat::None).isSensitive());
        REQUIRE(source.toEscaped(EscapeFormat::Json).isSensitive());
        REQUIRE(source.toEscaped(EscapeFormat::Json, EscapeAmount::Nothing).isSensitive());
        auto escapedSource = T{u8"prefix😀\"\n"_el};
        escapedSource.markAsSensitive();
        const auto escaped = escapedSource.toEscaped(EscapeFormat::Json);
        REQUIRE(escaped.isSensitive());
        REQUIRE_NOT_EQUAL(escaped.storageId(), escapedSource.storageId());
        REQUIRE_EQUAL(StringConverter{escaped}.toStdU32String(), std::u32string{U"prefix😀\\\"\\n"});
        const auto slice = escapedSource.slice(CpRange{CpIndex{6U}, CpCount{3U}});
        REQUIRE(slice.toEscaped(EscapeFormat::Json).isSensitive());
        REQUIRE_EQUAL(
            StringConverter{slice.toEscaped(EscapeFormat::Json)}.toStdU32String(), std::u32string{U"😀\\\"\\n"});
    }

    template <typename tReadOnly, typename tEditor>
    void requireIndependentEditor() {
        const auto source = tReadOnly{tEditor{}.append(U'a', CpCount{8U})};
        const auto sourceEditor = tEditor{source};
        auto result = sourceEditor.toEscaped(EscapeFormat::Json);
        WITH_CONTEXT(requireUnchanged(sourceEditor, result));
        result.append(U'b');
        REQUIRE_NOT_EQUAL(result.storageId(), source.storageId());
        REQUIRE_EQUAL(source.characterLength(), CpCount{8U});
        REQUIRE_EQUAL(sourceEditor.characterLength(), CpCount{8U});
        REQUIRE_EQUAL(result.characterLength(), CpCount{9U});
    }
};
