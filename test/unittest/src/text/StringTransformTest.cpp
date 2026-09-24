// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/geometry/Alignment.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/SafeStringFlag.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/TruncateMode.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <string>
#include <string_view>

using namespace el::text::literals;

using el::geometry::Alignment;
using el::unit::CpLength;
using namespace el::text;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(
    StringEditor String U16StringEditor U16String U32StringEditor U32String TruncateMode SafeStringFlag SafeStringFlags
        StringSafeTransformTools)
class StringTransformTest final : public el::UnitTest {
public:
    void testU8TruncationModes() {

        auto text = StringEditor{"A¢€😀Z"_el};

        REQUIRE_EQUAL(StringConverter{text.truncated(CpLength{3U})}.toStdU8String(), std::u8string{u8"A¢€"});
        REQUIRE_EQUAL(
            StringConverter{text.truncated(CpLength{3U}, TruncateMode::Begin)}.toStdU8String(),
            std::u8string{u8"€😀Z"});
        REQUIRE_EQUAL(
            StringConverter{text.truncated(CpLength{3U}, TruncateMode::Middle)}.toStdU8String(),
            std::u8string{u8"A¢Z"});
        REQUIRE_EQUAL(
            StringConverter{text.truncated(CpLength{4U}, TruncateMode::Middle, "."_el)}.toStdU8String(),
            std::u8string{u8"A¢.Z"});
        REQUIRE_EQUAL(
            StringConverter{text.truncated(CpLength{2U}, TruncateMode::End, "..."_el)}.toStdU8String(),
            std::u8string{u8"A¢"});
        REQUIRE(text.truncated(CpLength::zero()).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{text.truncated(CpLength::infinite())}.toStdU8String(),
            StringConverter{text}.toStdU8String());

        text.truncate(CpLength{4U}, TruncateMode::Begin, "."_el);
        REQUIRE_EQUAL(StringConverter{text}.toStdU8String(), std::u8string{u8".€😀Z"});
    }

    void testNativeTruncationApis() {

        const auto u8Text = String{"A¢€😀Z"_el};
        REQUIRE_EQUAL(
            StringConverter{String{u8Text}.truncated(CpLength{4U}, TruncateMode::End, "."_el)}.toStdU8String(),
            std::u8string{u8"A¢€."});

        const auto u16Text = U16StringEditor{std::u16string_view{u"A¢€😀Z"}};
        REQUIRE_EQUAL(
            StringConverter{u16Text.truncated(CpLength{4U}, TruncateMode::End, u"…"_el)}.toStdU16String(),
            std::u16string{u"A¢€…"});
        REQUIRE_EQUAL(
            StringConverter{U16String{u16Text}.truncated(CpLength{3U}, TruncateMode::Middle)}.toStdU16String(),
            std::u16string{u"A¢Z"});

        const auto u32Text = U32StringEditor{std::u32string_view{U"A¢€😀Z"}};
        REQUIRE_EQUAL(
            StringConverter{u32Text.truncated(CpLength{4U}, TruncateMode::Begin, U"…"_el)}.toStdU32String(),
            std::u32string{U"…€😀Z"});
        REQUIRE_EQUAL(
            StringConverter{U32String{u32Text}.truncated(CpLength{4U}, TruncateMode::Middle, U"."_el)}.toStdU32String(),
            std::u32string{U"A¢.Z"});
    }

