// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Literals.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/U16StringSplitter.hpp>
#include <erbsland/U32StringSplitter.hpp>
#include <erbsland/U8StringSplitter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>

using namespace el::text;
using namespace el::text::literals;
using el::unit::ItemCount;
using el::unit::ItemIndex;
using el::util::LoopResult;
using el::util::LoopStatus;

static_assert(std::is_same_v<StringSplitter, U8StringSplitter>);

TESTED_TARGETS(StringSplitMode StringSplitter U8StringSplitter U16StringSplitter U32StringSplitter)
class StringSplitterTest final : public el::UnitTest {
public:
    void testDiscardSeparator() {
        auto splitter = StringSplitter{",alpha,,beta,"_el, Char{U','}};

        REQUIRE_FALSE(splitter.isAtEnd());
        REQUIRE_EQUAL(splitter.remaining(), ",alpha,,beta,"_el);
        REQUIRE_EQUAL(splitter.next(), ""_el);
        REQUIRE_EQUAL(splitter.next(), "alpha"_el);
        REQUIRE_EQUAL(splitter.next(), ""_el);
        REQUIRE_EQUAL(splitter.next(), "beta"_el);
        REQUIRE_FALSE(splitter.isAtEnd());
        REQUIRE_EQUAL(splitter.next(), ""_el);
        REQUIRE(splitter.isAtEnd());
        REQUIRE(splitter.next().isEmpty());
        REQUIRE(splitter.remaining().isEmpty());
    }

    void testKeepSeparator() {
        auto splitter = StringSplitter{"one\ntwo\n\n"_el, Char{U'\n'}, StringSplitMode::KeepSeparator};

        REQUIRE_EQUAL(splitter.next(), "one\n"_el);
        REQUIRE_EQUAL(splitter.next(), "two\n"_el);
        REQUIRE_EQUAL(splitter.next(), "\n"_el);
        REQUIRE(splitter.isAtEnd());
        REQUIRE(splitter.next().isEmpty());
    }

    void testCharacterSetAndUnicode() {
        auto splitter = StringSplitter{"one,two;three☃four"_el, CharSet{",;☃"_el}};

        REQUIRE_EQUAL(splitter.next(), "one"_el);
        REQUIRE_EQUAL(splitter.next(), "two"_el);
        REQUIRE_EQUAL(splitter.next(), "three"_el);
        REQUIRE_EQUAL(splitter.next(), "four"_el);
        REQUIRE(splitter.isAtEnd());
    }

    void testEmptyAndNoSeparator() {
        auto empty = StringSplitter{""_el, Char{U','}};
        REQUIRE_FALSE(empty.isAtEnd());
        REQUIRE(empty.next().isEmpty());
        REQUIRE(empty.isAtEnd());

        auto noSeparator = StringSplitter{"complete"_el, CharSet{}};
        REQUIRE_EQUAL(noSeparator.next(), "complete"_el);
        REQUIRE(noSeparator.isAtEnd());
    }

    void testResetAndSourceAccess() {
        auto splitter = StringSplitter{"a,b"_el, Char{U','}};
        REQUIRE_EQUAL(splitter.text(), "a,b"_el);
        REQUIRE_EQUAL(splitter.next(), "a"_el);
        REQUIRE_EQUAL(splitter.remaining(), "b"_el);

        splitter.reset();
        REQUIRE_FALSE(splitter.isAtEnd());
        REQUIRE_EQUAL(splitter.remaining(), "a,b"_el);
        REQUIRE_EQUAL(splitter.next(), "a"_el);
    }

    void testSkipPartsWithoutCreatingSlices() {
        auto discard = StringSplitter{"first,,third"_el, Char{U','}};
        discard.skip();
        REQUIRE_EQUAL(discard.remaining(), ",third"_el);
        discard.skip();
        REQUIRE_EQUAL(discard.next(), "third"_el);
        REQUIRE(discard.isAtEnd());
        discard.skip();
        REQUIRE(discard.isAtEnd());

        auto keep = StringSplitter{"one\ntwo\n"_el, Char{U'\n'}, StringSplitMode::KeepSeparator};
        keep.skip();
        REQUIRE_EQUAL(keep.remaining(), "two\n"_el);
        keep.skip();
        REQUIRE(keep.isAtEnd());
    }

