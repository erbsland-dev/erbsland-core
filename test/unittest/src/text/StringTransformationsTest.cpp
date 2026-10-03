// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/AnyString.hpp>
#include <erbsland/AnyStringEditor.hpp>
#include <erbsland/AsciiCategory.hpp>
#include <erbsland/Capitalization.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringTransformations.hpp>
#include <erbsland/TransformCharacterFn.hpp>
#include <erbsland/U16String.hpp>
#include <erbsland/U16StringEditor.hpp>
#include <erbsland/U32String.hpp>
#include <erbsland/U32StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

TESTED_TARGETS(StringTransformations Capitalization)
class StringTransformationsTest final : public el::UnitTest {
private:
    struct NonString {
        auto transformed(TransformCharacterFn) const -> String;
    };

public:
    void testAllWidthsAndInputKinds() {
        WITH_CONTEXT((requireTransformations<String, String>()));
        WITH_CONTEXT((requireTransformations<StringEditor, String>()));
        WITH_CONTEXT((requireTransformations<U16String, U16String>()));
        WITH_CONTEXT((requireTransformations<U16StringEditor, U16String>()));
        WITH_CONTEXT((requireTransformations<U32String, U32String>()));
        WITH_CONTEXT((requireTransformations<U32StringEditor, U32String>()));
    }

    void testRejectedTypes() {
        requireRejected<int>();
        requireRejected<Char>();
        requireRejected<CharSet>();
        requireRejected<std::string>();
        requireRejected<std::string_view>();
        requireRejected<const char *>();
        requireRejected<decltype("literal"_el)>();
        requireRejected<decltype(u"literal"_el)>();
        requireRejected<decltype(U"literal"_el)>();
        requireRejected<AnyString>();
        requireRejected<AnyStringEditor>();
        requireRejected<NonString>();
    }

    void testStorageReuse() {
        WITH_CONTEXT((requireStorage<String, StringEditor>(String{u8"--Hello 😀--"_el})));
        WITH_CONTEXT((requireStorage<U16String, U16StringEditor>(U16String{u"--Hello 😀--"_el})));
        WITH_CONTEXT((requireStorage<U32String, U32StringEditor>(U32String{U"--Hello 😀--"_el})));
    }

    void testSensitiveResults() {
        auto source = String{u8"  hELLO_äpfelHTTP  "_el};
        source.markAsSensitive();
        WITH_CONTEXT(requireSensitive(source));
        WITH_CONTEXT(requireSensitive(StringEditor{source}));
        auto whitespace = String{" \t "_el};
        whitespace.markAsSensitive();
        REQUIRE(toWhitespaceSimplified(whitespace).isEmpty());
        REQUIRE(toWhitespaceSimplified(whitespace).isSensitive());
        REQUIRE(toAsciiWhitespaceSimplified(whitespace).isSensitive());
        REQUIRE(toCamelCaseFromWords(whitespace).isSensitive());
        REQUIRE(toAsciiCamelCaseFromWords(whitespace).isSensitive());
    }

private:
    template <typename T>
    [[nodiscard]] static auto make(const String &value) -> T {
        if constexpr (std::same_as<T, String> || std::same_as<T, StringEditor>) {
            return T{value};
        } else if constexpr (std::same_as<T, U16String> || std::same_as<T, U16StringEditor>) {
            return T{StringConverter{value}.toU16String()};
        } else {
            return T{StringConverter{value}.toU32String()};
        }
    }

    template <typename T>
    void requireText(const T &actual, const String &expected) {
        REQUIRE_EQUAL(StringConverter{actual}.toString(), expected);
    }

