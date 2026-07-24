// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/text/AnyString.hpp>
#include <erbsland/text/AnyStringEditor.hpp>
#include <erbsland/text/ParseNumberError.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringCharReader.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringKind.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/CpRange.hpp>
#include <erbsland/unit/U16DataIndex.hpp>
#include <erbsland/unit/U16DataLength.hpp>
#include <erbsland/unit/U16DataRange.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using namespace el::text;
using namespace el::unit;
using el::err::OverflowError;
using el::text::ParseNumberError;
using el::util::LoopResult;
using el::util::LoopStatus;

namespace th = erbsland::unittest::th;

static_assert(std::is_same_v<decltype(std::declval<StringCharReader &>().takeCapture()), AnyString>);
static_assert(std::is_same_v<decltype(std::declval<StringCharReader &>().takeBuffer()), AnyString>);

TESTED_TARGETS(
    StorageIdentifier StringCharReader ReadIntegerResult ReadNumberStatus ParseNumberError StringReader
        StringReaderState StringReaderBase U8StringReader U16StringReader U32StringReader)
class StringCharReaderTest final : public el::UnitTest {
public:
    void testStorageIdentifier() {
        const auto text = U8StringEditor{std::string_view{"abcd"}};
        const auto whole = text.storageId();
        const auto wholeView = U8String{text}.storageId();
        const auto prefix = text.slice(ByteRange{ByteIndex{0U}, ByteLength{2U}}).storageId();
        const auto suffix = text.slice(ByteRange{ByteIndex{2U}, ByteLength{2U}}).storageId();

        REQUIRE_FALSE(whole.isEmpty());
        REQUIRE_EQUAL(whole, wholeView);
        REQUIRE_NOT_EQUAL(whole, prefix);
        REQUIRE_NOT_EQUAL(prefix, suffix);
    }

    void testDefaultReaderIsAtEnd() {
        auto reader = StringCharReader{};

        REQUIRE(reader.isAtEnd());
        REQUIRE(reader.position().isZero());
        REQUIRE(reader.peek().isEndOfData());
        REQUIRE(reader.read().isEndOfData());
        REQUIRE_FALSE(reader.canRead(CpLength::one()));
        REQUIRE(reader.canRead(CpLength::zero()));
        REQUIRE_FALSE(reader.advance());
        REQUIRE_THROWS(reader.advanceOrThrow());
    }

