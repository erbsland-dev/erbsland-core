// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CharSet.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/text/impl/DecodedStringSearch.hpp>
#include <erbsland/U16String.hpp>
#include <erbsland/U16StringEditor.hpp>
#include <erbsland/U32String.hpp>
#include <erbsland/U32StringEditor.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <compare>
#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

TESTED_TARGETS(String StringEditor U16String U16StringEditor U32String U32StringEditor DecodedStringSearch)
class StringSeparatorSplitTest final : public el::UnitTest {
public:
    void testU8() { WITH_CONTEXT((requireWidth<String, StringEditor>())); }
    void testU16() { WITH_CONTEXT((requireWidth<U16String, U16StringEditor>())); }
    void testU32() { WITH_CONTEXT((requireWidth<U32String, U32StringEditor>())); }

    void testMalformedData() {
        const auto bytes = el::unittest::th::stdStringFromHex("61 f0 90 80 62 ed a0 80 63");
        WITH_CONTEXT((requireMalformed<String, StringEditor>(
            String{std::string_view{bytes}}, String{u8"�"_el}, CpIndex{1}, CpIndex{5})));
        const auto units16 = std::u16string{u'a', char16_t{0xd800}, u'b', char16_t{0xdc00}, u'c'};
        WITH_CONTEXT((requireMalformed<U16String, U16StringEditor>(
            U16String{std::u16string_view{units16}}, U16String{u"�"_el}, CpIndex{1}, CpIndex{3})));
        const auto units32 = std::u32string{U'a', char32_t{0x110000}, U'b', char32_t{0xd800}, U'c'};
        WITH_CONTEXT((requireMalformed<U32String, U32StringEditor>(
            U32String{std::u32string_view{units32}}, U32String{U"�"_el}, CpIndex{1}, CpIndex{3})));
    }

    void testSensitiveResults() {
        auto source = String{"abc/xyz"_el};
        source.markAsSensitive();
        auto editor = StringEditor{source};
        for (const auto separator : {String{"/"_el}, String{"abc"_el}, String{"xyz"_el}, source}) {
            const auto first = source.splitAtFirst(separator);
            const auto last = editor.splitAtLast(separator);
            REQUIRE(first.has_value());
            REQUIRE(last.has_value());
            REQUIRE(first->first.isSensitive());
            REQUIRE(first->second.isSensitive());
            REQUIRE(last->first.isSensitive());
            REQUIRE(last->second.isSensitive());
        }
        const auto parts = source.splitAtLast(CharSet{U'/'});
        REQUIRE(parts.has_value());
        REQUIRE(parts->first.isSensitive());
        REQUIRE(parts->second.isSensitive());
    }

    void testSearchDoesNotReadSourceTwice() {
        const auto pattern = U32String{U"ababaca"_el};
        auto patternReads = std::size_t{};
        const auto readPattern = [&](CpIndex &index) -> Char {
            ++patternReads;
            return pattern.readCharAndAdvance(index);
        };
        const auto search = el::text::impl::DecodedStringSearch{7U, CpIndex::zero(), readPattern, nullptr};
        auto source = U32StringEditor{};
        source.append(U'x', CpCount{1000}).append(pattern).append(U'z');
        auto sourceReads = std::size_t{};
        const auto match = search.findRange(CpIndex::zero(), [&](CpIndex &index) -> Char {
            ++sourceReads;
            return source.readCharAndAdvance(index);
        });
        REQUIRE(match.has_value());
        REQUIRE_EQUAL(match->first, CpIndex{1000});
        REQUIRE_EQUAL(match->second, CpIndex{1007});
        REQUIRE_EQUAL(sourceReads, std::size_t{1007});
        REQUIRE_EQUAL(patternReads, std::size_t{8});
    }

private:
    struct Case {
        String source;
        String separator;
        String firstBefore;
        String firstAfter;
        String lastBefore;
        String lastAfter;
    };

    template <typename S>
    static auto converted(const String &value) -> S {
        if constexpr (std::same_as<S, String>) {
            return value;
        } else if constexpr (std::same_as<S, U16String>) {
            return StringConverter{value}.toU16String();
        } else {
            return StringConverter{value}.toU32String();
        }
    }

    template <typename T, typename S>
    void requireParts(const std::optional<std::pair<T, T>> &parts, const S &before, const S &after) {
        REQUIRE(parts.has_value());
        REQUIRE_EQUAL(S{parts->first}, before);
        REQUIRE_EQUAL(S{parts->second}, after);
    }