    void testAlignment() {
        const auto text = String{"Ab¢"_el};

        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{6U}, Alignment::Left, U'.')}.toStdU8String(),
            std::u8string{u8"Ab¢..."});
        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{6U}, Alignment::Right, U'.')}.toStdU8String(),
            std::u8string{u8"...Ab¢"});
        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{6U}, Alignment::Center, U'.')}.toStdU8String(),
            std::u8string{u8".Ab¢.."});
        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{6U}, Alignment::Top, U'.')}.toStdU8String(),
            std::u8string{u8"Ab¢..."});
        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{5U}, Alignment::Left, U'.')}.toStdU8String(),
            std::u8string{u8"Ab¢.."});
        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{5U}, Alignment::Right, U'.')}.toStdU8String(),
            std::u8string{u8"..Ab¢"});

        const auto u16Text = U16StringEditor{std::u16string_view{u"Ab¢"}};
        REQUIRE_EQUAL(
            StringConverter{u16Text.aligned(CpLength{5U}, Alignment::Right, U'.')}.toStdU16String(),
            std::u16string{u"..Ab¢"});

        const auto u32Text = U32StringEditor{std::u32string_view{U"Ab¢"}};
        REQUIRE_EQUAL(
            StringConverter{u32Text.aligned(CpLength{5U}, Alignment::Center, U'.')}.toStdU32String(),
            std::u32string{U".Ab¢."});
    }

    void testChangingTransformFunctionCannotOverflowReservation() {
        _changingTransformCall = 0U;
        auto u8Source = String{u8"Ab"_el};
        u8Source.markAsSensitive();
        const auto u8Result = u8Source.transformed(changingTransform);
        REQUIRE_EQUAL(StringConverter{u8Result}.toStdU32String(), std::u32string{U"😀😀"});
        REQUIRE(u8Result.isSensitive());

        _changingTransformCall = 0U;
        const auto u16Result = U16String{u"Ab"_el}.transformed(changingTransform);
        REQUIRE_EQUAL(StringConverter{u16Result}.toStdU32String(), std::u32string{U"😀😀"});

        _changingTransformCall = 0U;
        const auto u32Result = U32String{U"Ab"_el}.transformed(changingTransform);
        REQUIRE_EQUAL(StringConverter{u32Result}.toStdU32String(), std::u32string{U"😀😀"});
    }

    void testSafeString() {
        const auto text = String{"A\né Z"_el};

        REQUIRE_EQUAL(
            StringConverter{text.toSafeString(CpLength{100U})}.toStdU8String(), std::u8string{u8"\"A\\né Z\""});
        REQUIRE_EQUAL(
            StringConverter{text.toSafeString(CpLength{100U}, SafeStringFlag::OnlyAscii)}.toStdU8String(),
            std::u8string{u8"A\\n\\u00E9 Z"});
        REQUIRE_EQUAL(
            StringConverter{text.toSafeString(CpLength{100U}, SafeStringFlag::OnlyAscii | SafeStringFlag::AutoQuotes)}
                .toStdU8String(),
            std::u8string{u8"\"A\\n\\u00E9 Z\""});

        const auto longText = StringEditor{"abcdefghijklmnopqrstuvwxyz"_el};
        REQUIRE_EQUAL(
            StringConverter{longText.toSafeString(CpLength{20U})}.toStdU32String(),
            std::u32string{U"\"abcdefg…(26 total)\""});
        REQUIRE_EQUAL(StringConverter{longText.toSafeString(CpLength{3U})}.toStdU32String(), std::u32string{U"…"});

        const auto invalidUtf8 = String{th::stdStringFromHex("41 C0 42")};
        REQUIRE_EQUAL(
            StringConverter{invalidUtf8.toSafeString(
                                CpLength{20U}, SafeStringFlags{SafeStringFlag::OnlyAscii, SafeStringFlag::AutoQuotes})}
                .toStdString(),
            std::string{"\"A\\uFFFDB\""});

        const auto u16Text = U16StringEditor{std::u16string_view{u"A\né"}};
        REQUIRE_EQUAL(
            StringConverter{u16Text.toSafeString(CpLength{100U}, SafeStringFlag::OnlyAscii)}.toStdU16String(),
            std::u16string{u"A\\n\\u00E9"});

        const auto u32Text = U32StringEditor{std::u32string_view{U"A\né"}};
        REQUIRE_EQUAL(
            StringConverter{u32Text.toSafeString(CpLength{100U}, SafeStringFlag::OnlyAscii)}.toStdU32String(),
            std::u32string{U"A\\n\\u00E9"});
    }

    void testSafeStringCropFlags() {
        const auto text = String{"abcdefghijklmnopqrstuvwxyz"_el};

        requireSafeString(text, 20U, SafeStringFlag::Defaults, U"\"abcdefg…(26 total)\"");
        requireSafeString(
            text,
            20U,
            SafeStringFlags{SafeStringFlag::AddCropMark, SafeStringFlag::AddTotalsOnCrop},
            U"abcdefghi…(26 total)");
        requireSafeString(
            text,
            20U,
            SafeStringFlags{SafeStringFlag::AutoQuotes, SafeStringFlag::AddTotalsOnCrop},
            U"\"abcdefgh(26 total)\"");
        requireSafeString(
            text,
            20U,
            SafeStringFlags{SafeStringFlag::AutoQuotes, SafeStringFlag::AddCropMark},
            U"\"abcdefghijklmnopq…\"");
        requireSafeString(text, 20U, SafeStringFlag::AutoQuotes, U"abcdefghijklmnopqrst");
        requireSafeString(text, 20U, SafeStringFlag::None, U"abcdefghijklmnopqrst");
        requireSafeString(text, 3U, SafeStringFlag::Defaults, U"…");
        requireSafeString(text, 3U, SafeStringFlag::AddTotalsOnCrop, U"…");
        requireSafeString(text, 1U, SafeStringFlag::Defaults, U"…");
        requireSafeString(text, 0U, SafeStringFlag::Defaults, U"");
        requireSafeString(text, CpLength::infinite(), SafeStringFlag::Defaults, U"abcdefghijklmnopqrstuvwxyz");
    }

    void testSafeStringQuotesAndEscapes() {
        requireSafeString(String{"a b"_el}, 5U, SafeStringFlag::Defaults, U"\"a b\"");
        requireSafeString(String{"a b"_el}, 4U, SafeStringFlag::AutoQuotes, U"\"a \"");
        requireSafeString(String{"a\nb"_el}, 100U, SafeStringFlag::Defaults, U"\"a\\nb\"");
        requireSafeString(String{"a\\b"_el}, 100U, SafeStringFlag::Defaults, U"\"a\\\\b\"");
        requireSafeString(String{"a\"b"_el}, 100U, SafeStringFlag::Defaults, U"\"a\\\"b\"");
        requireSafeString(String{"a\nb"_el}, 100U, SafeStringFlag::None, U"a\\nb");
        requireSafeString(String{"a\nb"_el}, 2U, SafeStringFlag::None, U"a");
    }

    void testSafeStringAllWidthsAndValueKinds() {
        const auto expected = std::u32string{U"\"é😀…(20 total)\""};
        const auto u8View = std::u8string_view{u8"é😀abcdefghijklmnopqr"};
        const auto u16View = std::u16string_view{u"é😀abcdefghijklmnopqr"};
        const auto u32View = std::u32string_view{U"é😀abcdefghijklmnopqr"};

        requireSafeString(U8String{u8View}, 15U, SafeStringFlag::Defaults, expected);
        requireSafeString(U8StringEditor{u8View}, 15U, SafeStringFlag::Defaults, expected);
        requireSafeString(U16String{u16View}, 15U, SafeStringFlag::Defaults, expected);
        requireSafeString(U16StringEditor{u16View}, 15U, SafeStringFlag::Defaults, expected);
        requireSafeString(U32String{u32View}, 15U, SafeStringFlag::Defaults, expected);
        requireSafeString(U32StringEditor{u32View}, 15U, SafeStringFlag::Defaults, expected);
    }

    void testSafeStringStorageReuse() {
        auto u8 = String{"ordinary"_el};
        const auto u16 = U16String{u"ordinary"_el};
        const auto u32 = U32String{U"ordinary"_el};
        auto u8Editor = StringEditor{"ordinary"_el};
        const auto u16Editor = U16StringEditor{u"ordinary"_el};
        const auto u32Editor = U32StringEditor{U"ordinary"_el};

        REQUIRE_EQUAL(u8.toSafeString(CpLength{100U}).storageId(), u8.storageId());
        REQUIRE_EQUAL(u16.toSafeString(CpLength{100U}).storageId(), u16.storageId());
        REQUIRE_EQUAL(u32.toSafeString(CpLength{100U}).storageId(), u32.storageId());
        REQUIRE_EQUAL(u8Editor.toSafeString(CpLength{100U}).storageId(), u8Editor.storageId());
        REQUIRE_EQUAL(u16Editor.toSafeString(CpLength{100U}).storageId(), u16Editor.storageId());
        REQUIRE_EQUAL(u32Editor.toSafeString(CpLength{100U}).storageId(), u32Editor.storageId());

        u8.markAsSensitive();
        const auto sensitiveResult = u8.toSafeString(CpLength{100U});
        REQUIRE_EQUAL(sensitiveResult.storageId(), u8.storageId());
        REQUIRE(sensitiveResult.isSensitive());
        u8Editor.markAsSensitive();
        const auto sensitiveEditorResult = u8Editor.toSafeString(CpLength{100U});
        REQUIRE_EQUAL(sensitiveEditorResult.storageId(), u8Editor.storageId());
        REQUIRE(sensitiveEditorResult.isSensitive());

        const auto replacement = String{u8"A�B"_el};
        const auto replacementResult = replacement.toSafeString(CpLength{100U}, SafeStringFlag::None);
        REQUIRE_EQUAL(StringConverter{replacementResult}.toStdU32String(), std::u32string{U"A�B"});
        REQUIRE_NOT_EQUAL(replacementResult.storageId(), replacement.storageId());
        const auto replacement16 = U16String{u"A�B"_el};
        const auto replacement32 = U32String{U"A�B"_el};
        REQUIRE_NOT_EQUAL(
            replacement16.toSafeString(CpLength{100U}, SafeStringFlag::None).storageId(), replacement16.storageId());
        REQUIRE_NOT_EQUAL(
            replacement32.toSafeString(CpLength{100U}, SafeStringFlag::None).storageId(), replacement32.storageId());
    }

    void testSafeStringMalformedInput() {
        const auto invalidUtf8 = String{th::stdStringFromHex("41 C0 42")};
        const auto invalidUtf16Data = std::u16string{u'A', static_cast<char16_t>(0xD800U), u'B'};
        const auto invalidUtf32Data = std::u32string{U'A', static_cast<char32_t>(0x110000U), U'B'};

        requireSafeString(invalidUtf8, 100U, SafeStringFlag::None, U"A�B");
        requireSafeString(U16String{invalidUtf16Data}, 100U, SafeStringFlag::None, U"A�B");
        requireSafeString(U32String{invalidUtf32Data}, 100U, SafeStringFlag::None, U"A�B");
        requireSafeString(invalidUtf8, 100U, SafeStringFlag::OnlyAscii | SafeStringFlag::AutoQuotes, U"\"A\\uFFFDB\"");
    }

private:
    template <typename T>
    void requireSafeString(
        const T &text,
        const std::size_t maximumWidth,
        const SafeStringFlags flags,
        const std::u32string_view expected) {
        requireSafeString(text, CpLength::fromSizeT(maximumWidth), flags, expected);
    }

    template <typename T>
    void requireSafeString(
        const T &text, const CpLength maximumWidth, const SafeStringFlags flags, const std::u32string_view expected) {
        REQUIRE_EQUAL(StringConverter{text.toSafeString(maximumWidth, flags)}.toStdU32String(), expected);
    }

    static auto changingTransform(const Char) noexcept -> Char {
        const auto call = _changingTransformCall++;
        if (call == 0U) {
            return Char{U'a'};
        }
        if (call == 1U) {
            return Char::noCodePoint();
        }
        return Char{U'😀'};
    }

private:
    static inline std::size_t _changingTransformCall{};
};
