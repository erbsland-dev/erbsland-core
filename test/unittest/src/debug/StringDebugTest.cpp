// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/debug/StringDebug.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

using el::debug::DebugViewDetail;
using el::debug::toDebugString;
using el::text::StringConverter;
using el::text::StringView;
using el::text::U16String;
using el::text::U32String;
using el::text::U8String;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;

TESTED_TARGETS(DebugViewDetail DebugViewDetails StringDebug)
class StringDebugTest final : public el::UnitTest {
public:
    void testContentsPreview() {
        const auto text = U8String{std::u8string_view{u8"A\né"}};
        const auto details = DebugViewDetail::ContentInTitle | DebugViewDetail::CoreDetails;
        const auto output = StringConverter{toDebugString(text, details)}.toStdString();

        REQUIRE(containsText(output, "U8String(\"A\\né\")"));
        REQUIRE(containsText(output, "isEmpty: false"));
        REQUIRE(containsText(output, "isEncodingValid: true"));
        REQUIRE_FALSE(containsText(output, "backingStorageId"));
    }

    void testMemoryDetailsForLiteralViewAndSlice() {
        using namespace el::text::literals;

        const auto text = StringView{"abcdef"_el};
        const auto slice = text.slice(ByteRange{ByteIndex{1U}, ByteLength{3U}});
        const auto details = DebugViewDetail::BackingStore | DebugViewDetail::CoreDetails | DebugViewDetail::StorageId;
        const auto textOutput = StringConverter{toDebugString(text, details)}.toStdString();
        const auto sliceOutput = StringConverter{toDebugString(slice, details)}.toStdString();

        REQUIRE(containsText(textOutput, "storageKind: literal"));
        REQUIRE_EQUAL(lineValue(textOutput, "backingStorageId"), lineValue(sliceOutput, "backingStorageId"));
        REQUIRE_NOT_EQUAL(lineValue(textOutput, "storageId"), lineValue(sliceOutput, "storageId"));
        REQUIRE(containsText(sliceOutput, "index: 1"));
        REQUIRE(containsText(sliceOutput, "length: 3"));
    }

    void testUtf16AndUtf32Contents() {
        const auto u16Text = U16String{std::u16string_view{u"β\n"}};
        const auto u32Text = U32String{std::u32string_view{U"中"}};

        REQUIRE(
            StringConverter{toDebugString(u16Text, DebugViewDetail::ContentInTitle)}.toStdString().find(
                "U16String(\"β\\n\")") != std::string::npos);
        REQUIRE(
            StringConverter{toDebugString(u32Text, DebugViewDetail::ContentInTitle)}.toStdString().find(
                "U32String(中)") != std::string::npos);
    }

private:
    [[nodiscard]] static auto lineValue(const std::string &text, const std::string &label) -> std::string {
        const auto prefix = label + ": ";
        auto position = std::size_t{0U};
        while (position < text.size()) {
            const auto lineEnd = text.find('\n', position);
            const auto length = lineEnd == std::string::npos ? std::string::npos : lineEnd - position;
            auto line = text.substr(position, length);
            const auto first = line.find_first_not_of(' ');
            if (first != std::string::npos) {
                line = line.substr(first);
            }
            if (line.starts_with(prefix)) {
                return line.substr(prefix.size());
            }
            if (lineEnd == std::string::npos) {
                break;
            }
            position = lineEnd + 1U;
        }
        return {};
    }

    [[nodiscard]] static auto containsText(const std::string &text, const std::string &needle) noexcept -> bool {
        return text.find(needle) != std::string::npos;
    }
};