    template <typename S, typename E>
    void requireWidth() {
        static_assert(noexcept(std::declval<const S &>().splitAtFirst(std::declval<const S &>())));
        static_assert(std::same_as<
            decltype(std::declval<const E &>().splitAtLast(std::declval<const S &>())),
            std::optional<std::pair<E, E>>>);
        const auto cases = std::array{
            Case{"abc/xyz"_el, "/"_el, "abc"_el, "xyz"_el, "abc"_el, "xyz"_el},
            Case{"abc###xyz###end"_el, "###"_el, "abc"_el, "xyz###end"_el, "abc###xyz"_el, "end"_el},
            Case{"/abc"_el, "/"_el, ""_el, "abc"_el, ""_el, "abc"_el},
            Case{"abc/"_el, "/"_el, "abc"_el, ""_el, "abc"_el, ""_el},
            Case{"/"_el, "/"_el, ""_el, ""_el, ""_el, ""_el},
            Case{"aaa"_el, "aa"_el, ""_el, "a"_el, "a"_el, ""_el},
            Case{"ababa"_el, "aba"_el, ""_el, "ba"_el, "ab"_el, ""_el},
            Case{"a//b"_el, "/"_el, "a"_el, "/b"_el, "a/"_el, "b"_el},
            Case{u8"α😀β😀γ"_el, u8"😀"_el, u8"α"_el, u8"β😀γ"_el, u8"α😀β"_el, u8"γ"_el},
            Case{u8"a¢€b¢€c"_el, u8"¢€"_el, "a"_el, u8"b¢€c"_el, u8"a¢€b"_el, "c"_el}};
        for (const auto &item : cases) {
            const auto separator = converted<S>(item.separator);
            const auto source = converted<S>(item.source);
            const auto padded = S{E{}.append(U'!').append(source).append(U'!')};
            const auto slice = padded.slice(CpRange{CpIndex{1}, source.characterLength()});
            const auto beforeFirst = converted<S>(item.firstBefore);
            const auto afterFirst = converted<S>(item.firstAfter);
            const auto beforeLast = converted<S>(item.lastBefore);
            const auto afterLast = converted<S>(item.lastAfter);
            for (const auto &value : {source, source.copy(), slice}) {
                WITH_CONTEXT(requireParts(value.splitAtFirst(separator), beforeFirst, afterFirst));
                WITH_CONTEXT(requireParts(value.splitAtLast(separator), beforeLast, afterLast));
                const auto editor = E{value};
                WITH_CONTEXT(requireParts(editor.splitAtFirst(separator), beforeFirst, afterFirst));
                WITH_CONTEXT(requireParts(editor.splitAtLast(separator), beforeLast, afterLast));
                if (separator.characterLength() == CpCount::one()) {
                    const auto set = CharSet{separator.charAt(CpIndex::zero())};
                    WITH_CONTEXT(requireParts(value.splitAtFirst(set), beforeFirst, afterFirst));
                    WITH_CONTEXT(requireParts(value.splitAtLast(set), beforeLast, afterLast));
                    WITH_CONTEXT(requireParts(editor.splitAtFirst(set), beforeFirst, afterFirst));
                    WITH_CONTEXT(requireParts(editor.splitAtLast(set), beforeLast, afterLast));
                }
            }
        }
        for (const auto &source : {S{}, converted<S>("abc"_el)}) {
            const auto editor = E{source};
            for (const auto &separator : {S{}, converted<S>("missing"_el)}) {
                REQUIRE(!source.splitAtFirst(separator));
                REQUIRE(!source.splitAtLast(separator));
                REQUIRE(!editor.splitAtFirst(separator));
                REQUIRE(!editor.splitAtLast(separator));
            }
            for (const auto &set : {CharSet{}, CharSet{U'/'}}) {
                REQUIRE(!source.splitAtFirst(set));
                REQUIRE(!source.splitAtLast(set));
                REQUIRE(!editor.splitAtFirst(set));
                REQUIRE(!editor.splitAtLast(set));
            }
        }
        const auto mixed = converted<S>(u8"a/β😀c"_el);
        const auto set = CharSet{U'/', U'😀'};
        WITH_CONTEXT(requireParts(mixed.splitAtFirst(set), converted<S>("a"_el), converted<S>(u8"β😀c"_el)));
        WITH_CONTEXT(requireParts(mixed.splitAtLast(set), converted<S>(u8"a/β"_el), converted<S>("c"_el)));
        const auto nullSource = S{E{}.append(U'a').append(U'\0').append(U'b')};
        const auto nullSeparator = S{E{}.append(U'\0')};
        WITH_CONTEXT(requireParts(nullSource.splitAtFirst(nullSeparator), converted<S>("a"_el), converted<S>("b"_el)));
        WITH_CONTEXT(requireParts(nullSource.splitAtLast(CharSet{U'\0'}), converted<S>("a"_el), converted<S>("b"_el)));
        WITH_CONTEXT((requireStorage<S, E>()));
        WITH_CONTEXT((requireComparisons<S, E>()));
        WITH_CONTEXT((requireLongSearch<S, E>()));
    }

