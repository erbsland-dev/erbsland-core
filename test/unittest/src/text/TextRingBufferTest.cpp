// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/text/TextRingBuffer.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(TextRingBuffer)
class TextRingBufferTest final : public el::UnitTest {
public:
    void testEncodingMatchesStringEncoder() {
        const auto source = el::text::String{"A—😀"_el};
        for (
            const auto encoding :
            {el::text::StringEncoding::Utf8,
                el::text::StringEncoding::Utf16LittleEndian,
                el::text::StringEncoding::Utf16BigEndian,
                el::text::StringEncoding::Utf32LittleEndian,
                el::text::StringEncoding::Utf32BigEndian}) {
            auto buffer = el::text::TextRingBuffer{el::unit::ByteLength{4U}, el::unit::ByteLength{128U}};
            REQUIRE(buffer.writeEncoded(source, encoding, el::text::StringBomMode::Require));
            const auto expected = el::text::StringEncoder{source}.encode(encoding, el::text::StringBomMode::Require);
            REQUIRE_EQUAL(buffer.read(el::unit::ByteLength::infinite()), expected);
        }
    }

    void testFailedWriteDoesNotModifyRing() {
        auto buffer = el::text::TextRingBuffer{el::unit::ByteLength{4U}};
        REQUIRE(buffer.writeEncoded("A"_el, el::text::StringEncoding::Utf8));
        REQUIRE_FALSE(buffer.writeEncoded("too long"_el, el::text::StringEncoding::Utf8));
        REQUIRE_EQUAL(buffer.length(), el::unit::ByteLength{1U});
    }
};