    void testSplitCounters() {
        auto splitter = StringSplitter{",alpha,,beta,"_el, U','};
        REQUIRE_EQUAL(splitter.splitCount(), el::unit::ItemCount{0});
        REQUIRE_EQUAL(splitter.splitIndex(), el::unit::ItemIndex{0});
        for (auto index = el::unit::ItemIndex{}; index.isWithin(el::unit::ItemCount{5U}); index.uncheckedIncrement()) {
            REQUIRE_EQUAL(splitter.splitIndex(), index);
            if (index.toRawValue() % 2U == 0U) {
                splitter.skip();
            } else {
                [[maybe_unused]] const auto part = splitter.next();
            }
            REQUIRE_EQUAL(splitter.splitCount(), index.incremented().distanceFromZero());
        }
        REQUIRE(splitter.isAtEnd());
        REQUIRE(splitter.next().isEmpty());
        splitter.skip();
        REQUIRE_EQUAL(splitter.splitCount(), el::unit::ItemCount{5});
        REQUIRE_EQUAL(splitter.splitIndex(), el::unit::ItemIndex{5});
        splitter.reset();
        REQUIRE_EQUAL(splitter.splitCount(), el::unit::ItemCount{0});
        REQUIRE_EQUAL(splitter.splitIndex(), el::unit::ItemIndex{0});
        REQUIRE(splitter.next().isEmpty());
        REQUIRE_EQUAL(splitter.splitCount(), el::unit::ItemCount{1});
    }

    void testSplitCountersForEmptyAndUnseparatedText() {
        for (const auto &text : StringList{""_el, "single"_el}) {
            for (const auto mode : {StringSplitMode::DiscardSeparator, StringSplitMode::KeepSeparator}) {
                auto splitter = StringSplitter{text, CharSet{}, mode};
                REQUIRE_EQUAL(splitter.splitCount(), el::unit::ItemCount{0});
                REQUIRE_EQUAL(splitter.next(), text);
                REQUIRE(splitter.isAtEnd());
                REQUIRE_EQUAL(splitter.splitCount(), el::unit::ItemCount{1});
                REQUIRE_EQUAL(splitter.splitIndex(), el::unit::ItemIndex{1});
                splitter.skip();
                REQUIRE_EQUAL(splitter.splitCount(), el::unit::ItemCount{1});
            }
        }
    }

    void testSplitCountersAcrossWidthsAndCopies() {
        auto splitter16 = U16StringSplitter{u"a☃b☃"_el, U'☃', StringSplitMode::KeepSeparator};
        REQUIRE_EQUAL(splitter16.splitIndex(), el::unit::ItemIndex{0});
        REQUIRE_EQUAL(splitter16.next(), u"a☃"_el);
        auto copy = splitter16;
        splitter16.skip();
        REQUIRE(splitter16.isAtEnd());
        REQUIRE_EQUAL(splitter16.splitCount(), el::unit::ItemCount{2});
        REQUIRE_EQUAL(copy.splitCount(), el::unit::ItemCount{1});
        REQUIRE_EQUAL(copy.next(), u"b☃"_el);
        REQUIRE_EQUAL(copy.splitIndex(), el::unit::ItemIndex{2});
        copy.reset();
        REQUIRE_EQUAL(copy.splitCount(), el::unit::ItemCount{0});
        REQUIRE_EQUAL(splitter16.splitCount(), el::unit::ItemCount{2});

        auto splitter32 = U32StringSplitter{U"a☃b☃"_el, U'☃'};
        for (auto index = el::unit::ItemIndex{}; index.isWithin(el::unit::ItemCount{3U}); index.uncheckedIncrement()) {
            REQUIRE_EQUAL(splitter32.splitIndex(), index);
            splitter32.skip();
        }
        REQUIRE(splitter32.isAtEnd());
        REQUIRE_EQUAL(splitter32.splitCount(), el::unit::ItemCount{3});
    }

    void testMalformedUtf8IsPreserved() {
        auto malformed = std::string{"left"};
        malformed.push_back(static_cast<char>(0x80U));
        malformed.append(",right");
        auto splitter = StringSplitter{String{malformed}, Char{U','}};

        const auto first = splitter.next();
        REQUIRE_FALSE(first.isEncodingValid());
        REQUIRE_EQUAL(first.length().toSizeT(), std::size_t{5U});
        REQUIRE_EQUAL(splitter.next(), "right"_el);
        REQUIRE(splitter.isAtEnd());
    }

    void testForEachParts() {
        auto splitter = StringSplitter{",alpha,,beta,"_el, U','};
        auto parts = StringList{};
        const auto collect = [&](const String &part) -> LoopStatus {
            parts.append(part);
            return LoopStatus::Continue;
        };
        REQUIRE_EQUAL(splitter.forEach(collect), LoopResult::Success);
        REQUIRE_EQUAL(parts, (StringList{""_el, "alpha"_el, ""_el, "beta"_el, ""_el}));
        REQUIRE(splitter.isAtEnd());
        REQUIRE_EQUAL(splitter.splitCount(), ItemCount{5});
        REQUIRE_EQUAL(splitter.forEach(collect), LoopResult::Success);
        REQUIRE_EQUAL(parts.count(), ItemCount{5});
        splitter.reset();
        parts.clear();
        REQUIRE_EQUAL(splitter.forEach(collect), LoopResult::Success);
        REQUIRE_EQUAL(parts.count(), ItemCount{5});
    }