    template <typename S, typename E>
    void requireStorage() {
        auto source = S{E{}.append(converted<S>(u8"!abc/😀xyz!"_el))};
        const auto view = source.slice(CpRange{CpIndex{1}, CpCount{8}});
        const auto expectedFirst = view.slice(CpRange{CpIndex{}, CpCount{3}}).storageId();
        const auto expectedLast = view.slice(CpRange{CpIndex{4}, CpCount{4}}).storageId();
        auto parts = view.splitAtFirst(converted<S>("/"_el));
        REQUIRE(parts.has_value());
        REQUIRE_EQUAL(parts->first.storageId(), expectedFirst);
        REQUIRE_EQUAL(parts->second.storageId(), expectedLast);
        source = {};
        REQUIRE_EQUAL(parts->second, converted<S>(u8"😀xyz"_el));
        auto editor = E{converted<S>("!abc/xyz!"_el)}.slice(CpRange{CpIndex{1}, CpCount{7}});
        auto edited = editor.splitAtLast(CharSet{U'/'});
        REQUIRE(edited.has_value());
        REQUIRE_EQUAL(edited->first.storageId(), editor.slice(CpRange{CpIndex{}, CpCount{3}}).storageId());
        edited->first.append(U'!');
        editor.append(U'?');
        REQUIRE_EQUAL(S{edited->first}, converted<S>("abc!"_el));
        REQUIRE_EQUAL(S{edited->second}, converted<S>("xyz"_el));
        REQUIRE_EQUAL(S{editor}, converted<S>("abc/xyz?"_el));
        const auto retained = E{converted<S>("left/right"_el)}.splitAtFirst(converted<S>("/"_el));
        WITH_CONTEXT(requireParts(retained, converted<S>("left"_el), converted<S>("right"_el)));
    }

    static auto compareMapped(Char left, Char right) noexcept -> std::strong_ordering {
        if (left == U'😀') {
            left = Char{U'x'};
        }
        if (right == U'😀') {
            right = Char{U'x'};
        }
        return left <=> right;
    }

    template <typename S, typename E>
    void requireComparisons() {
        const auto source = converted<S>(u8"aKbKc"_el);
        const auto separator = converted<S>("k"_el);
        WITH_CONTEXT(requireParts(
            source.splitAtFirst(separator, Char::compareCaseFolded), converted<S>("a"_el), converted<S>("bKc"_el)));
        WITH_CONTEXT(requireParts(
            E{source}.splitAtLast(separator, Char::compareCaseFolded), converted<S>(u8"aKb"_el), converted<S>("c"_el)));
        const auto mapped = converted<S>(u8"a😀bxz"_el);
        WITH_CONTEXT(requireParts(
            mapped.splitAtFirst(converted<S>("x"_el), compareMapped), converted<S>("a"_el), converted<S>("bxz"_el)));
        WITH_CONTEXT(requireParts(
            mapped.splitAtLast(converted<S>(u8"😀"_el), compareMapped),
            converted<S>(u8"a😀b"_el),
            converted<S>("z"_el)));
        WITH_CONTEXT(requireParts(
            mapped.splitAtFirst(converted<S>("x"_el), compareSourceMapped),
            converted<S>("a"_el),
            converted<S>("bxz"_el)));
        const auto lastMapped = converted<S>(u8"ax😀z"_el);
        WITH_CONTEXT(requireParts(
            lastMapped.splitAtLast(converted<S>("x"_el), compareSourceMapped),
            converted<S>("ax"_el),
            converted<S>("z"_el)));
        REQUIRE(!source.splitAtFirst(separator));
    }

    static auto compareSourceMapped(Char source, const Char separator) noexcept -> std::strong_ordering {
        if (source == U'😀') {
            source = Char{U'x'};
        }
        return source <=> separator;
    }

    static auto compareCounted(const Char left, const Char right) noexcept -> std::strong_ordering {
        ++_comparisons;
        return left <=> right;
    }

