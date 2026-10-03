// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/AnyString.hpp>
#include <erbsland/AnyStringEditor.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/U16StringEditor.hpp>
#include <erbsland/U32StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(StringJoins StringJoinTools)
class StringJoinsTest final : public el::UnitTest {
    template <typename... T>
    static constexpr bool cCanJoin = requires(const T &...parts) { el::join(parts...); };
    template <typename... T>
    static constexpr bool cCanJoinList = requires(const T &...parts) { el::join({parts...}); };
    static_assert(cCanJoin<el::String, el::StringEditor, el::StringLiteral>);
    static_assert(cCanJoinList<el::String, el::StringEditor, el::StringLiteral>);
    static_assert(cCanJoin<decltype("a"_el), decltype(u8"b"_el)>);
    static_assert(!cCanJoin<> && !cCanJoinList<>);
    static_assert(!cCanJoin<el::String, el::U16String>);
    static_assert(!cCanJoinList<el::String, el::U16String>);
    static_assert(!cCanJoin<el::U16String, el::U32String>);
    static_assert(!cCanJoin<el::StringEditor, decltype(U"b"_el)>);
    static_assert(!cCanJoin<int> && !cCanJoin<el::Char>);
    static_assert(!cCanJoin<const char *> && !cCanJoin<std::string>);
    static_assert(!cCanJoinList<const char *> && !cCanJoinList<std::string>);
    static_assert(!cCanJoin<el::AnyString> && !cCanJoin<el::AnyStringEditor>);
    static_assert(!cCanJoin<std::vector<el::String>>);
    static_assert(std::same_as<decltype(el::join(u"a"_el, u"b"_el)), el::U16String>);
    static_assert(std::same_as<decltype(el::join({U"a"_el, U"b"_el})), el::U32String>);

public:
    void testAllWidthsAndInputs() {
        WITH_CONTEXT((check<el::String, el::StringEditor>("a😀"_el, "b"_el)));
        WITH_CONTEXT((check<el::U16String, el::U16StringEditor>(u"a😀"_el, u"b"_el)));
        WITH_CONTEXT((check<el::U32String, el::U32StringEditor>(U"a😀"_el, U"b"_el)));
        REQUIRE_EQUAL(el::join("a"_el, u8"b"_el), "ab"_el);
        REQUIRE_EQUAL(el::join({"a"_el, u8"b"_el}), "ab"_el);
    }
    void testNullsAndSlices() {
        WITH_CONTEXT((checkSlices<el::String>("a\0b"_el)));
        WITH_CONTEXT((checkSlices<el::U16String>(u"a\0b"_el)));
        WITH_CONTEXT((checkSlices<el::U32String>(U"a\0b"_el)));
    }

private:
    template <typename tString, typename tEditor, typename tLiteral>
    void check(tLiteral first, tLiteral last) {
        auto editor = tEditor{last};
        const auto source = tString{first};
        const auto expected = tString::fromJoined({first, last, first});
        REQUIRE_EQUAL(el::join(source, editor, first), expected);
        REQUIRE_EQUAL(el::join({source, editor, first}), expected);
        REQUIRE_EQUAL(editor, tEditor{last});
        REQUIRE_EQUAL(el::join(tString{first}, tEditor{last}, first), expected);
        REQUIRE_EQUAL(el::join(tString{}, source, tString{}), source);
        REQUIRE_EQUAL(el::join(source).storageId(), source.storageId());
        REQUIRE_EQUAL(el::join({source}).storageId(), source.storageId());
        REQUIRE_EQUAL(el::join(first), source);
        if constexpr (std::same_as<tString, el::String>) {
            auto sensitive = source;
            sensitive.markAsSensitive();
            REQUIRE(el::join(tString{}, sensitive).isSensitive());
            REQUIRE(el::join({sensitive, source}).isSensitive());
            REQUIRE(el::join(tEditor{sensitive}).isSensitive());
            auto empty = tString{};
            empty.markAsSensitive();
            REQUIRE(el::join(empty, tString{}).isSensitive());
        }
    }
    template <typename tString, typename tLiteral>
    void checkSlices(tLiteral literal) {
        const auto source = tString{literal};
        const auto split = source.splitAtFirst(el::CharSet{U'\0'});
        REQUIRE(split.has_value());
        REQUIRE_EQUAL(el::join(split->first, split->second), tString::fromJoined({split->first, split->second}));
        REQUIRE_EQUAL(el::join(source, source).length(), source.length() + source.length());
        REQUIRE_EQUAL(el::join({source, source}), tString::fromJoined({source, source}));
    }
};
