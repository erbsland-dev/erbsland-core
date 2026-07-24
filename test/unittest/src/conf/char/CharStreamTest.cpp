// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/char/CharClass.hpp>
#include <erbsland/conf/impl/char/CharStream.hpp>
#include <erbsland/conf/impl/char/NamedChars.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/mem/impl/UnsafeByteBlockAccess.hpp>
#include <erbsland/unit/CodeLocation.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using el::conf::impl::CharClass;
using el::conf::impl::CharStream;
using el::conf::impl::CharStreamPtr;
using el::conf::impl::DecodedChar;
using namespace el::text::literals;
namespace nc = el::conf::impl::nc;

TESTED_TARGETS(Decoder DecodedChar)
class CharStreamTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    CharStreamPtr decoder;
    DecodedChar decodedChar;

    void requireLocation(const DecodedChar &character, const std::size_t line, const std::size_t column) {
        REQUIRE_EQUAL(character.codeLocation().line(), el::unit::LineIndex::fromSizeT(line));
        REQUIRE_EQUAL(character.codeLocation().column(), el::unit::ColumnIndex::fromSizeT(column));
        REQUIRE_FALSE(character.codeLocation().position().isNoIndex());
    }

    void tearDown() override { cleanUpTestFileDirectory(); }

    void testConstruction() {
        const auto testFile = createTestFile("[main]"_el);
        auto source = Source::fromFile(el::path::Path{testFile});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        REQUIRE(decoder != nullptr);
    }

    void requireMatchingAsciiData(const el::text::String &testData) {
        el::unit::CodeLocation pos{
            el::unit::LineIndex::zero(), el::unit::ColumnIndex::zero(), el::unit::CpIndex::zero()};
        auto testReader = el::text::StringCharReader{testData};
        for (auto testCharacter = testReader.read(); !testCharacter.isEndOfData(); testCharacter = testReader.read()) {
            decodedChar = decoder->next();
            if (testCharacter == el::text::Char{U'\n'}) {
                REQUIRE(decodedChar.character() == CharClass::LineBreak);
                REQUIRE(decodedChar.character() == U'\n');
                REQUIRE(decodedChar.codeLocation() == pos);
                pos.nextLine();
            } else {
                REQUIRE(decodedChar.character() == testCharacter);
                el::text::StringEditor decodedString;
                decodedString.append(decodedChar.character());
                auto expectedStr = el::text::StringEditor::fromCharacter(testCharacter);
                REQUIRE_EQUAL(decodedString, expectedStr);
                REQUIRE(decodedChar.codeLocation() == pos);
                pos.nextColumn();
            }
        }
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().isEndOfData());
        // At end, each further call of `next` shall return end of data.
        REQUIRE_NOTHROW(decodedChar = decoder->next());
        REQUIRE(decodedChar.character().isEndOfData());
    }

    void testBasicAsciiFileDecode() {
        el::text::String testData = "[main]\nkey: \"test\"\r\nlast"_el;
        const auto testFile = createTestFile(el::text::String{testData});
        auto source = Source::fromFile(el::path::Path{testFile});
        REQUIRE(source != nullptr);
        REQUIRE_NOTHROW(source->open());
        REQUIRE(source->name() == "file"_el);
        decoder = CharStream::create(source);
        REQUIRE(decoder != nullptr);
        WITH_CONTEXT(requireMatchingAsciiData(testData))
    }

    void testBasicAsciiStringDecode() {
        el::text::String testData = "[main]\nkey: \"test\"\r\nlast"_el;
        // std::u8string
        auto source = Source::fromString(el::text::String{testData});
        REQUIRE(source != nullptr);
        REQUIRE_NOTHROW(source->open());
        REQUIRE(source->name() == "text"_el);
        REQUIRE(source->path().isEmpty());
        REQUIRE(source->identifier()->toText() == "text"_el);
        decoder = CharStream::create(source);
        REQUIRE(decoder != nullptr);
        WITH_CONTEXT(requireMatchingAsciiData(testData))
    }

    void testValidUtf8Sequences() {
        // × = C3 97 = U+00D7
        // ← = E2 86 90 = U+2190
        // 😄 = F0 9F 98 84 = U+1F604
        const auto testData = el::text::String{"×←😄"_el};
        const auto testFile = createTestFile(testData);
        auto source = Source::fromFile(el::path::Path{testFile});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        REQUIRE(decoder != nullptr);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == 0x00D7U);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == 0x2190U);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == 0x1F604U);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().isEndOfData());
    }
    void testBom() {
        const auto content = el::mem::ByteBlock::fromVector(
            std::vector<uint8_t>{
                0xEFU,
                0xBBU,
                0xBFU, // utf-8 BOM
                0x41U, // a
                0x42U, // b
            });
        const auto testFile = createTestFile(content);
        auto source = Source::fromFile(el::path::Path{testFile});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        REQUIRE(decoder != nullptr);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == 0x41U);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == 0x42U);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().isEndOfData());
    }

    template <typename T>
    void requireErrorAfterValidA(const T &content, ConfErrorCategory expectedErrorCategory) {
        auto source = [&]() -> SourcePtr {
            if constexpr (std::is_same_v<T, el::mem::ByteBlock>) {
                const auto byteSpan = el::mem::impl::UnsafeByteBlockAccess{content}.data();
                return Source::fromString(
                    el::text::String{
                        std::string_view{reinterpret_cast<const char *>(byteSpan.data()), byteSpan.size()}});
            } else {
                return Source::fromString(content);
            }
        }();
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        REQUIRE(decoder != nullptr);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == 0x41U);
        try {
            decodedChar = decoder->next();
            REQUIRE(false);
        } catch (ConfError &e) {
            REQUIRE(e.category() == expectedErrorCategory);
        }
    }

    void testInvalidUtf8Sequences() {
        auto content = el::mem::ByteBuffer{
            el::mem::Byte(0x41),        // a
            el::mem::Byte(0b11110100U), // => 1'0011'1111'1111'1111'1111 = 0x13FFFF
            el::mem::Byte(0b10111111U), // ConfError, because it exceeds the valid unicode range.
            el::mem::Byte(0b10111111U),
            el::mem::Byte(0b10111111U),
            el::mem::Byte(0x41), // a
        };
        WITH_CONTEXT(requireErrorAfterValidA(el::mem::ByteBlock::fromSpan(content.span()), ConfErrorCategory::Encoding))
        content = {
            el::mem::Byte(0x41),        // A
            el::mem::Byte(0b11110100U), // 4 byte sequence
            el::mem::Byte(0b11111111U), // Invalid followup byte
            el::mem::Byte(0b10111111U),
            el::mem::Byte(0b10111111U),
            el::mem::Byte(0x41), // A
        };
        WITH_CONTEXT(requireErrorAfterValidA(el::mem::ByteBlock::fromSpan(content.span()), ConfErrorCategory::Encoding))
        content = {
            el::mem::Byte(0x41),        // A
            el::mem::Byte(0b11110100U), // 4 byte sequence
            el::mem::Byte(0b10000001U), // ok
            el::mem::Byte(0b11000000U), // not ok.
            el::mem::Byte(0b10000000U), // ok
            el::mem::Byte(0x41),        // A
        };
        WITH_CONTEXT(requireErrorAfterValidA(el::mem::ByteBlock::fromSpan(content.span()), ConfErrorCategory::Encoding))
        content = {
            el::mem::Byte(0x41),        // A
            el::mem::Byte(0b11110100U), // 4 byte sequence
            el::mem::Byte(0b10000001U), // ok
            el::mem::Byte(0b10000000U), // ok
            el::mem::Byte(0b00111111U), // not ok.
            el::mem::Byte(0x41),        // A
        };
        WITH_CONTEXT(requireErrorAfterValidA(el::mem::ByteBlock::fromSpan(content.span()), ConfErrorCategory::Encoding))
        content = {
            el::mem::Byte(0x41),        // A
            el::mem::Byte(0b11110100U), // 4 byte sequence
            el::mem::Byte(0b10000001U), // ok
            el::mem::Byte(0b10000000U), // ok
            // last byte is missing.
        };
        WITH_CONTEXT(requireErrorAfterValidA(el::mem::ByteBlock::fromSpan(content.span()), ConfErrorCategory::Encoding))
        content = {
            el::mem::Byte(0x41),        // A
            el::mem::Byte(0b10000001U), // not ok, follow-up byte without start byte.
            el::mem::Byte(0x41),        // A
            el::mem::Byte(0x41),        // A
        };
        WITH_CONTEXT(requireErrorAfterValidA(el::mem::ByteBlock::fromSpan(content.span()), ConfErrorCategory::Encoding))
        content = {
            el::mem::Byte(0x41),        // A
            el::mem::Byte(0b11101101U), // low surrogate U+D800, not ok!
            el::mem::Byte(0b10100000U),
            el::mem::Byte(0b10000000U),
            el::mem::Byte(0x41), // A
        };
        WITH_CONTEXT(requireErrorAfterValidA(el::mem::ByteBlock::fromSpan(content.span()), ConfErrorCategory::Encoding))
        content = {
            el::mem::Byte(0x41),        // A
            el::mem::Byte(0b11101101U), // high surrogate U+DFFF, not ok!
            el::mem::Byte(0b10111111U),
            el::mem::Byte(0b10111111U),
            el::mem::Byte(0x41), // A
        };
        WITH_CONTEXT(requireErrorAfterValidA(el::mem::ByteBlock::fromSpan(content.span()), ConfErrorCategory::Encoding))
        content = {
            el::mem::Byte(0x41),  // A
            el::mem::Byte(0xEFU), // BOM in the middle of the document is not allowed.
            el::mem::Byte(0xBBU),
            el::mem::Byte(0xBFU),
            el::mem::Byte(0x41), // A
        };
        WITH_CONTEXT(requireErrorAfterValidA(el::mem::ByteBlock::fromSpan(content.span()), ConfErrorCategory::Encoding))
    }

    void testInvalidControlCharacters() {
        for (char8_t c = u8'\x00'; c != u8'\x1f'; ++c) {
            auto content = el::text::StringEditor{"A"_el};
            content.append(el::text::Char{static_cast<char32_t>(c)});
            content.append("A"_el);
            if (c == u8'\x0a' || c == u8'\x0d' || c == u8'\x09') {
                if (c == u8'\x0d') {
                    continue; // don't test CR alone.
                }
                auto source = Source::fromString(el::text::String{content});
                REQUIRE_NOTHROW(source->open());
                decoder = CharStream::create(source);
                REQUIRE(decoder != nullptr);
                WITH_CONTEXT(requireMatchingAsciiData(el::text::String{content}));
            } else {
                WITH_CONTEXT(requireErrorAfterValidA(el::text::String{content}, ConfErrorCategory::Character));
            }
        }
    }

    void testLineBreaks() {
        el::text::String content = "A\nA"_el;
        auto source = Source::fromString(content);
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'A');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 1U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'A');
        WITH_CONTEXT(requireLocation(decodedChar, 1U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().isEndOfData());

        content = "\n\n\nA"_el;
        source = Source::fromString(el::text::String{content});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 1U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 2U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'A');
        WITH_CONTEXT(requireLocation(decodedChar, 3U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().isEndOfData());

        content = "\r\n\r\n\r\nA"_el;
        source = Source::fromString(el::text::String{content});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\r');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 1U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\r');
        WITH_CONTEXT(requireLocation(decodedChar, 1U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 1U, 1U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\r');
        WITH_CONTEXT(requireLocation(decodedChar, 2U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 2U, 1U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LetterA);
        REQUIRE(decodedChar.character().toRawValue() == U'A');
        WITH_CONTEXT(requireLocation(decodedChar, 3U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().isEndOfData());

        content = "\n\r\n\nA"_el;
        source = Source::fromString(el::text::String{content});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\r');
        WITH_CONTEXT(requireLocation(decodedChar, 1U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 1U, 1U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 2U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'A');
        WITH_CONTEXT(requireLocation(decodedChar, 3U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().isEndOfData());

        content = "\n\rA"_el;
        source = Source::fromString(el::text::String{content});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 0U));
        // The decoder ignores invalid line-breaks but changes the position correctly.
        // The lexer will raise an error - as it has more context for better error reporting.
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\r');
        WITH_CONTEXT(requireLocation(decodedChar, 1U, 0U));

        content = "A\n"_el;
        source = Source::fromString(el::text::String{content});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'A');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 1U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().isEndOfData());

        content = "A\r\n"_el;
        source = Source::fromString(el::text::String{content});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LetterA);
        REQUIRE(decodedChar.character().toRawValue() == U'A');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 0U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\r');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 1U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\n');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 2U));
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().isEndOfData());

        content = "A\r"_el;
        source = Source::fromString(el::text::String{content});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character().toRawValue() == U'A');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 0U));
        // The decoder ignored the invalid line-ending but changed the position correctly.
        // The lexer will raise an error - as it has more context for better error reporting.
        decodedChar = decoder->next();
        REQUIRE(decodedChar.character() == CharClass::LineBreak);
        REQUIRE(decodedChar.character().toRawValue() == U'\r');
        WITH_CONTEXT(requireLocation(decodedChar, 0U, 1U));
    }
};
