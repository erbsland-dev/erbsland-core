// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/AnyString.hpp>
#include <erbsland/AnyStringEditor.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/U16EncodingError.hpp>
#include <erbsland/U16Range.hpp>
#include <erbsland/U16StringEditor.hpp>
#include <erbsland/U32EncodingError.hpp>
#include <erbsland/U32StringEditor.hpp>
#include <erbsland/U8EncodingError.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <string_view>

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

TESTED_TARGETS(
    U8String U8StringEditor U8StringLiteral U8StringReadTools U16String U16StringEditor U16StringLiteral
        U16StringReadTools U32String U32StringEditor U32StringLiteral U32StringReadTools AnyString AnyStringEditor)
class StringEncodingValidationTest final : public el::UnitTest {
private:
    template <typename T>
    void verifyValid(const T &text) {
        REQUIRE(text.isEncodingValid());
        REQUIRE_NOTHROW(text.encodingValidOrThrow());
    }

    template <typename Error, typename T, typename Index>
    void verifyInvalid(const T &text, const Index expectedIndex) {
        REQUIRE_FALSE(text.isEncodingValid());
        try {
            text.encodingValidOrThrow();
            REQUIRE(false);
        } catch (const Error &error) {
            REQUIRE_EQUAL(error.index(), expectedIndex);
            REQUIRE_FALSE(error.reason().isEmpty());
        }
        REQUIRE_FALSE(text.isEncodingValid());
    }

    template <typename StringType, typename EditorType>
    void verifyValidForms(const StringType &text, const EditorType &editor) {
        WITH_CONTEXT(verifyValid(text));
        WITH_CONTEXT(verifyValid(editor));
        WITH_CONTEXT(verifyValid(AnyString{text}));
        WITH_CONTEXT(verifyValid(AnyStringEditor{editor}));
    }

    template <typename Error, typename StringType, typename EditorType, typename Index>
    void verifyInvalidForms(const StringType &text, const EditorType &editor, const Index expectedIndex) {
        WITH_CONTEXT(verifyInvalid<Error>(text, expectedIndex));
        WITH_CONTEXT(verifyInvalid<Error>(editor, expectedIndex));
        WITH_CONTEXT(verifyInvalid<Error>(AnyString{text}, expectedIndex));
        WITH_CONTEXT(verifyInvalid<Error>(AnyStringEditor{editor}, expectedIndex));
    }

public:
    void testEmptyAndValidStrings() {
        WITH_CONTEXT(verifyValidForms(U8String{}, U8StringEditor{}));
        WITH_CONTEXT(verifyValidForms(U16String{}, U16StringEditor{}));
        WITH_CONTEXT(verifyValidForms(U32String{}, U32StringEditor{}));
        WITH_CONTEXT(verifyValid(AnyString{}));
        WITH_CONTEXT(verifyValid(AnyStringEditor{}));
        WITH_CONTEXT(verifyValidForms(U8String{u8"A\0é😀"_el}, U8StringEditor{u8"A\0é😀"_el}));
        WITH_CONTEXT(verifyValidForms(U16String{u"A\0é😀"_el}, U16StringEditor{u"A\0é😀"_el}));
        WITH_CONTEXT(verifyValidForms(U32String{U"A\0é😀"_el}, U32StringEditor{U"A\0é😀"_el}));
    }

    void testUtf8ErrorsReportBytePositions() {
        struct TestCase final {
            std::string_view hex;
            std::size_t index;
        };
        const auto cases = std::array{
            TestCase{"41ff", 1U},
            TestCase{"41c080", 1U},
            TestCase{"41e282", 3U},
            TestCase{"41e228a1", 2U},
            TestCase{"41eda080", 1U},
            TestCase{"41f4908080", 1U},
            TestCase{"41c3a9ff", 3U}};
        for (const auto &testCase : cases) {
            const auto bytes = el::unittest::th::stdStringFromHex(testCase.hex);
            const auto editor = U8StringEditor{std::string_view{bytes}};
            const auto text = U8String{editor};
            const auto storage = text.storageId();
            WITH_CONTEXT(verifyInvalidForms<U8EncodingError>(text, editor, ByteIndex::fromSizeT(testCase.index)));
            REQUIRE_EQUAL(text.storageId(), storage);
        }
    }