    template <typename T, typename R>
    void requireTransformations() {
        requireReturnTypes<T, R>();
        const auto source = make<T>(u8"hELLO ÄÖΣ𐐀"_el);
        const auto original = StringConverter{source}.toString();
        WITH_CONTEXT(requireText(toAsciiLowercase(source), u8"hello ÄÖΣ𐐀"_el));
        WITH_CONTEXT(requireText(toAsciiUppercase(source), u8"HELLO ÄÖΣ𐐀"_el));
        WITH_CONTEXT(requireText(toAsciiCapitalized(source), u8"Hello ÄÖΣ𐐀"_el));
        WITH_CONTEXT(requireText(toLowercase(source), u8"hello äöσ𐐨"_el));
        WITH_CONTEXT(requireText(toUppercase(source), u8"HELLO ÄÖΣ𐐀"_el));
        WITH_CONTEXT(requireText(toCapitalized(source), u8"Hello äöσ𐐨"_el));
        WITH_CONTEXT(requireText(toCaseFolded(make<T>(u8"Σςß"_el)), u8"σσß"_el));
        WITH_CONTEXT(requireText(toIdentifierNormalized(make<T>(u8" A B Ä\t"_el)), u8"_a_b_Ä\t"_el));
        WITH_CONTEXT(requireText(toCapitalized(make<T>("hELLO wORLD"_el)), "Hello world"_el));
        WITH_CONTEXT(requireText(toCapitalized(make<T>("!HELLO"_el)), "!hello"_el));
        WITH_CONTEXT(requireText(toCapitalized(make<T>("a"_el)), "A"_el));
        for (
            const auto capitalization :
            {Capitalization::Lowercase, Capitalization::Uppercase, Capitalization::Capitalized}) {
            if (capitalization == Capitalization::Lowercase) {
                REQUIRE_EQUAL(toCapitalization(source, capitalization), toLowercase(source));
                REQUIRE_EQUAL(toAsciiCapitalization(source, capitalization), toAsciiLowercase(source));
            } else if (capitalization == Capitalization::Uppercase) {
                REQUIRE_EQUAL(toCapitalization(source, capitalization), toUppercase(source));
                REQUIRE_EQUAL(toAsciiCapitalization(source, capitalization), toAsciiUppercase(source));
            } else {
                REQUIRE_EQUAL(toCapitalization(source, capitalization), toCapitalized(source));
                REQUIRE_EQUAL(toAsciiCapitalization(source, capitalization), toAsciiCapitalized(source));
            }
        }
        WITH_CONTEXT(requireJoining<T>());
        WITH_CONTEXT(requireSplitting<T>());
        WITH_CONTEXT(requireWhitespace<T>());
        WITH_CONTEXT(requireEmpty<T>());
        WITH_CONTEXT(requireText(source, original));
    }

    template <typename T>
    void requireJoining() {
        const auto separators = CharSet{U'_', U'-'};
        const auto source = make<T>("__foo_HTTP--bar_"_el);
        WITH_CONTEXT(requireText(toCamelCaseFromWords(source, separators), "fooHTTPBar"_el));
        WITH_CONTEXT(requireText(toAsciiCamelCaseFromWords(source, separators), "fooHTTPBar"_el));
        for (
            const auto &[capitalization, expected] :
            {std::pair{Capitalization::Capitalized, String{"FooHttpBar"_el}},
                std::pair{Capitalization::Lowercase, String{"foohttpbar"_el}},
                std::pair{Capitalization::Uppercase, String{"FOOHTTPBAR"_el}}}) {
            WITH_CONTEXT(requireText(toCamelCaseFromWords(source, separators, capitalization), expected));
            WITH_CONTEXT(requireText(toAsciiCamelCaseFromWords(source, separators, capitalization), expected));
        }
        WITH_CONTEXT(requireText(toCamelCaseFromWords(make<T>("Word_word"_el), separators), "WordWord"_el));
        WITH_CONTEXT(requireText(toCamelCaseFromWords(make<T>("foo_!bar_2baz"_el), separators), "foo!bar2baz"_el));
        WITH_CONTEXT(requireText(toCamelCaseFromWords(make<T>(u8"foo_äpfel_𐐨"_el), separators), u8"fooÄpfel𐐀"_el));
        WITH_CONTEXT(requireText(toAsciiCamelCaseFromWords(make<T>(u8"foo_äpfel_𐐨"_el), separators), u8"fooäpfel𐐨"_el));
        const auto spaced = make<T>(" foo\tHTTP\r\nbar "_el);
        const auto asciiWhitespace = CharSet::from(AsciiCategory::Whitespace);
        REQUIRE_EQUAL(toAsciiCamelCaseFromWords(spaced), toAsciiCamelCaseFromWords(spaced, asciiWhitespace));
        REQUIRE_EQUAL(
            toAsciiCamelCaseFromWords(spaced, Capitalization::Capitalized),
            toAsciiCamelCaseFromWords(spaced, asciiWhitespace, Capitalization::Capitalized));
        WITH_CONTEXT(requireText(toCamelCaseFromWords(spaced), "fooHTTPBar"_el));
        WITH_CONTEXT(requireText(toCamelCaseFromWords(spaced, Capitalization::Capitalized), "FooHttpBar"_el));
        const auto unicodeSpaced = make<T>(u8" foo barbaz　qux "_el);
        const auto unicodeWhitespace = CharSet{String{u8"  　"_el}};
        REQUIRE_EQUAL(toCamelCaseFromWords(unicodeSpaced), toCamelCaseFromWords(unicodeSpaced, unicodeWhitespace));
        WITH_CONTEXT(requireText(toCamelCaseFromWords(unicodeSpaced), "fooBarBazQux"_el));
        WITH_CONTEXT(requireText(toAsciiCamelCaseFromWords(unicodeSpaced), u8"foo barbaz　qux"_el));
        WITH_CONTEXT(requireText(toCamelCaseFromWords(make<T>("foo_bar-baz"_el)), "foo_bar-baz"_el));
        WITH_CONTEXT(requireText(toCamelCaseFromWords(make<T>("___"_el), separators), ""_el));
        REQUIRE_EQUAL(toCamelCaseFromWords(source, CharSet{}).storageId(), source.storageId());
    }