    void testForEachIndexedRemainingParts() {
        auto splitter = StringSplitter{"first,second,,fourth"_el, U','};
        REQUIRE_EQUAL(splitter.next(), "first"_el);
        splitter.skip();
        auto expectedIndex = ItemIndex{2};
        auto parts = StringList{};
        const auto result = splitter.forEach([&](const String &part, const ItemIndex index) -> LoopStatus {
            REQUIRE_EQUAL(index, expectedIndex);
            REQUIRE_EQUAL(splitter.splitIndex(), index + ItemCount{1});
            ++expectedIndex;
            parts.append(part);
            return LoopStatus::Continue;
        });
        REQUIRE_EQUAL(result, LoopResult::Success);
        REQUIRE_EQUAL(parts, (StringList{""_el, "fourth"_el}));
        REQUIRE_EQUAL(splitter.splitCount(), ItemCount{4});
        REQUIRE(splitter.isAtEnd());
    }

    void testForEachStopAndError() {
        for (const auto status : {LoopStatus::Stop, LoopStatus::Error}) {
            auto expectedResult = LoopResult::Stopped;
            if (status == LoopStatus::Error) {
                expectedResult = LoopResult::Error;
            }
            auto splitter = StringSplitter{"first,second,third"_el, U','};
            REQUIRE_EQUAL(
                splitter.forEach([&](const String &part) -> LoopStatus {
                    REQUIRE_EQUAL(part, "first"_el);
                    return status;
                }),
                expectedResult);
            REQUIRE_EQUAL(splitter.remaining(), "second,third"_el);
            REQUIRE_EQUAL(splitter.splitCount(), ItemCount{1});
            REQUIRE_EQUAL(
                splitter.forEach([&](const String &part, const ItemIndex index) -> LoopStatus {
                    REQUIRE_EQUAL(part, "second"_el);
                    REQUIRE_EQUAL(index, ItemIndex{1});
                    return status;
                }),
                expectedResult);
            REQUIRE_EQUAL(splitter.remaining(), "third"_el);
            REQUIRE_EQUAL(splitter.splitCount(), ItemCount{2});
            REQUIRE_EQUAL(splitter.next(), "third"_el);
            REQUIRE(splitter.isAtEnd());

            splitter.reset();
            splitter.skip();
            splitter.skip();
            REQUIRE_EQUAL(splitter.forEach([&](const String &) -> LoopStatus { return status; }), expectedResult);
            REQUIRE(splitter.isAtEnd());
            splitter.reset();
            splitter.skip();
            splitter.skip();
            REQUIRE_EQUAL(
                splitter.forEach([&](const String &, ItemIndex) -> LoopStatus { return status; }), expectedResult);
            REQUIRE(splitter.isAtEnd());
        }
    }

    void testForEachEmptyAndUnseparatedText() {
        for (const auto &text : StringList{""_el, "single"_el}) {
            for (const auto mode : {StringSplitMode::DiscardSeparator, StringSplitMode::KeepSeparator}) {
                auto splitter = StringSplitter{text, CharSet{}, mode};
                auto parts = StringList{};
                REQUIRE_EQUAL(
                    splitter.forEach([&](const String &part) -> LoopStatus {
                        parts.append(part);
                        return LoopStatus::Continue;
                    }),
                    LoopResult::Success);
                REQUIRE_EQUAL(parts, StringList{text});
                splitter.reset();
                parts.clear();
                const auto collect = [&](const String &part, const ItemIndex index) -> LoopStatus {
                    REQUIRE_EQUAL(index, ItemIndex{0});
                    parts.append(part);
                    return LoopStatus::Continue;
                };
                REQUIRE_EQUAL(splitter.forEach(collect), LoopResult::Success);
                REQUIRE_EQUAL(parts, StringList{text});
                REQUIRE_EQUAL(splitter.forEach(collect), LoopResult::Success);
                REQUIRE_EQUAL(parts.count(), ItemCount{1});
            }
        }
    }