    template <typename S, typename E>
    void requireLongSearch() {
        for (const auto length : {CpCount{63U}, CpCount{64U}, CpCount{65U}, CpCount{256U}}) {
            const auto needle = S{E{}.append(U'a', length - CpCount::one()).append(U'b')};
            const auto left = S{E{}.append(U'a', CpCount{2048})};
            const auto right = S{E{}.append(U'b', CpCount{2048})};
            const auto source = S{E{}.append(left).append(needle).append(right)};
            _comparisons = 0U;
            WITH_CONTEXT(requireParts(source.splitAtFirst(needle, compareCounted), left, right));
            if (length >= CpCount{64U}) {
                REQUIRE_LESS(_comparisons, std::size_t{30000});
            }
            _comparisons = 0U;
            WITH_CONTEXT(requireParts(E{source}.splitAtLast(needle, compareCounted), left, right));
            if (length >= CpCount{64U}) {
                REQUIRE_LESS(_comparisons, std::size_t{30000});
            }
            if (length >= CpCount{64U}) {
                const auto reverseNeedle = S{E{}.append(U'a').append(U'b', length - CpCount::one())};
                const auto reverseSource = S{E{}.append(left).append(reverseNeedle).append(right)};
                _comparisons = 0U;
                WITH_CONTEXT(requireParts(reverseSource.splitAtLast(reverseNeedle, compareCounted), left, right));
                REQUIRE_LESS(_comparisons, std::size_t{30000});
            }
            const auto overlap = S{E{}.append(U'a', length + CpCount::one())};
            const auto repeated = S{E{}.append(U'a', length)};
            WITH_CONTEXT(requireParts(overlap.splitAtLast(repeated), converted<S>("a"_el), S{}));
            REQUIRE(!right.splitAtFirst(needle));
            _comparisons = 0U;
            REQUIRE(!left.splitAtLast(needle, compareCounted));
            if (length >= CpCount{64U}) {
                REQUIRE_LESS(_comparisons, std::size_t{30000});
            }
            const auto mappedNeedle = S{E{}.append(U'x', length)};
            const auto mappedSource = S{E{}.append(U'!').append(U'😀', length).append(U'?')};
            WITH_CONTEXT(requireParts(
                mappedSource.splitAtFirst(mappedNeedle, compareMapped), converted<S>("!"_el), converted<S>("?"_el)));
            WITH_CONTEXT(requireParts(
                mappedSource.splitAtLast(mappedNeedle, compareMapped), converted<S>("!"_el), converted<S>("?"_el)));
            WITH_CONTEXT(requireParts(
                mappedSource.splitAtFirst(mappedNeedle, compareSourceMapped),
                converted<S>("!"_el),
                converted<S>("?"_el)));
            WITH_CONTEXT(requireParts(
                E{mappedSource}.splitAtLast(mappedNeedle, compareSourceMapped),
                converted<S>("!"_el),
                converted<S>("?"_el)));
        }
    }

    template <typename S, typename E>
    void requireMalformed(const S &source, const S &separator, const CpIndex first, const CpIndex last) {
        const auto check = [&]<typename T>(const T &value) -> void {
            for (
                const auto &parts : {value.splitAtFirst(separator), value.splitAtFirst(CharSet{Char::replacement()})}) {
                REQUIRE(parts.has_value());
                REQUIRE_EQUAL(parts->first.storageId(), value.slice(CpRange{CpIndex{}, first}).storageId());
                REQUIRE_EQUAL(
                    parts->second.storageId(),
                    value.slice(CpRange{first.incremented(), CpCount::infinite()}).storageId());
            }
            for (const auto &parts : {value.splitAtLast(separator), value.splitAtLast(CharSet{Char::replacement()})}) {
                REQUIRE(parts.has_value());
                REQUIRE_EQUAL(parts->first.storageId(), value.slice(CpRange{CpIndex{}, last}).storageId());
                REQUIRE_EQUAL(
                    parts->second.storageId(),
                    value.slice(CpRange{last.incremented(), CpCount::infinite()}).storageId());
            }
        };
        WITH_CONTEXT(check(source));
        WITH_CONTEXT(check(E{source}));
        const auto longNeedle = S{E{}.append(U'a', CpCount{64}).append(separator)};
        const auto longSource = S{E{}.append(U'a', CpCount{64}).append(source.slice(CpRange{first, CpCount{1}}))};
        WITH_CONTEXT(requireParts(longSource.splitAtFirst(longNeedle), S{}, S{}));
        WITH_CONTEXT(requireParts(longSource.splitAtLast(longNeedle), S{}, S{}));
    }

private:
    inline static std::size_t _comparisons{};
};