    template <typename T>
    void requireSplitting() {
        for (
            const auto &[input, expected] :
            {std::pair{String{""_el}, String{""_el}},
                std::pair{String{"already_lower"_el}, String{"already_lower"_el}},
                std::pair{String{"A"_el}, String{"a"_el}},
                std::pair{String{"HTTP"_el}, String{"http"_el}},
                std::pair{String{"HTTPServer"_el}, String{"http_server"_el}},
                std::pair{String{"myHTTPServer"_el}, String{"my_http_server"_el}},
                std::pair{String{"URL2Value"_el}, String{"url2_value"_el}},
                std::pair{String{"version2"_el}, String{"version2"_el}},
                std::pair{String{"ABc"_el}, String{"a_bc"_el}},
                std::pair{String{"TwoWords"_el}, String{"two_words"_el}}}) {
            WITH_CONTEXT(requireText(toWordsFromCamelCase(make<T>(input), U'_', Capitalization::Lowercase), expected));
            WITH_CONTEXT(
                requireText(toAsciiWordsFromCamelCase(make<T>(input), U'_', Capitalization::Lowercase), expected));
        }
        const auto source = make<T>("myHTTPServer"_el);
        WITH_CONTEXT(requireText(toWordsFromCamelCase(source), "my HTTP Server"_el));
        WITH_CONTEXT(requireText(toAsciiWordsFromCamelCase(source), "my HTTP Server"_el));
        REQUIRE_EQUAL(toWordsFromCamelCase(source), toWordsFromCamelCase(source, U' '));
        REQUIRE_EQUAL(toAsciiWordsFromCamelCase(source), toAsciiWordsFromCamelCase(source, U' '));
        WITH_CONTEXT(requireText(toWordsFromCamelCase(source, Capitalization::Capitalized), "My Http Server"_el));
        WITH_CONTEXT(requireText(toAsciiWordsFromCamelCase(source, Capitalization::Uppercase), "MY HTTP SERVER"_el));
        WITH_CONTEXT(requireText(toWordsFromCamelCase(source, U'-', Capitalization::Lowercase), "my-http-server"_el));
        WITH_CONTEXT(requireText(toWordsFromCamelCase(source, U'😀'), u8"my😀HTTP😀Server"_el));
        WITH_CONTEXT(
            requireText(toWordsFromCamelCase(make<T>(u8"éValue"_el), U'_', Capitalization::Lowercase), u8"é_value"_el));
        WITH_CONTEXT(requireText(
            toAsciiWordsFromCamelCase(make<T>(u8"éValue"_el), U'_', Capitalization::Lowercase), u8"évalue"_el));
        WITH_CONTEXT(requireText(toWordsFromCamelCase(make<T>(u8"Äpfel"_el), Capitalization::Lowercase), u8"äpfel"_el));
        WITH_CONTEXT(
            requireText(toAsciiWordsFromCamelCase(make<T>(u8"Äpfel"_el), Capitalization::Lowercase), u8"Äpfel"_el));
        WITH_CONTEXT(requireText(
            toWordsFromCamelCase(make<T>(u8"GrüßeHTTPServer"_el), U'_', Capitalization::Lowercase),
            u8"grüße_http_server"_el));
        WITH_CONTEXT(requireText(toWordsFromCamelCase(make<T>(u8"wordǅelta"_el)), u8"word ǅelta"_el));
        WITH_CONTEXT(requireText(toWordsFromCamelCase(make<T>(u8"x٢Value"_el)), u8"x٢ Value"_el));
        WITH_CONTEXT(requireText(toAsciiWordsFromCamelCase(make<T>(u8"x٢Value"_el)), u8"x٢Value"_el));
        WITH_CONTEXT(requireText(toWordsFromCamelCase(make<T>(u8"𐐨Value"_el)), u8"𐐨 Value"_el));
        WITH_CONTEXT(requireText(toWordsFromCamelCase(make<T>(u8"x😀Value"_el)), u8"x😀Value"_el));
        WITH_CONTEXT(requireText(toWordsFromCamelCase(make<T>(u8"éValue"_el)), u8"éValue"_el));
    }