    void testReadUtf8StringAndView() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};
        auto reader = StringCharReader{text};

        REQUIRE(reader.canRead(CpLength{4U}));
        REQUIRE_FALSE(reader.canRead(CpLength{5U}));
        REQUIRE(reader.position().isZero());
        REQUIRE_EQUAL(reader.peek().toRawValue(), U'A');
        REQUIRE_EQUAL(reader.read().toRawValue(), U'A');
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});

        const auto saved = reader.save();
        REQUIRE_EQUAL(reader.read().toRawValue(), U'\u00A2');
        REQUIRE(reader.restore(saved));
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});
        REQUIRE_EQUAL(reader.read().toRawValue(), U'\u00A2');

        REQUIRE(reader.advance(CpLength{2U}));
        REQUIRE_EQUAL(reader.position(), CpIndex{4U});
        REQUIRE(reader.isAtEnd());
        REQUIRE(reader.read().isEndOfData());
        REQUIRE_EQUAL(reader.position(), CpIndex{4U});
        reader.reset();
        REQUIRE(reader.position().isZero());
        REQUIRE_EQUAL(reader.read().toRawValue(), U'A');

        const auto sliced = text.slice(ByteRange{ByteIndex{1U}, ByteLength{5U}});
        auto viewReader = StringCharReader{U8String{sliced}};

        REQUIRE_EQUAL(viewReader.read().toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(viewReader.read().toRawValue(), U'\u20AC');
        REQUIRE(viewReader.isAtEnd());
    }

    void testReadUtf16StringAndView() {
        const auto text = U16StringEditor{std::u16string_view{u"A¢€😀"}};
        auto reader = StringCharReader{text};

        REQUIRE(reader.canRead(CpLength{4U}));
        REQUIRE_EQUAL(reader.read().toRawValue(), U'A');
        REQUIRE_EQUAL(reader.peek().toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(reader.read().toRawValue(), U'\u00A2');

        const auto saved = reader.save();
        reader.advanceOrThrow();
        REQUIRE(reader.restore(saved));
        REQUIRE_EQUAL(reader.read().toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(reader.read().toRawValue(), U'\U0001F600');
        REQUIRE(reader.isAtEnd());

        const auto sliced = text.slice(U16DataRange{U16DataIndex{3U}, U16DataLength{2U}});
        auto viewReader = StringCharReader{U16String{sliced}};

        REQUIRE_EQUAL(viewReader.read().toRawValue(), U'\U0001F600');
        REQUIRE(viewReader.isAtEnd());
    }

    void testReadUtf32StringAndView() {
        const auto text = U32StringEditor{std::u32string_view{U"A¢€😀"}};
        auto reader = StringCharReader{text};

        REQUIRE(reader.canRead(CpLength{4U}));
        REQUIRE_EQUAL(reader.read().toRawValue(), U'A');
        REQUIRE(reader.advance(CpLength{2U}));
        REQUIRE_EQUAL(reader.read().toRawValue(), U'\U0001F600');
        REQUIRE(reader.isAtEnd());

        const auto sliced = text.slice(CpRange{CpIndex{1U}, CpLength{2U}});
        auto viewReader = StringCharReader{U32String{sliced}};

        REQUIRE_EQUAL(viewReader.read().toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(viewReader.read().toRawValue(), U'\u20AC');
        REQUIRE(viewReader.isAtEnd());
    }

    void testConditionalReadUtf8String() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€"}};
        auto reader = StringCharReader{text};

        REQUIRE_FALSE(reader.readIf(Char{U'B'}));
        REQUIRE(reader.position().isZero());
        REQUIRE(reader.readIf(Char{U'A'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});

        REQUIRE_FALSE(reader.readIf(nonMatchingSet()).has_value());
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});
        const auto cent = reader.readIf(matchingSet());
        REQUIRE(cent.has_value());
        REQUIRE_EQUAL(cent.value().toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(reader.position(), CpIndex{2U});

        REQUIRE_FALSE(reader.readIf(Char{U'!'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{2U});
        REQUIRE(reader.readIf(Char{U'\u20AC'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{3U});
        REQUIRE_FALSE(reader.readIf(Char{U'!'}));
    }

    void testConditionalReadUtf16String() {
        const auto text = U16StringEditor{std::u16string_view{u"A😀€"}};
        auto reader = StringCharReader{text};

        REQUIRE_FALSE(reader.readIf(Char{U'B'}));
        REQUIRE(reader.position().isZero());
        REQUIRE(reader.readIf(Char{U'A'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});

        REQUIRE_FALSE(reader.readIf(nonMatchingSet()).has_value());
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});
        const auto emoji = reader.readIf(matchingSet());
        REQUIRE(emoji.has_value());
        REQUIRE_EQUAL(emoji.value().toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(reader.position(), CpIndex{2U});

        const auto euro = reader.readIf(matchingSet());
        REQUIRE(euro.has_value());
        REQUIRE_EQUAL(euro.value().toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(reader.position(), CpIndex{3U});
        REQUIRE_FALSE(reader.readIf(matchingSet()).has_value());
    }

    void testConditionalReadUtf32String() {
        const auto text = U32StringEditor{std::u32string_view{U"A¢€"}};
        auto reader = StringCharReader{text};

        REQUIRE_FALSE(reader.readIf(Char{U'B'}));
        REQUIRE(reader.position().isZero());
        REQUIRE(reader.readIf(Char{U'A'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});

        const auto cent = reader.readIf(matchingSet());
        REQUIRE(cent.has_value());
        REQUIRE_EQUAL(cent.value().toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(reader.position(), CpIndex{2U});
        REQUIRE_FALSE(reader.readIf(Char{U'!'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{2U});
        REQUIRE(reader.readIf(Char{U'\u20AC'}));
        REQUIRE_FALSE(reader.readIf(Char{U'!'}));
    }

    void testConditionalAdvanceUtf8String() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€"}};
        auto reader = StringCharReader{text};

        REQUIRE_FALSE(reader.advance(CpLength::zero()));
        REQUIRE_FALSE(reader.advanceIf(Char{U'B'}));
        REQUIRE(reader.position().isZero());
        REQUIRE(reader.advanceIf(Char{U'A'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});
        REQUIRE_FALSE(reader.advanceIf(CharSet{Char{U'!'}}));
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});
        REQUIRE(reader.advanceIf(matchingSet()));
        REQUIRE_EQUAL(reader.position(), CpIndex{2U});
        REQUIRE(reader.advanceIf(Char{U'\u20AC'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{3U});
        REQUIRE_FALSE(reader.advanceIf(Char{U'!'}));
    }

    void testConditionalAdvanceUtf16String() {
        const auto text = U16StringEditor{std::u16string_view{u"A😀€"}};
        auto reader = StringCharReader{text};

        REQUIRE_FALSE(reader.advance(CpLength::zero()));
        REQUIRE_FALSE(reader.advanceIf(Char{U'B'}));
        REQUIRE(reader.position().isZero());
        REQUIRE(reader.advanceIf(Char{U'A'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});
        REQUIRE(reader.advanceIf(matchingSet()));
        REQUIRE_EQUAL(reader.position(), CpIndex{2U});
        REQUIRE_FALSE(reader.advanceIf(Char{U'!'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{2U});
        REQUIRE(reader.advanceIf(matchingSet()));
        REQUIRE_EQUAL(reader.position(), CpIndex{3U});
        REQUIRE(reader.isAtEnd());
    }

    void testConditionalAdvanceUtf32String() {
        const auto text = U32StringEditor{std::u32string_view{U"A¢€"}};
        auto reader = StringCharReader{text};

        REQUIRE_FALSE(reader.advance(CpLength::zero()));
        REQUIRE_FALSE(reader.advanceIf(Char{U'B'}));
        REQUIRE(reader.position().isZero());
        REQUIRE(reader.advanceIf(Char{U'A'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{1U});
        REQUIRE(reader.advanceIf(matchingSet()));
        REQUIRE_EQUAL(reader.position(), CpIndex{2U});
        REQUIRE(reader.advanceIf(Char{U'\u20AC'}));
        REQUIRE_EQUAL(reader.position(), CpIndex{3U});
        REQUIRE_FALSE(reader.advanceIf(matchingSet()));
    }

    void testReadWhileAcrossEncodings() {
        auto utf8Reader = StringCharReader{U8StringEditor{std::u8string_view{u8"ab¢!"}}};
        auto utf8Text = std::u32string{};
        REQUIRE_EQUAL(utf8Reader.readWhile(collectText(utf8Text), loopTextSet()), LoopResult::Success);
        REQUIRE_EQUAL(utf8Text, std::u32string{U"ab¢"});
        REQUIRE_EQUAL(utf8Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf8Reader.peek().toRawValue(), U'!');

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600B!"}}};
        auto utf16Text = std::u32string{};
        REQUIRE_EQUAL(utf16Reader.readWhile(collectText(utf16Text), loopTextSet()), LoopResult::Success);
        REQUIRE_EQUAL(utf16Text, std::u32string{U"A\U0001F600B"});
        REQUIRE_EQUAL(utf16Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf16Reader.peek().toRawValue(), U'!');

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"xy€?"}}};
        auto utf32Text = std::u32string{};
        REQUIRE_EQUAL(utf32Reader.readWhile(collectText(utf32Text), loopTextSet()), LoopResult::Success);
        REQUIRE_EQUAL(utf32Text, std::u32string{U"xy€"});
        REQUIRE_EQUAL(utf32Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf32Reader.peek().toRawValue(), U'?');
    }

    void testReadUntilAcrossEncodings() {
        const auto stopSet = CharSet{Char{U';'}};

        auto utf8Reader = StringCharReader{U8StringEditor{std::u8string_view{u8"az¢;tail"}}};
        auto utf8Text = std::u32string{};
        REQUIRE_EQUAL(utf8Reader.readUntil(collectText(utf8Text), stopSet), LoopResult::Success);
        REQUIRE_EQUAL(utf8Text, std::u32string{U"az¢"});
        REQUIRE_EQUAL(utf8Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf8Reader.peek().toRawValue(), U';');

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600B;tail"}}};
        auto utf16Text = std::u32string{};
        REQUIRE_EQUAL(utf16Reader.readUntil(collectText(utf16Text), stopSet), LoopResult::Success);
        REQUIRE_EQUAL(utf16Text, std::u32string{U"A\U0001F600B"});
        REQUIRE_EQUAL(utf16Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf16Reader.peek().toRawValue(), U';');

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"xy€;tail"}}};
        auto utf32Text = std::u32string{};
        REQUIRE_EQUAL(utf32Reader.readUntil(collectText(utf32Text), stopSet), LoopResult::Success);
        REQUIRE_EQUAL(utf32Text, std::u32string{U"xy€"});
        REQUIRE_EQUAL(utf32Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf32Reader.peek().toRawValue(), U';');
    }

    void testAdvanceWhileAcrossEncodings() {
        auto utf8Reader = StringCharReader{U8StringEditor{std::u8string_view{u8"ab¢!"}}};
        REQUIRE_EQUAL(utf8Reader.advanceWhile(loopTextSet()), LoopResult::Success);
        REQUIRE_EQUAL(utf8Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf8Reader.peek().toRawValue(), U'!');

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600B!"}}};
        REQUIRE_EQUAL(utf16Reader.advanceWhile(loopTextSet()), LoopResult::Success);
        REQUIRE_EQUAL(utf16Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf16Reader.peek().toRawValue(), U'!');

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"xy€?"}}};
        REQUIRE_EQUAL(utf32Reader.advanceWhile(loopTextSet()), LoopResult::Success);
        REQUIRE_EQUAL(utf32Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf32Reader.peek().toRawValue(), U'?');
    }

    void testAdvanceUntilAcrossEncodings() {
        const auto stopSet = CharSet{Char{U';'}};

        auto utf8Reader = StringCharReader{U8StringEditor{std::u8string_view{u8"az¢;tail"}}};
        REQUIRE_EQUAL(utf8Reader.advanceUntil(stopSet), LoopResult::Success);
        REQUIRE_EQUAL(utf8Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf8Reader.peek().toRawValue(), U';');

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600B;tail"}}};
        REQUIRE_EQUAL(utf16Reader.advanceUntil(stopSet), LoopResult::Success);
        REQUIRE_EQUAL(utf16Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf16Reader.peek().toRawValue(), U';');

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"xy€;tail"}}};
        REQUIRE_EQUAL(utf32Reader.advanceUntil(stopSet), LoopResult::Success);
        REQUIRE_EQUAL(utf32Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf32Reader.peek().toRawValue(), U';');
    }

    void testAdvanceLoopsLimitEndOfDataAndZeroLimit() {
        const auto stopSet = CharSet{Char{U';'}};

        auto limitWhileReader = StringCharReader{U8StringEditor{std::u8string_view{u8"ab¢!"}}};
        REQUIRE_EQUAL(limitWhileReader.advanceWhile(loopTextSet(), CpLength{2U}), LoopResult::LimitReached);
        REQUIRE_EQUAL(limitWhileReader.position(), CpIndex{2U});
        REQUIRE_EQUAL(limitWhileReader.peek().toRawValue(), U'\u00A2');

        auto limitUntilReader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600B;"}}};
        REQUIRE_EQUAL(limitUntilReader.advanceUntil(stopSet, CpLength{2U}), LoopResult::LimitReached);
        REQUIRE_EQUAL(limitUntilReader.position(), CpIndex{2U});
        REQUIRE_EQUAL(limitUntilReader.peek().toRawValue(), U'B');

        auto whileEndReader = StringCharReader{U32StringEditor{std::u32string_view{U"xy"}}};
        REQUIRE_EQUAL(whileEndReader.advanceWhile(loopTextSet()), LoopResult::EndOfData);
        REQUIRE_EQUAL(whileEndReader.position(), CpIndex{2U});
        REQUIRE(whileEndReader.isAtEnd());

        auto untilEndReader = StringCharReader{U8StringEditor{std::string_view{"xy"}}};
        REQUIRE_EQUAL(untilEndReader.advanceUntil(stopSet), LoopResult::EndOfData);
        REQUIRE_EQUAL(untilEndReader.position(), CpIndex{2U});
        REQUIRE(untilEndReader.isAtEnd());

        auto zeroLimitWhileMatchingReader = StringCharReader{U8StringEditor{std::string_view{"abc"}}};
        REQUIRE_EQUAL(
            zeroLimitWhileMatchingReader.advanceWhile(loopTextSet(), CpLength::zero()), LoopResult::LimitReached);
        REQUIRE(zeroLimitWhileMatchingReader.position().isZero());
        REQUIRE_EQUAL(zeroLimitWhileMatchingReader.peek(), U'a');

        auto zeroLimitUntilMatchingReader = StringCharReader{U8StringEditor{std::string_view{"abc"}}};
        REQUIRE_EQUAL(zeroLimitUntilMatchingReader.advanceUntil(stopSet, CpLength::zero()), LoopResult::LimitReached);
        REQUIRE(zeroLimitUntilMatchingReader.position().isZero());
        REQUIRE_EQUAL(zeroLimitUntilMatchingReader.peek(), U'a');

        auto zeroLimitWhileStoppedReader = StringCharReader{U8StringEditor{std::string_view{"!abc"}}};
        REQUIRE_EQUAL(zeroLimitWhileStoppedReader.advanceWhile(loopTextSet(), CpLength::zero()), LoopResult::Success);
        REQUIRE(zeroLimitWhileStoppedReader.position().isZero());
        REQUIRE_EQUAL(zeroLimitWhileStoppedReader.peek(), U'!');

        auto zeroLimitUntilStoppedReader = StringCharReader{U8StringEditor{std::string_view{";abc"}}};
        REQUIRE_EQUAL(zeroLimitUntilStoppedReader.advanceUntil(stopSet, CpLength::zero()), LoopResult::Success);
        REQUIRE(zeroLimitUntilStoppedReader.position().isZero());
        REQUIRE_EQUAL(zeroLimitUntilStoppedReader.peek(), U';');
    }

    void testReadLoopCallbackStopsLeaveCharacterUnread() {
        WITH_CONTEXT(requireReadLoopCallbackResult(
            StringCharReader{U8StringEditor{std::u8string_view{u8"ab¢!"}}},
            Char{U'\u00A2'},
            LoopStatus::Stop,
            LoopResult::Stopped,
            CpIndex{2U}));
        WITH_CONTEXT(requireReadLoopCallbackResult(
            StringCharReader{U8StringEditor{std::u8string_view{u8"ab¢!"}}},
            Char{U'\u00A2'},
            LoopStatus::Error,
            LoopResult::Error,
            CpIndex{2U}));
        WITH_CONTEXT(requireReadLoopCallbackResult(
            StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600B!"}}},
            Char{U'\U0001F600'},
            LoopStatus::Stop,
            LoopResult::Stopped,
            CpIndex{1U}));
        WITH_CONTEXT(requireReadLoopCallbackResult(
            StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600B!"}}},
            Char{U'\U0001F600'},
            LoopStatus::Error,
            LoopResult::Error,
            CpIndex{1U}));
        WITH_CONTEXT(requireReadLoopCallbackResult(
            StringCharReader{U32StringEditor{std::u32string_view{U"xy€?"}}},
            Char{U'\u20AC'},
            LoopStatus::Stop,
            LoopResult::Stopped,
            CpIndex{2U}));
        WITH_CONTEXT(requireReadLoopCallbackResult(
            StringCharReader{U32StringEditor{std::u32string_view{U"xy€?"}}},
            Char{U'\u20AC'},
            LoopStatus::Error,
            LoopResult::Error,
            CpIndex{2U}));
    }

    void testReadLoopLimitReachedLeavesNextCharacterUnread() {
        auto utf8Reader = StringCharReader{U8StringEditor{std::u8string_view{u8"ab¢!"}}};
        auto utf8Text = std::u32string{};
        REQUIRE_EQUAL(
            utf8Reader.readWhile(collectText(utf8Text), loopTextSet(), CpLength{2U}), LoopResult::LimitReached);
        REQUIRE_EQUAL(utf8Text, std::u32string{U"ab"});
        REQUIRE_EQUAL(utf8Reader.position(), CpIndex{2U});
        REQUIRE_EQUAL(utf8Reader.peek().toRawValue(), U'\u00A2');

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600B;"}}};
        auto utf16Text = std::u32string{};
        REQUIRE_EQUAL(
            utf16Reader.readUntil(collectText(utf16Text), CharSet{Char{U';'}}, CpLength{2U}), LoopResult::LimitReached);
        REQUIRE_EQUAL(utf16Text, std::u32string{U"A\U0001F600"});
        REQUIRE_EQUAL(utf16Reader.position(), CpIndex{2U});
        REQUIRE_EQUAL(utf16Reader.peek().toRawValue(), U'B');

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"xy€?"}}};
        auto utf32Text = std::u32string{};
        REQUIRE_EQUAL(
            utf32Reader.readWhile(collectText(utf32Text), loopTextSet(), CpLength{2U}), LoopResult::LimitReached);
        REQUIRE_EQUAL(utf32Text, std::u32string{U"xy"});
        REQUIRE_EQUAL(utf32Reader.position(), CpIndex{2U});
        REQUIRE_EQUAL(utf32Reader.peek().toRawValue(), U'\u20AC');
    }

    void testReadLoopEndOfDataAndZeroLimit() {
        auto endReader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600"}}};
        auto endText = std::u32string{};
        REQUIRE_EQUAL(endReader.readWhile(collectText(endText), loopTextSet()), LoopResult::EndOfData);
        REQUIRE_EQUAL(endText, std::u32string{U"A\U0001F600"});
        REQUIRE_EQUAL(endReader.position(), CpIndex{2U});
        REQUIRE(endReader.isAtEnd());

        auto untilEndReader = StringCharReader{U32StringEditor{std::u32string_view{U"xy"}}};
        auto untilEndText = std::u32string{};
        REQUIRE_EQUAL(untilEndReader.readUntil(collectText(untilEndText), CharSet{Char{U';'}}), LoopResult::EndOfData);
        REQUIRE_EQUAL(untilEndText, std::u32string{U"xy"});
        REQUIRE(untilEndReader.isAtEnd());

        auto zeroLimitMatchingReader = StringCharReader{U8StringEditor{std::string_view{"abc"}}};
        auto zeroLimitText = std::u32string{};
        REQUIRE_EQUAL(
            zeroLimitMatchingReader.readWhile(collectText(zeroLimitText), loopTextSet(), CpLength::zero()),
            LoopResult::LimitReached);
        REQUIRE(zeroLimitText.empty());
        REQUIRE(zeroLimitMatchingReader.position().isZero());
        REQUIRE_EQUAL(zeroLimitMatchingReader.peek().toRawValue(), U'a');

        auto zeroLimitStoppedReader = StringCharReader{U8StringEditor{std::string_view{"!abc"}}};
        REQUIRE_EQUAL(
            zeroLimitStoppedReader.readWhile(collectText(zeroLimitText), loopTextSet(), CpLength::zero()),
            LoopResult::Success);
        REQUIRE(zeroLimitStoppedReader.position().isZero());
        REQUIRE_EQUAL(zeroLimitStoppedReader.peek().toRawValue(), U'!');

        auto zeroLimitEndReader = StringCharReader{U8StringEditor{}};
        REQUIRE_EQUAL(
            zeroLimitEndReader.readWhile(collectText(zeroLimitText), loopTextSet(), CpLength::zero()),
            LoopResult::EndOfData);
    }

    void testCaptureAcrossEncodings() {
        auto utf8Reader = StringCharReader{U8StringEditor{std::u8string_view{u8"A¢€!"}}};
        REQUIRE(utf8Reader.takeCapture().isEmpty());
        utf8Reader.startCapture();
        REQUIRE(utf8Reader.advance(CpLength{2U}));
        WITH_CONTEXT(requireCapture(utf8Reader.takeCapture(), StringKind::U8, U"A¢"));
        REQUIRE(utf8Reader.advance());
        WITH_CONTEXT(requireCapture(utf8Reader.takeCapture(), StringKind::U8, U"€"));
        utf8Reader.startCapture();
        REQUIRE(utf8Reader.takeCapture().isEmpty());
        utf8Reader.startCapture();
        utf8Reader.reset();
        REQUIRE(utf8Reader.takeCapture().isEmpty());

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600B!"}}};
        REQUIRE(utf16Reader.takeCapture().isEmpty());
        utf16Reader.startCapture();
        REQUIRE(utf16Reader.advance(CpLength{2U}));
        WITH_CONTEXT(requireCapture(utf16Reader.takeCapture(), StringKind::U16, U"A\U0001F600"));
        REQUIRE(utf16Reader.advance());
        WITH_CONTEXT(requireCapture(utf16Reader.takeCapture(), StringKind::U16, U"B"));
        utf16Reader.startCapture();
        REQUIRE(utf16Reader.takeCapture().isEmpty());
        utf16Reader.startCapture();
        utf16Reader.reset();
        REQUIRE(utf16Reader.takeCapture().isEmpty());

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"xy€?"}}};
        REQUIRE(utf32Reader.takeCapture().isEmpty());
        utf32Reader.startCapture();
        REQUIRE(utf32Reader.advance(CpLength{2U}));
        WITH_CONTEXT(requireCapture(utf32Reader.takeCapture(), StringKind::U32, U"xy"));
        REQUIRE(utf32Reader.advance());
        WITH_CONTEXT(requireCapture(utf32Reader.takeCapture(), StringKind::U32, U"€"));
        utf32Reader.startCapture();
        REQUIRE(utf32Reader.takeCapture().isEmpty());
        utf32Reader.startCapture();
        utf32Reader.reset();
        REQUIRE(utf32Reader.takeCapture().isEmpty());
    }

    void testBufferManualOperationsAcrossEncodings() {
        auto utf8Reader = StringCharReader{U8StringEditor{std::string_view{"source"}}};
        REQUIRE(utf8Reader.isBufferEmpty());
        REQUIRE(utf8Reader.bufferView().isEmpty());
        REQUIRE_EQUAL(utf8Reader.bufferCharacterLength(), CpLength::zero());
        utf8Reader.appendToBuffer(Char{U'A'});
        const auto emojiText = U16StringEditor{std::u16string_view{u"\U0001F600"}};
        utf8Reader.appendToBuffer(emojiText);
        WITH_CONTEXT(requireBufferView(utf8Reader.bufferView(), StringKind::U8, U"A\U0001F600"));
        REQUIRE_EQUAL(utf8Reader.bufferCharacterLength(), CpLength{2U});
        WITH_CONTEXT(requireBuffer(utf8Reader.takeBuffer(), StringKind::U8, U"A\U0001F600"));
        REQUIRE(utf8Reader.isBufferEmpty());
        const auto utf32Text = U32StringEditor{std::u32string_view{U"xy€"}};
        utf8Reader.setBuffer(utf32Text);
        WITH_CONTEXT(requireBufferView(utf8Reader.bufferView(), StringKind::U8, U"xy€"));
        utf8Reader.clearBuffer();
        REQUIRE(utf8Reader.isBufferEmpty());

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"source"}}};
        const auto utf8Text = U8StringEditor{std::u8string_view{u8"A¢"}};
        utf16Reader.appendToBuffer(utf8Text);
        utf16Reader.appendToBuffer(Char{U'\U0001F600'});
        WITH_CONTEXT(requireBufferView(utf16Reader.bufferView(), StringKind::U16, U"A¢\U0001F600"));
        REQUIRE_EQUAL(utf16Reader.bufferCharacterLength(), CpLength{3U});
        WITH_CONTEXT(requireBuffer(utf16Reader.takeBuffer(), StringKind::U16, U"A¢\U0001F600"));

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"source"}}};
        const auto azText = U8StringEditor{std::u8string_view{u8"az"}};
        const auto euroText = U16StringEditor{std::u16string_view{u"€"}};
        utf32Reader.setBuffer(azText);
        utf32Reader.appendToBuffer(euroText);
        WITH_CONTEXT(requireBufferView(utf32Reader.bufferView(), StringKind::U32, U"az€"));
        REQUIRE_EQUAL(utf32Reader.bufferCharacterLength(), CpLength{3U});
        WITH_CONTEXT(requireBuffer(utf32Reader.takeBuffer(), StringKind::U32, U"az€"));
    }

    void testReadToBufferConditionalReads() {
        auto utf8Reader = StringCharReader{U8StringEditor{std::u8string_view{u8"A¢€!"}}};
        REQUIRE_EQUAL(utf8Reader.readToBuffer().toRawValue(), U'A');
        REQUIRE_FALSE(utf8Reader.readToBufferIf(Char{U'!'}));
        REQUIRE_EQUAL(utf8Reader.position(), CpIndex{1U});
        REQUIRE_EQUAL(utf8Reader.bufferCharacterLength(), CpLength::one());
        REQUIRE(utf8Reader.readToBufferIf(Char{U'\u00A2'}));
        const auto euro = utf8Reader.readToBufferIf(matchingSet());
        REQUIRE(euro.has_value());
        REQUIRE_EQUAL(euro.value(), Char{U'\u20AC'});
        REQUIRE_FALSE(utf8Reader.readToBufferIf(matchingSet()).has_value());
        WITH_CONTEXT(requireBufferView(utf8Reader.bufferView(), StringKind::U8, U"A¢€"));
        REQUIRE_EQUAL(utf8Reader.peek(), U'!');

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600!"}}};
        REQUIRE_EQUAL(utf16Reader.readToBuffer().toRawValue(), U'A');
        REQUIRE(utf16Reader.readToBufferIf(matchingSet()).has_value());
        REQUIRE_FALSE(utf16Reader.readToBufferIf(Char{U'\u20AC'}));
        WITH_CONTEXT(requireBufferView(utf16Reader.bufferView(), StringKind::U16, U"A\U0001F600"));
        REQUIRE_EQUAL(utf16Reader.peek(), U'!');

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"xy€"}}};
        REQUIRE(utf32Reader.readToBufferIf(Char{U'x'}));
        REQUIRE_EQUAL(utf32Reader.readToBuffer().toRawValue(), U'y');
        REQUIRE(utf32Reader.readToBufferIf(matchingSet()).has_value());
        REQUIRE(utf32Reader.readToBuffer().isEndOfData());
        WITH_CONTEXT(requireBufferView(utf32Reader.bufferView(), StringKind::U32, U"xy€"));
    }

    void testReadToBufferLoopsAcrossEncodings() {
        auto utf8Reader = StringCharReader{U8StringEditor{std::u8string_view{u8"ab¢!"}}};
        REQUIRE_EQUAL(utf8Reader.readToBufferWhile(loopTextSet()), LoopResult::Success);
        WITH_CONTEXT(requireBufferView(utf8Reader.bufferView(), StringKind::U8, U"ab¢"));
        REQUIRE_EQUAL(utf8Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf8Reader.peek(), U'!');

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600B;tail"}}};
        REQUIRE_EQUAL(utf16Reader.readToBufferUntil(CharSet{Char{U';'}}), LoopResult::Success);
        WITH_CONTEXT(requireBufferView(utf16Reader.bufferView(), StringKind::U16, U"A\U0001F600B"));
        REQUIRE_EQUAL(utf16Reader.position(), CpIndex{3U});
        REQUIRE_EQUAL(utf16Reader.peek(), U';');

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"xy€?"}}};
        REQUIRE_EQUAL(utf32Reader.readToBufferWhile(loopTextSet(), CpLength{2U}), LoopResult::LimitReached);
        WITH_CONTEXT(requireBufferView(utf32Reader.bufferView(), StringKind::U32, U"xy"));
        REQUIRE_EQUAL(utf32Reader.position(), CpIndex{2U});
        REQUIRE_EQUAL(utf32Reader.peek(), U'\u20AC');
    }

    void testReadToBufferLoopEndOfDataAndZeroLimit() {
        auto endReader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600"}}};
        REQUIRE_EQUAL(endReader.readToBufferWhile(loopTextSet()), LoopResult::EndOfData);
        WITH_CONTEXT(requireBufferView(endReader.bufferView(), StringKind::U16, U"A\U0001F600"));
        REQUIRE(endReader.isAtEnd());

        auto zeroLimitMatchingReader = StringCharReader{U8StringEditor{std::string_view{"abc"}}};
        REQUIRE_EQUAL(
            zeroLimitMatchingReader.readToBufferWhile(loopTextSet(), CpLength::zero()), LoopResult::LimitReached);
        REQUIRE(zeroLimitMatchingReader.isBufferEmpty());
        REQUIRE(zeroLimitMatchingReader.position().isZero());
        REQUIRE_EQUAL(zeroLimitMatchingReader.peek(), U'a');

        auto zeroLimitStoppedReader = StringCharReader{U8StringEditor{std::string_view{"!abc"}}};
        REQUIRE_EQUAL(zeroLimitStoppedReader.readToBufferWhile(loopTextSet(), CpLength::zero()), LoopResult::Success);
        REQUIRE(zeroLimitStoppedReader.isBufferEmpty());
        REQUIRE(zeroLimitStoppedReader.position().isZero());
        REQUIRE_EQUAL(zeroLimitStoppedReader.peek(), U'!');
    }

    void testCaptureCanBeAppendedToBuffer() {
        auto utf8Reader = StringCharReader{U8StringEditor{std::u8string_view{u8"A¢€!"}}};
        utf8Reader.startCapture();
        REQUIRE(utf8Reader.advance(CpLength{2U}));
        utf8Reader.appendCaptureToBuffer();
        WITH_CONTEXT(requireBufferView(utf8Reader.bufferView(), StringKind::U8, U"A¢"));
        REQUIRE(utf8Reader.advance());
        utf8Reader.appendCaptureToBuffer();
        WITH_CONTEXT(requireBufferView(utf8Reader.bufferView(), StringKind::U8, U"A¢€"));

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"A\U0001F600!"}}};
        utf16Reader.startCapture();
        REQUIRE(utf16Reader.advance(CpLength{2U}));
        utf16Reader.appendCaptureToBuffer();
        WITH_CONTEXT(requireBufferView(utf16Reader.bufferView(), StringKind::U16, U"A\U0001F600"));

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"xy€"}}};
        utf32Reader.startCapture();
        REQUIRE(utf32Reader.advance(CpLength{2U}));
        utf32Reader.appendCaptureToBuffer();
        WITH_CONTEXT(requireBufferView(utf32Reader.bufferView(), StringKind::U32, U"xy"));
    }

    void testCopiesHaveIndependentBuffers() {
        const auto text = U8StringEditor{std::string_view{"abc"}};
        auto first = StringCharReader{text};
        first.appendToBuffer(Char{U'A'});
        auto second = first;

        REQUIRE_EQUAL(second.readToBuffer().toRawValue(), U'a');
        WITH_CONTEXT(requireBufferView(second.bufferView(), StringKind::U8, U"Aa"));
        WITH_CONTEXT(requireBufferView(first.bufferView(), StringKind::U8, U"A"));

        first.appendToBuffer(Char{U'1'});
        second.appendToBuffer(Char{U'2'});
        WITH_CONTEXT(requireBufferView(first.bufferView(), StringKind::U8, U"A1"));
        WITH_CONTEXT(requireBufferView(second.bufferView(), StringKind::U8, U"Aa2"));
    }

    void testRestoreDoesNotChangeBufferOrCapture() {
        auto reader = StringCharReader{U8StringEditor{std::string_view{"abcd"}}};
        reader.startCapture();
        REQUIRE(reader.advance(CpLength{2U}));
        const auto state = reader.save();
        reader.appendToBuffer(Char{U'X'});
        REQUIRE(reader.advance());
        reader.appendCaptureToBuffer();
        REQUIRE(reader.restore(state));

        REQUIRE_EQUAL(reader.position(), CpIndex{2U});
        WITH_CONTEXT(requireBufferView(reader.bufferView(), StringKind::U8, U"Xabc"));
        REQUIRE(reader.advance());
        reader.appendCaptureToBuffer();
        WITH_CONTEXT(requireBufferView(reader.bufferView(), StringKind::U8, U"Xabc"));
        REQUIRE(reader.advance());
        reader.appendCaptureToBuffer();
        WITH_CONTEXT(requireBufferView(reader.bufferView(), StringKind::U8, U"Xabcd"));
    }

    void testReadToBufferToleratesMalformedEncoding() {
        auto utf8Reader = StringCharReader{U8StringEditor{std::string_view{th::stdStringFromHex("41 C0 42")}}};
        REQUIRE_EQUAL(utf8Reader.readToBuffer().toRawValue(), U'A');
        REQUIRE(utf8Reader.readToBuffer().isReplacement());
        REQUIRE_EQUAL(utf8Reader.readToBuffer().toRawValue(), U'B');
        WITH_CONTEXT(requireBufferView(utf8Reader.bufferView(), StringKind::U8, U"A\uFFFDB"));

        const auto invalidUtf16 = std::u16string{u'A', char16_t{0xD800U}, u'B'};
        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{invalidUtf16}}};
        REQUIRE_EQUAL(utf16Reader.readToBuffer().toRawValue(), U'A');
        REQUIRE(utf16Reader.readToBuffer().isReplacement());
        REQUIRE_EQUAL(utf16Reader.readToBuffer().toRawValue(), U'B');
        WITH_CONTEXT(requireBufferView(utf16Reader.bufferView(), StringKind::U16, U"A\uFFFDB"));

        const auto invalidUtf32 = std::u32string{U'A', char32_t{0x110000U}, U'B'};
        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{invalidUtf32}}};
        REQUIRE_EQUAL(utf32Reader.readToBuffer().toRawValue(), U'A');
        REQUIRE(utf32Reader.readToBuffer().isReplacement());
        REQUIRE_EQUAL(utf32Reader.readToBuffer().toRawValue(), U'B');
        WITH_CONTEXT(requireBufferView(utf32Reader.bufferView(), StringKind::U32, U"A\uFFFDB"));
    }

    void testCopiesHaveIndependentPositions() {
        const auto text = U8StringEditor{std::string_view{"abc"}};
        auto first = StringCharReader{text};
        auto second = first;

        REQUIRE_EQUAL(second.read().toRawValue(), U'a');
        REQUIRE_EQUAL(second.read().toRawValue(), U'b');
        REQUIRE_EQUAL(first.read().toRawValue(), U'a');

        auto third = second;
        REQUIRE_EQUAL(third.read().toRawValue(), U'c');
        REQUIRE_EQUAL(second.read().toRawValue(), U'c');
    }

    void testRestoreRejectsWrongKindAndOutOfRange() {
        const auto longText = U8StringEditor{std::string_view{"abcd"}};
        auto longUtf8Reader = StringCharReader{longText};
        REQUIRE(longUtf8Reader.advance(CpLength{3U}));
        const auto longUtf8State = longUtf8Reader.save();

        auto shortUtf8Reader = StringCharReader{U8StringEditor{std::string_view{"a"}}};
        REQUIRE_FALSE(shortUtf8Reader.restore(longUtf8State));
        REQUIRE_EQUAL(shortUtf8Reader.read().toRawValue(), U'a');

        auto sameStorageReader = StringCharReader{longText};
        REQUIRE(sameStorageReader.restore(longUtf8State));
        REQUIRE_EQUAL(sameStorageReader.position(), CpIndex{3U});
        REQUIRE_EQUAL(sameStorageReader.read().toRawValue(), U'd');

        auto slicedReader = StringCharReader{longText.slice(ByteRange{ByteIndex{1U}, ByteLength{3U}})};
        REQUIRE_FALSE(slicedReader.restore(longUtf8State));
        REQUIRE(slicedReader.position().isZero());

        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{u"abc"}}};
        REQUIRE(utf16Reader.advance());
        const auto utf16State = utf16Reader.save();

        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{U"abc"}}};
        REQUIRE_FALSE(utf32Reader.restore(utf16State));
        REQUIRE_EQUAL(utf32Reader.read().toRawValue(), U'a');
    }

    void testParseIntegerBasics() {
        auto decimalReader = StringCharReader{U8StringEditor{std::string_view{"123abc"}}};
        const auto decimal = decimalReader.parseInteger(integerOptions(IntegerBase::Decimal, CpLength{3U}));
        REQUIRE_EQUAL(decimal.status, ReadNumberStatus::Success);
        REQUIRE_EQUAL(decimal.value, std::uint64_t{123U});
        REQUIRE_EQUAL(decimal.digitCount, CpLength{3U});
        REQUIRE_FALSE(decimal.isNegative);
        REQUIRE_EQUAL(decimal.base, IntegerBase::Decimal);
        REQUIRE_EQUAL(decimalReader.position(), CpIndex{3U});
        REQUIRE_EQUAL(decimalReader.peek(), U'a');

        auto hexadecimalReader = StringCharReader{U8StringEditor{std::string_view{"1fZ"}}};
        const auto hexadecimal = hexadecimalReader.parseInteger(integerOptions(IntegerBase::Hexadecimal, CpLength{3U}));
        REQUIRE_EQUAL(hexadecimal.status, ReadNumberStatus::Success);
        REQUIRE_EQUAL(hexadecimal.value, std::uint64_t{31U});
        REQUIRE_EQUAL(hexadecimal.digitCount, CpLength{2U});
        REQUIRE_EQUAL(hexadecimal.base, IntegerBase::Hexadecimal);
        REQUIRE_EQUAL(hexadecimalReader.peek(), U'Z');

        auto noDigitsReader = StringCharReader{U8StringEditor{std::string_view{"abc"}}};
        const auto noDigits = noDigitsReader.parseInteger(integerOptions(IntegerBase::Decimal, CpLength{3U}));
        REQUIRE_EQUAL(noDigits.status, ReadNumberStatus::NoDigits);
        REQUIRE(noDigitsReader.position().isZero());

        auto tooManyDigitsReader = StringCharReader{U8StringEditor{std::string_view{"1234"}}};
        const auto tooManyDigits = tooManyDigitsReader.parseInteger(integerOptions(IntegerBase::Decimal, CpLength{3U}));
        REQUIRE_EQUAL(tooManyDigits.status, ReadNumberStatus::TooManyDigits);
        REQUIRE(tooManyDigitsReader.position().isZero());

        auto overflowReader = StringCharReader{U8StringEditor{std::string_view{"18446744073709551616"}}};
        const auto overflow = overflowReader.parseInteger(integerOptions(IntegerBase::Decimal, CpLength{20U}));
        REQUIRE_EQUAL(overflow.status, ReadNumberStatus::Overflow);
        REQUIRE(overflowReader.position().isZero());
    }

    void testParseIntegerSignsAndPrefixes() {
        auto negativeHexReader = StringCharReader{U8StringEditor{std::string_view{"-0x1f!"}}};
        auto signedOptions = IntegerParseOptions::parserDefault();
        signedOptions.addFlags(IntegerParseFlag::AcceptMinusSign);
        const auto negativeHex = negativeHexReader.parseInteger(signedOptions);
        REQUIRE_EQUAL(negativeHex.status, ReadNumberStatus::Success);
        REQUIRE_EQUAL(negativeHex.value, std::uint64_t{31U});
        REQUIRE(negativeHex.isNegative);
        REQUIRE_EQUAL(negativeHex.base, IntegerBase::Hexadecimal);
        REQUIRE_EQUAL(negativeHexReader.peek(), U'!');

        auto plusReader = StringCharReader{U8StringEditor{std::string_view{"+123"}}};
        auto plusOptions = integerOptions(IntegerBase::Decimal);
        plusOptions.addFlags(IntegerParseFlag::IgnorePlusSign);
        const auto plus = plusReader.parseInteger(plusOptions);
        REQUIRE_EQUAL(plus.status, ReadNumberStatus::Success);
        REQUIRE_EQUAL(plus.value, std::uint64_t{123U});
        REQUIRE_FALSE(plus.isNegative);

        auto rejectedPlusReader = StringCharReader{U8StringEditor{std::string_view{"+123"}}};
        const auto rejectedPlus = rejectedPlusReader.parseInteger(integerOptions(IntegerBase::Decimal));
        REQUIRE_EQUAL(rejectedPlus.status, ReadNumberStatus::ParseError);
        REQUIRE(rejectedPlusReader.position().isZero());

        auto rejectedMinusReader = StringCharReader{U8StringEditor{std::string_view{"-123"}}};
        const auto rejectedMinus = rejectedMinusReader.parseInteger(integerOptions(IntegerBase::Decimal));
        REQUIRE_EQUAL(rejectedMinus.status, ReadNumberStatus::ParseError);
        REQUIRE(rejectedMinusReader.position().isZero());

        auto fixedBasePrefixReader = StringCharReader{U8StringEditor{std::string_view{"0xff"}}};
        const auto fixedBasePrefix = fixedBasePrefixReader.parseInteger(integerOptions(IntegerBase::Hexadecimal));
        REQUIRE_EQUAL(fixedBasePrefix.status, ReadNumberStatus::ParseError);
        REQUIRE(fixedBasePrefixReader.position().isZero());
    }

    void testParseIntegerDigitModes() {
        auto limitedReader = StringCharReader{U8StringEditor{std::string_view{"1234"}}};
        auto limitedOptions = integerOptions(IntegerBase::Decimal, CpLength{3U});
        limitedOptions.addFlags(IntegerParseFlag::StopAtMaximum);
        const auto limited = limitedReader.parseInteger(limitedOptions);
        REQUIRE_EQUAL(limited.status, ReadNumberStatus::Success);
        REQUIRE_EQUAL(limited.value, std::uint64_t{123U});
        REQUIRE_EQUAL(limited.digitCount, CpLength{3U});
        REQUIRE_EQUAL(limitedReader.peek(), U'4');

        auto fixedReader = StringCharReader{U8StringEditor{std::string_view{"1234"}}};
        const auto fixedOptions = IntegerParseOptions::fixedDecimal(CpLength{3U});
        const auto fixed = fixedReader.parseInteger(fixedOptions);
        REQUIRE_EQUAL(fixed.status, ReadNumberStatus::Success);
        REQUIRE_EQUAL(fixed.value, std::uint64_t{123U});
        REQUIRE_EQUAL(fixedReader.peek(), U'4');

        auto tooFewReader = StringCharReader{U8StringEditor{std::string_view{"12x"}}};
        const auto tooFew = tooFewReader.parseInteger(fixedOptions);
        REQUIRE_EQUAL(tooFew.status, ReadNumberStatus::TooFewDigits);
        REQUIRE(tooFewReader.position().isZero());
    }

    void testParseIntegerSeparators() {
        auto separatedReader = StringCharReader{U8StringEditor{std::string_view{"1_234x"}}};
        auto separatorOptions = integerOptions(IntegerBase::Decimal);
        separatorOptions.addFlags(IntegerParseFlag::AllowSeparator).setSeparator(U'_');
        const auto separated = separatedReader.parseInteger(separatorOptions);
        REQUIRE_EQUAL(separated.status, ReadNumberStatus::Success);
        REQUIRE_EQUAL(separated.value, std::uint64_t{1234U});
        REQUIRE_EQUAL(separated.digitCount, CpLength{4U});
        REQUIRE_EQUAL(separatedReader.peek(), U'x');

        auto leadingReader = StringCharReader{U8StringEditor{std::string_view{"_123"}}};
        REQUIRE_EQUAL(leadingReader.parseInteger(separatorOptions).status, ReadNumberStatus::ParseError);
        REQUIRE(leadingReader.position().isZero());

        auto trailingReader = StringCharReader{U8StringEditor{std::string_view{"123_"}}};
        REQUIRE_EQUAL(trailingReader.parseInteger(separatorOptions).status, ReadNumberStatus::ParseError);
        REQUIRE(trailingReader.position().isZero());

        auto doubledReader = StringCharReader{U8StringEditor{std::string_view{"12__3"}}};
        REQUIRE_EQUAL(doubledReader.parseInteger(separatorOptions).status, ReadNumberStatus::ParseError);
        REQUIRE(doubledReader.position().isZero());

        auto tooManyReader = StringCharReader{U8StringEditor{std::string_view{"12_3"}}};
        auto tooManyOptions = separatorOptions;
        tooManyOptions.setMaximumDigits(CpLength{2U});
        REQUIRE_EQUAL(tooManyReader.parseInteger(tooManyOptions).status, ReadNumberStatus::TooManyDigits);
        REQUIRE(tooManyReader.position().isZero());
    }

    void testReadIntegerOrThrow() {
        auto decimalReader = StringCharReader{U8StringEditor{std::string_view{"123abc"}}};
        REQUIRE_EQUAL(decimalReader.readIntegerOrThrow<std::int32_t>(integerOptions(IntegerBase::Decimal)), 123);
        REQUIRE_EQUAL(decimalReader.position(), CpIndex{3U});
        REQUIRE_EQUAL(decimalReader.peek(), U'a');

        auto signedReader = StringCharReader{U8StringEditor{std::string_view{"-128!"}}};
        auto signedOptions = integerOptions(IntegerBase::Decimal);
        signedOptions.addFlags(IntegerParseFlag::AcceptMinusSign);
        REQUIRE_EQUAL(signedReader.readIntegerOrThrow<std::int8_t>(signedOptions), std::int8_t{-128});
        REQUIRE_EQUAL(signedReader.peek(), U'!');

        auto fixedDecimalReader = StringCharReader{U8StringEditor{std::string_view{"2026-06-05"}}};
        REQUIRE_EQUAL(
            fixedDecimalReader.readIntegerOrThrow<std::int32_t>(IntegerParseOptions::fixedDecimal(CpLength{4U})), 2026);
        REQUIRE_EQUAL(fixedDecimalReader.peek(), U'-');

        auto fixedHexReader = StringCharReader{U8StringEditor{std::string_view{"ff:"}}};
        REQUIRE_EQUAL(
            fixedHexReader.readIntegerOrThrow<std::uint8_t>(IntegerParseOptions::fixedHex(CpLength{2U})),
            std::uint8_t{255U});
        REQUIRE_EQUAL(fixedHexReader.peek(), U':');

        WITH_CONTEXT(requireReadIntegerNumberError(
            "fx", IntegerParseOptions::fixedHex(CpLength{2U}), ReadNumberStatus::TooFewDigits, CpIndex{1U}));
        WITH_CONTEXT(requireReadIntegerNumberError(
            "123", integerOptions(IntegerBase::Decimal, CpLength{2U}), ReadNumberStatus::TooManyDigits, CpIndex{2U}));
        WITH_CONTEXT(requireReadIntegerNumberError(
            "0x1", IntegerParseOptions::fixedHex(CpLength{2U}), ReadNumberStatus::ParseError, CpIndex{1U}));
        WITH_CONTEXT(requireReadIntegerNumberError(
            "18446744073709551616",
            integerOptions(IntegerBase::Decimal, CpLength{20U}),
            ReadNumberStatus::Overflow,
            CpIndex{19U}));
    }

    void testReadIntegerOrThrow2() {
        auto targetOverflowReader = StringCharReader{U8StringEditor{std::string_view{"128!"}}};
        try {
            static_cast<void>(
                targetOverflowReader.readIntegerOrThrow<std::int8_t>(integerOptions(IntegerBase::Decimal)));
            REQUIRE(false);
        } catch (const OverflowError &error) {
            REQUIRE(targetOverflowReader.position().isZero());
        } catch (const erbsland::unittest::AssertFailed &) {
            throw;
        }
    }

    void testReadIntegerOrThrow3() {
        auto unsignedNegativeReader = StringCharReader{U8StringEditor{std::string_view{"-1!"}}};
        try {
            auto signedOptions = integerOptions(IntegerBase::Decimal);
            signedOptions.addFlags(IntegerParseFlag::AcceptMinusSign);
            static_cast<void>(unsignedNegativeReader.readIntegerOrThrow<std::uint32_t>(signedOptions));
            REQUIRE(false);
        } catch (const OverflowError &error) {
            REQUIRE(unsignedNegativeReader.position().isZero());
        }
    }

    void testReadIntegerOrThrow4() {
        auto invalidReader = StringCharReader{U8StringEditor{std::string_view{th::stdStringFromHex("31 C0 32")}}};
        try {
            static_cast<void>(
                invalidReader.readIntegerOrThrow<std::int32_t>(IntegerParseOptions::fixedDecimal(CpLength{2U})));
            REQUIRE(false);
        } catch (const ParseNumberError &) {
            REQUIRE(invalidReader.position().isZero());
        }
    }

    void testInvalidEncodingIsTolerant() {
        auto utf8Reader = StringCharReader{U8StringEditor{std::string_view{th::stdStringFromHex("41 C0 42")}}};
        REQUIRE_EQUAL(utf8Reader.read().toRawValue(), U'A');
        REQUIRE(utf8Reader.peek().isReplacement());
        REQUIRE(utf8Reader.read().isReplacement());
        REQUIRE_EQUAL(utf8Reader.read().toRawValue(), U'B');

        const auto invalidUtf16 = std::u16string{u'A', char16_t{0xD800U}, u'B'};
        auto utf16Reader = StringCharReader{U16StringEditor{std::u16string_view{invalidUtf16}}};
        REQUIRE_EQUAL(utf16Reader.read().toRawValue(), U'A');
        REQUIRE(utf16Reader.peek().isReplacement());
        REQUIRE(utf16Reader.read().isReplacement());
        REQUIRE_EQUAL(utf16Reader.read().toRawValue(), U'B');

        const auto invalidUtf32 = std::u32string{U'A', char32_t{0x110000U}, U'B'};
        auto utf32Reader = StringCharReader{U32StringEditor{std::u32string_view{invalidUtf32}}};
        REQUIRE_EQUAL(utf32Reader.read().toRawValue(), U'A');
        REQUIRE(utf32Reader.peek().isReplacement());
        REQUIRE(utf32Reader.read().isReplacement());
        REQUIRE_EQUAL(utf32Reader.read().toRawValue(), U'B');
    }

