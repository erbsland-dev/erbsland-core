// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/impl/UnsafeU8StringAccess.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringCharView.hpp>
#include <erbsland/text/u8/U8StringView.hpp>
#include <erbsland/text/u8/U8StringViewList.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/CpRange.hpp>
#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unit/ElementIndex.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/LoopResult.hpp>
#include <erbsland/util/LoopStatus.hpp>

#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

using el::bgeo::Alignment;
using namespace el::text;
using namespace el::unit;
using el::util::LoopResult;
using el::util::LoopStatus;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(
    U8String U8StringView U8StringCharView U8StringAppendTools U8StringModifyTools U8StringTransformTools
        U8StringSharedStorage)
class U8StringModifierTest final : public el::UnitTest {
public:
    void testAppendReusesCapacityAndKeepsNullTerminator() {
        using namespace el::text::literals;

        auto text = U8String{std::string_view{"Hi"}};
        text.reserve(ByteLength{16U});
        const auto *data = el::text::impl::UnsafeU8StringAccess{text}.data();

        text.append(", "_elv).append(Char{U'X'}).append(Char{U'!'}, CpLength{2U}).append("?"_elv, ElementCount{2U});

        REQUIRE_EQUAL(text, "Hi, X!!??"_el);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.data(), data);
        REQUIRE_EQUAL(text.capacity(), ByteLength{16U});
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.data()[text.length().toSizeT()], '\0');
    }

    void testAppendMaterializesSharedAndSlicedStorage() {
        using namespace el::text::literals;

        auto base = U8String{std::string_view{"abcdef"}};
        auto text = base.slice(ByteRange{ByteIndex{2U}, ByteLength{2U}});
        const auto copy = text;
        const auto *copyData = el::text::impl::UnsafeU8StringAccess{copy}.data();

        text.append("!"_elv);

        REQUIRE_EQUAL(text, "cd!"_el);
        REQUIRE_EQUAL(copy, "cd"_el);
        REQUIRE_EQUAL(base, "abcdef"_el);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{copy}.data(), copyData);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.data()[text.length().toSizeT()], '\0');
    }

    void testRemoveKeepAndCopyVariants() {
        using namespace el::text::literals;

        auto text = U8String{std::u8string_view{u8"A¢€😀"}};

        REQUIRE_EQUAL(
            StringConverter{text.removed(ByteRange{ByteIndex{1U}, ByteLength{5U}})}.toStdU8String(),
            std::u8string{u8"A😀"});
        REQUIRE_EQUAL(
            StringConverter{text.kept(ByteRange{ByteIndex{1U}, ByteLength{5U}})}.toStdU8String(),
            std::u8string{u8"¢€"});
        REQUIRE_EQUAL(
            StringConverter{text.removed(CpRange{CpIndex{1U}, CpLength{2U}})}.toStdU8String(), std::u8string{u8"A😀"});
        REQUIRE_EQUAL(
            StringConverter{text.kept(CpRange{CpIndex{1U}, CpLength{2U}})}.toStdU8String(), std::u8string{u8"¢€"});

        auto byteText = text;
        byteText.remove(ByteRange{ByteIndex{1U}, ByteLength{5U}});
        REQUIRE_EQUAL(byteText, u8"A😀"_el);

        byteText.keep(ByteRange::noRange());
        REQUIRE(byteText.isEmpty());

        text.remove(CpRange{CpIndex{1U}, CpLength{2U}});

        REQUIRE_EQUAL(text, u8"A😀"_el);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.data()[text.length().toSizeT()], '\0');
    }

    void testInPlaceRemoveKeepReuseUniqueFullRangeStorage() {
        using namespace el::text::literals;

        auto rangeText = U8String{std::string_view{"abcdef"}};
        rangeText.reserve(ByteLength{16U});
        const auto *rangeData = el::text::impl::UnsafeU8StringAccess{rangeText}.data();

        rangeText.remove(CpRange{CpIndex{2U}, CpLength{2U}});

        REQUIRE_EQUAL(rangeText, "abef"_el);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{rangeText}.data(), rangeData);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{rangeText}.data()[rangeText.length().toSizeT()], '\0');

        auto setText = U8String{std::string_view{"axbxcx"}};
        setText.reserve(ByteLength{16U});
        const auto *setData = el::text::impl::UnsafeU8StringAccess{setText}.data();

        setText.removeAll(CharSet{"x"_elv});

        REQUIRE_EQUAL(setText, "abc"_el);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{setText}.data(), setData);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{setText}.data()[setText.length().toSizeT()], '\0');

        auto needleText = U8String{std::string_view{"abcabc"}};
        needleText.reserve(ByteLength{16U});
        const auto *needleData = el::text::impl::UnsafeU8StringAccess{needleText}.data();

        needleText.removeAll("bc"_elv);

        REQUIRE_EQUAL(needleText, "aa"_el);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{needleText}.data(), needleData);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{needleText}.data()[needleText.length().toSizeT()], '\0');

        auto keepText = U8String{std::string_view{"abcdef"}};
        keepText.reserve(ByteLength{16U});
        const auto *keepData = el::text::impl::UnsafeU8StringAccess{keepText}.data();

        keepText.keep(CpRange{CpIndex{2U}, CpLength{3U}});

        REQUIRE_EQUAL(keepText, "cde"_el);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{keepText}.data(), keepData);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{keepText}.data()[keepText.length().toSizeT()], '\0');
    }

    void testInsertReplaceAndFirstModifiers() {
        using namespace el::text::literals;

        auto text = U8String{std::string_view{"abef"}};
        text.reserve(ByteLength{16U});
        const auto *data = el::text::impl::UnsafeU8StringAccess{text}.data();

        text.insert(ByteIndex{2U}, "cd"_elv);
        text.replace(ByteRange{ByteIndex{2U}, ByteLength{2U}}, "XY"_elv);
        text.insert(ByteIndex{99U}, "!"_elv);
        text.insert(ByteIndex::noIndex(), "?"_elv);
        text.replace(ByteRange::noRange(), "?"_elv);

        REQUIRE_EQUAL(text, "abXYef!"_el);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.data(), data);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.data()[text.length().toSizeT()], '\0');

        auto codePointText = U8String{std::u8string_view{u8"A😀"}};
        codePointText.insert(CpIndex{1U}, u8"¢"_elv);
        codePointText.replace(CpRange{CpIndex{1U}, CpLength{1U}}, u8"€"_elv);

        REQUIRE_EQUAL(codePointText, u8"A€😀"_el);
        REQUIRE_EQUAL(codePointText.inserted(CpIndex{1U}, u8"¢"_elv), u8"A¢€😀"_el);
        REQUIRE_EQUAL(codePointText.replaced(CpRange{CpIndex{1U}, CpLength{1U}}, u8"¢"_elv), u8"A¢😀"_el);

        auto firstText = U8String{std::string_view{"one one"}};
        firstText.removeFirst("one"_elv);
        REQUIRE_EQUAL(firstText, " one"_el);
        firstText.replaceFirst("ONE"_elv, "two"_elv, Char::compareCaseFolded);
        REQUIRE_EQUAL(firstText, " two"_el);

        const auto source = U8String{std::string_view{"one one"}};
        REQUIRE_EQUAL(source.removedFirst("one"_elv), " one"_el);
        REQUIRE_EQUAL(source.replacedFirst("one"_elv, "two"_elv), "two one"_el);
        REQUIRE_EQUAL(source.removedFirst(U8StringView{}), source);
    }

    void testAliasingSlicedAndMalformedNativeEditing() {
        using namespace el::text::literals;

        auto insertAlias = U8String{std::string_view{"abcdef"}};
        insertAlias.reserve(ByteLength{16U});
        const auto insertView = U8StringView{insertAlias}.slice(ByteRange{ByteIndex{1U}, ByteLength{3U}});
        insertAlias.insert(ByteIndex{3U}, insertView);

        REQUIRE_EQUAL(insertAlias, "abcbcddef"_el);

        auto replaceAlias = U8String{std::string_view{"abcdef"}};
        replaceAlias.reserve(ByteLength{16U});
        const auto replaceView = U8StringView{replaceAlias}.slice(ByteRange{ByteIndex{1U}, ByteLength{4U}});
        replaceAlias.replace(ByteRange{ByteIndex{2U}, ByteLength{2U}}, replaceView);

        REQUIRE_EQUAL(replaceAlias, "abbcdeef"_el);

        auto base = U8String{std::string_view{"xxabcdefyy"}};
        auto sliced = base.slice(ByteRange{ByteIndex{2U}, ByteLength{6U}});
        sliced.insert(ByteIndex{3U}, "!"_elv);

        REQUIRE_EQUAL(sliced, "abc!def"_el);
        REQUIRE_EQUAL(base, "xxabcdefyy"_el);

        auto malformed = U8String{std::string_view{invalidUtf8Data()}};
        malformed.replace(ByteRange{ByteIndex{1U}, ByteLength{1U}}, "?"_elv);
        malformed.insert(ByteIndex{99U}, "!"_elv);

        REQUIRE_EQUAL(malformed, "A?B!"_el);
    }

    void testRemoveAndReplaceCharacterSets() {
        using namespace el::text::literals;

        auto text = U8String{std::u8string_view{u8"axbxcx"}};

        REQUIRE_EQUAL(text.removedAll(CharSet{"x"_elv}), "abc"_el);
        REQUIRE_EQUAL(text.replacedAll(CharSet{"x"_elv}, Char{U'-'}), "a-b-c-"_el);
        REQUIRE_EQUAL(text.replacedAll(CharSet{"x"_elv}, "++"_elv), "a++b++c++"_el);

        text.removeAll(CharSet{"x"_elv});

        REQUIRE_EQUAL(text, "abc"_el);
    }

    void testRemoveAndReplaceTextNeedles() {
        using namespace el::text::literals;

        const auto text = U8String{std::string_view{"aaaa"}};

        REQUIRE(text.removedAll("aa"_elv).isEmpty());
        REQUIRE_EQUAL(text.replacedAll("aa"_elv, "b"_elv), "bb"_el);
        REQUIRE_EQUAL(text.replacedAll(U8StringView{}, "x"_elv), "aaaa"_el);
    }

    void testCaseInsensitiveModifiers() {
        using namespace el::text::literals;

        auto text = U8String{std::u8string_view{u8"ÄxxK"}};

        REQUIRE_EQUAL(text.removedAll(CharSet{u8"Ää"_elv}), u8"xxK"_el);
        REQUIRE_EQUAL(text.removedAll(u8"äX"_elv, Char::compareCaseFolded), u8"xK"_el);
        REQUIRE_EQUAL(text.replacedAll(CharSet{u8"kKK"_elv}, Char{U'K'}), u8"ÄxxK"_el);
        REQUIRE_EQUAL(text.replacedAll(u8"äX"_elv, "Q"_elv, Char::compareCaseFolded), u8"QxK"_el);
    }

    void testViewAndCharViewCopyModifiers() {
        using namespace el::text::literals;

        const auto text = U8String{std::string_view{"--alpha--"}};
        const auto view = U8StringView{text}.slice(ByteRange{ByteIndex{2U}, ByteLength{5U}});
        const auto charView = view.toCharView();

        REQUIRE_EQUAL(view.replacedAll(CharSet{"a"_elv}, "A"_elv), "AlphA"_el);
        REQUIRE_EQUAL(view.inserted(ByteIndex{2U}, "!"_elv), "al!pha"_el);
        REQUIRE_EQUAL(view.replaced(ByteRange{ByteIndex{2U}, ByteLength{2U}}, "X"_elv), "alXa"_el);
        REQUIRE_EQUAL(view.removedFirst("ph"_elv), "ala"_el);
        REQUIRE_EQUAL(view.replacedFirst("AL"_elv, "AL"_elv, Char::compareCaseFolded), "ALpha"_el);
        REQUIRE_EQUAL(
            StringConverter{charView.removed(CpRange{CpIndex{1U}, CpLength{3U}})}.toStdString(), std::string{"aa"});
        REQUIRE_EQUAL(StringConverter{charView.inserted(CpIndex{2U}, "!"_elv)}.toStdString(), std::string{"al!pha"});
        REQUIRE_EQUAL(
            StringConverter{charView.replaced(CpRange{CpIndex{2U}, CpLength{2U}}, "X"_elv)}.toStdString(),
            std::string{"alXa"});
        REQUIRE_EQUAL(StringConverter{charView.removedFirst("ph"_elv)}.toStdString(), std::string{"ala"});
        REQUIRE_EQUAL(
            StringConverter{charView.replacedFirst("AL"_elv, "AL"_elv, Char::compareCaseFolded)}.toStdString(),
            std::string{"ALpha"});
    }

    void testSplitAndJoin() {
        using namespace el::text::literals;

        const auto text = U8String{std::string_view{",a,,b,"}};
        const auto parts = U8StringViewList::fromSplit(text, CharSet{","_elv});
        const auto keptParts = U8StringViewList::fromSplit(text, CharSet{","_elv}, ElementCount::infinite(), true);

        REQUIRE_EQUAL(parts.count().toSizeT(), std::size_t{2U});
        REQUIRE_EQUAL(parts[ElementIndex{0}], "a"_el);
        REQUIRE_EQUAL(parts[ElementIndex{1}], "b"_el);
        REQUIRE_EQUAL(keptParts.count().toSizeT(), std::size_t{5U});
        REQUIRE_EQUAL(U8StringViewList::fromSplit(text, CharSet{","_elv}).count().toSizeT(), std::size_t{2U});
        REQUIRE_EQUAL(parts.join("|"_elv), "a|b"_el);
        REQUIRE_EQUAL(parts.join(), "ab"_el);
        REQUIRE_EQUAL(U8StringViewList::fromSplit(u8"ÄxK"_elv, CharSet{u8"kKK"_elv}).first(), u8"Äx"_el);
    }

    void testForEachTransformAndJustify() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"Ab¢"}};
        _seenCharacters.clear();

        const auto completed = text.forEach(collectUntilLowerB);

        REQUIRE_EQUAL(completed, LoopResult::Stopped);
        REQUIRE_EQUAL(_seenCharacters, std::u32string{U"Ab"});
        REQUIRE_EQUAL(text.forEach(nullptr), LoopResult::Success);
        REQUIRE_EQUAL(text.transformed(lowerUpperA), u8"ab¢"_el);
        REQUIRE_EQUAL(text.transformed(nullptr), text);
        REQUIRE_EQUAL(text.transformed(stopAtLowerB), "A"_el);
        REQUIRE_EQUAL(text.transformed(skipLowerB), u8"A¢"_el);
        REQUIRE_EQUAL(text.aligned(CpLength{5U}, Alignment::Left, Char{U'.'}), u8"Ab¢.."_el);
        REQUIRE_EQUAL(text.aligned(CpLength{5U}, Alignment::Right, Char{U'.'}), u8"..Ab¢"_el);
    }

    void testInvalidUtf8ModifierBehavior() {
        using namespace el::text::literals;

        const auto text = U8String{std::string_view{invalidUtf8Data()}};

        REQUIRE_EQUAL(rawBytes(text.removedAll(CharSet{"x"_elv})), invalidUtf8Data());
        REQUIRE_EQUAL(rawBytes(text.replacedAll("x"_elv, "?"_elv)), invalidUtf8Data());
        REQUIRE_EQUAL(text.removedAll(CharSet{Char::replacement()}), "AB"_el);
        REQUIRE_EQUAL(text.replacedAll(CharSet{Char::replacement()}, "?"_elv), "A?B"_el);
    }

    void testSizeAccountingRejectsOverflow() {
        constexpr auto maximum = std::numeric_limits<std::size_t>::max();

        REQUIRE_THROWS(el::text::impl::U8StringSharedStorage::checkedAddSize(maximum, 1U, "test overflow"));
        REQUIRE_THROWS(el::text::impl::U8StringSharedStorage::checkedMultiplySize(maximum, 2U, "test overflow"));
    }

private:
    [[nodiscard]] static auto rawBytes(const U8String &text) -> std::string {
        return std::string{el::text::impl::UnsafeU8StringAccess{text}.data(), text.length().toSizeT()};
    }

    [[nodiscard]] static auto invalidUtf8Data() -> std::string {
        auto result = std::string{"A"};
        result.push_back(static_cast<char>(0xC0U));
        result.push_back('B');
        return result;
    }
    static auto collectUntilLowerB(const Char character) noexcept -> LoopStatus {
        _seenCharacters.push_back(character.toRawValue());
        return character == Char{U'b'} ? LoopStatus::Stop : LoopStatus::Continue;
    }
    static auto lowerUpperA(const Char character) noexcept -> Char {
        return character == Char{U'A'} ? Char{U'a'} : character;
    }
    static auto stopAtLowerB(const Char character) noexcept -> Char {
        return character == Char{U'b'} ? Char::endOfData() : character;
    }
    static auto skipLowerB(const Char character) noexcept -> Char {
        return character == Char{U'b'} ? Char::noCodePoint() : character;
    }

private:
    static inline auto _seenCharacters = std::u32string{};
};