    template <typename T>
    void requireWhitespace() {
        WITH_CONTEXT(requireText(toWhitespaceSimplified(make<T>(" \tfoo\r\nbar\v\fbaz "_el)), "foo bar baz"_el));
        WITH_CONTEXT(requireText(toAsciiWhitespaceSimplified(make<T>(" \tfoo\r\nbar\v\fbaz "_el)), "foo bar baz"_el));
        WITH_CONTEXT(requireText(toWhitespaceSimplified(make<T>(u8" foo　bar baz  "_el)), "foo bar baz"_el));
        WITH_CONTEXT(requireText(
            toAsciiWhitespaceSimplified(make<T>(u8" foo　bar baz  "_el)), u8" foo　bar baz "_el));
        WITH_CONTEXT(requireText(toWhitespaceSimplified(make<T>("  foo  bar  "_el), U'_'), "foo_bar"_el));
        WITH_CONTEXT(requireText(toAsciiWhitespaceSimplified(make<T>("  foo  bar  "_el), U'😀'), u8"foo😀bar"_el));
        WITH_CONTEXT(requireText(toWhitespaceSimplified(make<T>(u8"a​b﻿c"_el)), u8"a​b﻿c"_el));
        WITH_CONTEXT(requireText(toWhitespaceSimplified(make<T>(u8"  "_el)), ""_el));
        WITH_CONTEXT(requireText(toAsciiWhitespaceSimplified(make<T>(" \t\r\n"_el)), ""_el));
        const auto simplified = make<T>("foo bar"_el);
        REQUIRE_EQUAL(toWhitespaceSimplified(simplified).storageId(), simplified.storageId());
        REQUIRE_EQUAL(toAsciiWhitespaceSimplified(simplified).storageId(), simplified.storageId());
    }

    template <typename T>
    void requireEmpty() {
        const auto source = T{};
        REQUIRE(toAsciiLowercase(source).isEmpty());
        REQUIRE(toAsciiUppercase(source).isEmpty());
        REQUIRE(toAsciiCapitalized(source).isEmpty());
        REQUIRE(toAsciiCapitalization(source, Capitalization::Capitalized).isEmpty());
        REQUIRE(toLowercase(source).isEmpty());
        REQUIRE(toUppercase(source).isEmpty());
        REQUIRE(toCapitalized(source).isEmpty());
        REQUIRE(toCapitalization(source, Capitalization::Capitalized).isEmpty());
        REQUIRE(toCaseFolded(source).isEmpty());
        REQUIRE(toIdentifierNormalized(source).isEmpty());
        REQUIRE(toCamelCaseFromWords(source).isEmpty());
        REQUIRE(toCamelCaseFromWords(source, Capitalization::Capitalized).isEmpty());
        REQUIRE(toCamelCaseFromWords(source, CharSet{}).isEmpty());
        REQUIRE(toWordsFromCamelCase(source).isEmpty());
        REQUIRE(toWordsFromCamelCase(source, Capitalization::Capitalized).isEmpty());
        REQUIRE(toWordsFromCamelCase(source, U'_').isEmpty());
        REQUIRE(toWhitespaceSimplified(source).isEmpty());
        REQUIRE(toAsciiCamelCaseFromWords(source).isEmpty());
        REQUIRE(toAsciiCamelCaseFromWords(source, Capitalization::Capitalized).isEmpty());
        REQUIRE(toAsciiCamelCaseFromWords(source, CharSet{}).isEmpty());
        REQUIRE(toAsciiWordsFromCamelCase(source).isEmpty());
        REQUIRE(toAsciiWordsFromCamelCase(source, Capitalization::Capitalized).isEmpty());
        REQUIRE(toAsciiWordsFromCamelCase(source, U'_').isEmpty());
        REQUIRE(toAsciiWhitespaceSimplified(source).isEmpty());
    }