    void testForEachNullCallbacksAndExceptions() {
        auto splitter = StringSplitter{"first,second"_el, U','};
        REQUIRE_EQUAL(splitter.forEach(StringSplitter::ProcessPartFn{}), LoopResult::Success);
        REQUIRE_EQUAL(splitter.forEach(StringSplitter::ProcessPartWithIndexFn{}), LoopResult::Success);
        REQUIRE_EQUAL(splitter.splitCount(), ItemCount{0});
        REQUIRE_EQUAL(splitter.remaining(), "first,second"_el);
        REQUIRE_THROWS_AS(std::runtime_error, splitter.forEach([](const String &) -> LoopStatus {
            throw std::runtime_error{"callback"};
        }));
        REQUIRE_EQUAL(splitter.splitCount(), ItemCount{1});
        REQUIRE_EQUAL(splitter.remaining(), "second"_el);
        REQUIRE_THROWS_AS(std::runtime_error, splitter.forEach([](const String &, ItemIndex) -> LoopStatus {
            throw std::runtime_error{"callback"};
        }));
        REQUIRE_EQUAL(splitter.splitCount(), ItemCount{2});
        REQUIRE(splitter.isAtEnd());
    }

    void testForEachWidthVariants() {
        auto splitter16 = U16StringSplitter{u"a☃b☃"_el, CharSet{U'☃'}, StringSplitMode::KeepSeparator};
        auto count16 = ItemCount{};
        REQUIRE_EQUAL(
            splitter16.forEach([&](const U16String &part, const ItemIndex index) -> LoopStatus {
                REQUIRE_EQUAL(index, ItemIndex::end(count16));
                REQUIRE_EQUAL(part.length(), el::unit::U16Count{2});
                REQUIRE(part.endsWith(u"☃"_el));
                ++count16;
                return LoopStatus::Continue;
            }),
            LoopResult::Success);
        REQUIRE_EQUAL(count16, ItemCount{2});
        REQUIRE(splitter16.isAtEnd());

        auto splitter32 = U32StringSplitter{U"a☃b☃"_el, U'☃'};
        auto count32 = ItemCount{};
        REQUIRE_EQUAL(
            splitter32.forEach([&](const U32String &part) -> LoopStatus {
                REQUIRE_FALSE(part.contains(U"☃"_el));
                ++count32;
                return LoopStatus::Continue;
            }),
            LoopResult::Success);
        REQUIRE_EQUAL(count32, ItemCount{3});
        REQUIRE(splitter32.isAtEnd());
    }

    void testWidthVariants() {
        auto splitter16 = U16StringSplitter{u"a☃b☃"_el, Char{U'☃'}};
        REQUIRE_EQUAL(splitter16.next(), u"a"_el);
        REQUIRE_EQUAL(splitter16.next(), u"b"_el);
        REQUIRE_EQUAL(splitter16.next(), u""_el);
        REQUIRE(splitter16.isAtEnd());

        auto splitter32 = U32StringSplitter{U"a☃b☃"_el, Char{U'☃'}, StringSplitMode::KeepSeparator};
        REQUIRE_EQUAL(splitter32.next(), U"a☃"_el);
        REQUIRE_EQUAL(splitter32.next(), U"b☃"_el);
        REQUIRE(splitter32.isAtEnd());
    }

    void testEndStateAcrossWidths() {
        const auto checkWidth = [this]<typename tString>(const tString &text) -> void {
            for (const auto &source : {text, tString{}}) {
                for (const auto mode : {StringSplitMode::DiscardSeparator, StringSplitMode::KeepSeparator}) {
                    auto splitter = el::text::impl::StringSplitter<tString>{source, U'☃', mode};
                    auto skipped = splitter;
                    REQUIRE_FALSE(splitter.isAtEnd());
                    while (!splitter.isAtEnd()) {
                        REQUIRE_FALSE(skipped.isAtEnd());
                        [[maybe_unused]] const auto part = splitter.next();
                        skipped.skip();
                        REQUIRE_EQUAL(skipped.isAtEnd(), splitter.isAtEnd());
                        REQUIRE_EQUAL(skipped.remaining(), splitter.remaining());
                    }
                    REQUIRE(splitter.remaining().isEmpty());
                    REQUIRE(skipped.remaining().isEmpty());
                    const auto count = splitter.splitCount();
                    REQUIRE(splitter.next().isEmpty());
                    skipped.skip();
                    REQUIRE_EQUAL(splitter.splitCount(), count);
                    REQUIRE_EQUAL(skipped.splitCount(), count);
                    splitter.reset();
                    REQUIRE_FALSE(splitter.isAtEnd());
                    REQUIRE_EQUAL(splitter.remaining(), source);
                    REQUIRE(skipped.isAtEnd());
                }
            }
        };
        checkWidth(String{"☃a☃☃"_el});
        checkWidth(U16String{u"☃a☃☃"_el});
        checkWidth(U32String{U"☃a☃☃"_el});
    }
};