private:
    [[nodiscard]] static auto matchingSet() -> CharSet {
        auto result = CharSet{};
        result.add(Char{U'\u00A2'});
        result.add(Char{U'\u20AC'});
        result.add(Char{U'\U0001F600'});
        return result;
    }

    [[nodiscard]] static auto nonMatchingSet() -> CharSet {
        auto result = CharSet{};
        result.add(Char{U'!'});
        result.add(Char{U'?'});
        return result;
    }

    [[nodiscard]] static auto loopTextSet() -> CharSet {
        auto result = CharSet{};
        result.add(Char{U'A'});
        result.add(Char{U'B'});
        result.add(Char{U'a'});
        result.add(Char{U'b'});
        result.add(Char{U'x'});
        result.add(Char{U'y'});
        result.add(Char{U'z'});
        result.add(Char{U'\u00A2'});
        result.add(Char{U'\u20AC'});
        result.add(Char{U'\U0001F600'});
        return result;
    }

    [[nodiscard]] static auto collectText(std::u32string &text) -> StringCharReader::ReadFn {
        return [&text](const Char character) -> LoopStatus {
            text.push_back(character.toRawValue());
            return LoopStatus::Continue;
        };
    }

    [[nodiscard]] static auto integerOptions(IntegerBase base, CpLength maximumDigits = CpLength::infinite()) noexcept
        -> IntegerParseOptions {
        auto result = IntegerParseOptions::parserDefault();
        result.setFixedBase(base).setMaximumDigits(maximumDigits);
        return result;
    }

    void requireReadLoopCallbackResult(
        StringCharReader reader, Char trigger, LoopStatus status, LoopResult expectedResult, CpIndex expectedPosition) {
        const auto result = reader.readWhile(
            [trigger, status](
                const Char character) -> LoopStatus { return character == trigger ? status : LoopStatus::Continue; },
            loopTextSet());
        REQUIRE_EQUAL(result, expectedResult);
        REQUIRE_EQUAL(reader.position(), expectedPosition);
        REQUIRE_EQUAL(reader.peek(), trigger);
    }

    void requireCapture(const AnyString &capture, StringKind expectedKind, std::u32string_view expectedText) {
        REQUIRE(capture.kind().has_value());
        REQUIRE_EQUAL(capture.kind().value(), expectedKind);
        REQUIRE_EQUAL(StringConverter{capture.toU32String()}.toStdU32String(), std::u32string{expectedText});
    }

    void requireBufferView(const AnyString &buffer, StringKind expectedKind, std::u32string_view expectedText) {
        REQUIRE(buffer.kind().has_value());
        REQUIRE_EQUAL(buffer.kind().value(), expectedKind);
        REQUIRE_EQUAL(buffer.characterLength(), CpLength::fromSizeTOrThrow(expectedText.size()));
        REQUIRE_EQUAL(StringConverter{buffer.toU32String()}.toStdU32String(), std::u32string{expectedText});
    }

    void requireBuffer(const AnyString &buffer, StringKind expectedKind, std::u32string_view expectedText) {
        REQUIRE(buffer.kind().has_value());
        REQUIRE_EQUAL(buffer.kind().value(), expectedKind);
        REQUIRE_EQUAL(buffer.characterLength(), CpLength::fromSizeTOrThrow(expectedText.size()));
        REQUIRE_EQUAL(StringConverter{buffer.toU32String()}.toStdU32String(), std::u32string{expectedText});
    }

    void requireReadIntegerNumberError(
        std::string_view text, IntegerParseOptions options, ReadNumberStatus status, CpIndex position) {
        auto reader = StringCharReader{U8StringEditor{text}};
        try {
            static_cast<void>(reader.readIntegerOrThrow<std::uint64_t>(options));
            REQUIRE(false);
        } catch (const ParseNumberError &error) {
            REQUIRE_EQUAL(error.status(), status);
            REQUIRE_EQUAL(error.position(), position);
            REQUIRE(reader.position().isZero());
        } catch (const OverflowError &error) {
            REQUIRE_EQUAL(status, ReadNumberStatus::Overflow);
            REQUIRE(reader.position().isZero());
        }
    }
};