    template <typename R, typename E>
    void requireStorage(const R &literal) {
        const auto shared = literal.copy();
        for (const auto &source : {literal, shared}) {
            const auto slice = source.slice(CpRange{CpIndex{2}, CpIndex{9}});
            REQUIRE_EQUAL(toCapitalized(slice).storageId(), slice.storageId());
            REQUIRE_EQUAL(toAsciiCapitalized(slice).storageId(), slice.storageId());
            REQUIRE_EQUAL(toWhitespaceSimplified(slice).storageId(), slice.storageId());
            REQUIRE_EQUAL(toAsciiWhitespaceSimplified(slice).storageId(), slice.storageId());
            REQUIRE_EQUAL(toCamelCaseFromWords(slice, CharSet{U'_'}).storageId(), slice.storageId());
            REQUIRE_EQUAL(toAsciiCamelCaseFromWords(slice, CharSet{U'_'}).storageId(), slice.storageId());
            REQUIRE_EQUAL(toWordsFromCamelCase(slice).storageId(), slice.storageId());
            REQUIRE_EQUAL(toAsciiWordsFromCamelCase(slice).storageId(), slice.storageId());
            auto editor = E{slice};
            const auto result = toCapitalized(editor);
            REQUIRE_EQUAL(result.storageId(), editor.storageId());
            editor.append(U'!');
            REQUIRE_EQUAL(result, slice);
            REQUIRE_NOT_EQUAL(result, R{editor});
        }
        const auto lower = make<R>("lower"_el);
        const auto upper = make<R>("UPPER"_el);
        REQUIRE_EQUAL(toLowercase(lower).storageId(), lower.storageId());
        REQUIRE_EQUAL(toAsciiLowercase(lower).storageId(), lower.storageId());
        REQUIRE_EQUAL(toCaseFolded(lower).storageId(), lower.storageId());
        REQUIRE_EQUAL(toIdentifierNormalized(lower).storageId(), lower.storageId());
        REQUIRE_EQUAL(toUppercase(upper).storageId(), upper.storageId());
        REQUIRE_EQUAL(toAsciiUppercase(upper).storageId(), upper.storageId());
    }

    template <typename T>
    void requireSensitive(const T &value) {
        REQUIRE(toAsciiLowercase(value).isSensitive());
        REQUIRE(toAsciiUppercase(value).isSensitive());
        REQUIRE(toAsciiCapitalized(value).isSensitive());
        REQUIRE(toAsciiCapitalization(value, Capitalization::Capitalized).isSensitive());
        REQUIRE(toLowercase(value).isSensitive());
        REQUIRE(toUppercase(value).isSensitive());
        REQUIRE(toCapitalized(value).isSensitive());
        REQUIRE(toCapitalization(value, Capitalization::Capitalized).isSensitive());
        REQUIRE(toCaseFolded(value).isSensitive());
        REQUIRE(toIdentifierNormalized(value).isSensitive());
        REQUIRE(toCamelCaseFromWords(value).isSensitive());
        REQUIRE(toCamelCaseFromWords(value, Capitalization::Capitalized).isSensitive());
        REQUIRE(toCamelCaseFromWords(value, CharSet{}).isSensitive());
        REQUIRE(toWordsFromCamelCase(value).isSensitive());
        REQUIRE(toWordsFromCamelCase(value, Capitalization::Capitalized).isSensitive());
        REQUIRE(toWordsFromCamelCase(value, U'_').isSensitive());
        REQUIRE(toWhitespaceSimplified(value).isSensitive());
        REQUIRE(toAsciiCamelCaseFromWords(value).isSensitive());
        REQUIRE(toAsciiCamelCaseFromWords(value, Capitalization::Capitalized).isSensitive());
        REQUIRE(toAsciiCamelCaseFromWords(value, CharSet{}).isSensitive());
        REQUIRE(toAsciiWordsFromCamelCase(value).isSensitive());
        REQUIRE(toAsciiWordsFromCamelCase(value, Capitalization::Capitalized).isSensitive());
        REQUIRE(toAsciiWordsFromCamelCase(value, U'_').isSensitive());
        REQUIRE(toAsciiWhitespaceSimplified(value).isSensitive());
        const auto unchanged = toAsciiLowercase(value);
        REQUIRE_EQUAL(toAsciiLowercase(unchanged).storageId(), unchanged.storageId());
    }