    void testUtf16ErrorsReportCodeUnitPositions() {
        for (const auto malformed : {char16_t{0xd800U}, char16_t{0xdc00U}}) {
            const auto units = std::array{u'A', char16_t{0xd83dU}, char16_t{0xde00U}, malformed};
            const auto editor = U16StringEditor{std::u16string_view{units.data(), units.size()}};
            WITH_CONTEXT(verifyInvalidForms<U16EncodingError>(U16String{editor}, editor, U16Index{3U}));
        }
        const auto units = std::array{u'A', char16_t{0xd800U}, u'B'};
        const auto editor = U16StringEditor{std::u16string_view{units.data(), units.size()}};
        WITH_CONTEXT(verifyInvalidForms<U16EncodingError>(U16String{editor}, editor, U16Index{1U}));
    }

    void testUtf32ErrorsReportCodePointPositions() {
        for (const auto malformed : {char32_t{0xd800U}, char32_t{0x110000U}}) {
            const auto units = std::array{U'A', U'😀', malformed};
            const auto editor = U32StringEditor{std::u32string_view{units.data(), units.size()}};
            WITH_CONTEXT(verifyInvalidForms<U32EncodingError>(U32String{editor}, editor, CpIndex{2U}));
        }
    }

    void testSlicePositionsAreRelativeToVisibleText() {
        const auto bytes = el::unittest::th::stdStringFromHex("ff4142ff43");
        const auto u8 = U8String{std::string_view{bytes}};
        const auto u8Slice = u8.slice({ByteIndex{2U}, ByteCount{3U}});
        WITH_CONTEXT(verifyInvalidForms<U8EncodingError>(u8Slice, U8StringEditor{u8Slice}, ByteIndex{1U}));
        WITH_CONTEXT(verifyValid(u8.slice({ByteIndex{1U}, ByteCount{2U}})));

        const auto u16Units = std::array{char16_t{0xdc00U}, u'A', u'B', char16_t{0xdc00U}, u'C'};
        const auto u16 = U16String{std::u16string_view{u16Units.data(), u16Units.size()}};
        const auto u16Slice = u16.slice(U16Range{U16Index{2U}, U16Count{3U}});
        WITH_CONTEXT(verifyInvalidForms<U16EncodingError>(u16Slice, U16StringEditor{u16Slice}, U16Index{1U}));
        WITH_CONTEXT(verifyValid(u16.slice(U16Range{U16Index{1U}, U16Count{2U}})));

        const auto u32Units = std::array{char32_t{0x110000U}, U'A', U'B', char32_t{0xd800U}, U'C'};
        const auto u32 = U32String{std::u32string_view{u32Units.data(), u32Units.size()}};
        const auto u32Slice = u32.slice(CpRange{CpIndex{2U}, CpCount{3U}});
        WITH_CONTEXT(verifyInvalidForms<U32EncodingError>(u32Slice, U32StringEditor{u32Slice}, CpIndex{1U}));
        WITH_CONTEXT(verifyValid(u32.slice(CpRange{CpIndex{1U}, CpCount{2U}})));
    }

    void testLiteralValidation() {
        WITH_CONTEXT(verifyValid(""_el));
        WITH_CONTEXT(verifyValid(u8"é😀"_el));
        WITH_CONTEXT(verifyValid(u"é😀"_el));
        WITH_CONTEXT(verifyValid(U"é😀"_el));
        static constexpr char cInvalidU8[] = {'A', static_cast<char>(0xffU), '\0'};
        static constexpr char8_t cInvalidChar8[] = {u8'A', char8_t{0xffU}, u8'\0'};
        static constexpr char16_t cInvalidU16[] = {u'A', char16_t{0xdc00U}, u'\0'};
        static constexpr char32_t cInvalidU32[] = {U'A', char32_t{0x110000U}, U'\0'};
        WITH_CONTEXT(verifyInvalid<U8EncodingError>(U8StringLiteral{cInvalidU8}, ByteIndex{1U}));
        WITH_CONTEXT(verifyInvalid<U8EncodingError>(U8StringLiteral{cInvalidChar8}, ByteIndex{1U}));
        WITH_CONTEXT(verifyInvalid<U16EncodingError>(U16StringLiteral{cInvalidU16}, U16Index{1U}));
        WITH_CONTEXT(verifyInvalid<U32EncodingError>(U32StringLiteral{cInvalidU32}, CpIndex{1U}));
    }
};