    template <typename T>
    static void requireRejected() {
        static_assert(!requires(const T &value) { toAsciiLowercase(value); });
        static_assert(!requires(const T &value) { toAsciiUppercase(value); });
        static_assert(!requires(const T &value) { toAsciiCapitalized(value); });
        static_assert(!requires(const T &value) { toAsciiCapitalization(value, Capitalization::Capitalized); });
        static_assert(!requires(const T &value) { toLowercase(value); });
        static_assert(!requires(const T &value) { toUppercase(value); });
        static_assert(!requires(const T &value) { toCapitalized(value); });
        static_assert(!requires(const T &value) { toCapitalization(value, Capitalization::Capitalized); });
        static_assert(!requires(const T &value) { toCaseFolded(value); });
        static_assert(!requires(const T &value) { toIdentifierNormalized(value); });
        static_assert(!requires(const T &value) { toCamelCaseFromWords(value); });
        static_assert(!requires(const T &value) { toCamelCaseFromWords(value, Capitalization::Capitalized); });
        static_assert(!requires(const T &value) { toCamelCaseFromWords(value, CharSet{}); });
        static_assert(!requires(const T &value) { toWordsFromCamelCase(value); });
        static_assert(!requires(const T &value) { toWordsFromCamelCase(value, Capitalization::Capitalized); });
        static_assert(!requires(const T &value) { toWordsFromCamelCase(value, U'_'); });
        static_assert(!requires(const T &value) { toWhitespaceSimplified(value); });
        static_assert(!requires(const T &value) { toAsciiCamelCaseFromWords(value); });
        static_assert(!requires(const T &value) { toAsciiCamelCaseFromWords(value, Capitalization::Capitalized); });
        static_assert(!requires(const T &value) { toAsciiCamelCaseFromWords(value, CharSet{}); });
        static_assert(!requires(const T &value) { toAsciiWordsFromCamelCase(value); });
        static_assert(!requires(const T &value) { toAsciiWordsFromCamelCase(value, Capitalization::Capitalized); });
        static_assert(!requires(const T &value) { toAsciiWordsFromCamelCase(value, U'_'); });
        static_assert(!requires(const T &value) { toAsciiWhitespaceSimplified(value); });
    }

    template <typename T, typename R>
    static void requireReturnTypes() {
        static_assert(requires(const T &value) {
            { toAsciiLowercase(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toAsciiUppercase(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toAsciiCapitalized(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toAsciiCapitalization(value, Capitalization::Capitalized) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toLowercase(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toUppercase(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toCapitalized(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toCapitalization(value, Capitalization::Capitalized) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toCaseFolded(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toIdentifierNormalized(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toCamelCaseFromWords(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toCamelCaseFromWords(value, Capitalization::Capitalized) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toCamelCaseFromWords(value, CharSet{}) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toWordsFromCamelCase(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toWordsFromCamelCase(value, Capitalization::Capitalized) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toWordsFromCamelCase(value, U'_') } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toWhitespaceSimplified(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toAsciiCamelCaseFromWords(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toAsciiCamelCaseFromWords(value, Capitalization::Capitalized) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toAsciiCamelCaseFromWords(value, CharSet{}) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toAsciiWordsFromCamelCase(value) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toAsciiWordsFromCamelCase(value, Capitalization::Capitalized) } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toAsciiWordsFromCamelCase(value, U'_') } -> std::same_as<R>;
        });
        static_assert(requires(const T &value) {
            { toAsciiWhitespaceSimplified(value) } -> std::same_as<R>;
        });
    }
};
