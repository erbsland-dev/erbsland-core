// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/StorageIdentifier.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/text/u8/U8StringLiteral.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

using namespace el::text::literals;

using el::mem::StorageIdentifier;
using el::text::StringConverter;
using el::text::U8String;
using el::text::U8StringEditor;
using el::text::U8StringLiteral;

TESTED_TARGETS(U8StringLiteral operator_el operator_el operator_el)
class U8StringLiteralTest final : public el::UnitTest {
public:
    void testConstexprNarrowLiteralConstructor() {
        constexpr auto literal = U8StringLiteral{"Hello"};
        const auto view = U8String{literal};
        const auto text = U8StringEditor{literal};

        static_assert(std::is_same_v<decltype(literal), const U8StringLiteral<char>>);
        REQUIRE_FALSE(view.isEmpty());
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
        REQUIRE_NOT_EQUAL(text.storageId(), view.storageId());
    }

    void testConstexprUtf8LiteralConstructor() {
        constexpr auto literal = U8StringLiteral{u8"Hello"};
        const auto view = U8String{literal};
        const auto text = U8StringEditor{literal};

        static_assert(std::is_same_v<decltype(literal), const U8StringLiteral<char8_t>>);
        REQUIRE_FALSE(view.isEmpty());
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
        REQUIRE_NOT_EQUAL(text.storageId(), view.storageId());
    }

    void testNarrowLiteralViewReferencesReadOnlyMemoryAndStringCopies() {
        static constexpr char cLiteral[] = "Literal";
        const auto literalStorageId = storageIdFor(cLiteral, std::size(cLiteral) - 1U);
        constexpr auto literal = U8StringLiteral{cLiteral};
        const auto view = U8String{literal};
        const auto text = U8StringEditor{literal};

        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Literal"});
        REQUIRE_EQUAL(view.storageId(), literalStorageId);
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Literal"});
        REQUIRE_NOT_EQUAL(text.storageId(), literalStorageId);
    }

    void testUtf8LiteralViewReferencesReadOnlyMemoryAndStringCopies() {
        static constexpr char8_t cLiteral[] = u8"Literal";
        const auto literalStorageId = storageIdFor(cLiteral, std::size(cLiteral) - 1U);
        constexpr auto literal = U8StringLiteral{cLiteral};
        const auto view = U8String{literal};
        const auto text = U8StringEditor{literal};

        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Literal"});
        REQUIRE_EQUAL(view.storageId(), literalStorageId);
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Literal"});
        REQUIRE_NOT_EQUAL(text.storageId(), literalStorageId);
    }

    void testStaticConstexprLiterals() {
        const auto narrowView = U8String{cStaticNarrowLiteral};
        const auto utf8View = U8String{cStaticUtf8Literal};

        REQUIRE_EQUAL(StringConverter{narrowView}.toStdString(), std::string{"Static"});
        REQUIRE_EQUAL(StringConverter{utf8View}.toStdString(), std::string{"Statisch"});
    }

    void testNarrowLiteralOperatorCreatesLiteral() {

        constexpr auto literal = "Hello"_el;
        const auto view = U8String{literal};
        const auto text = U8StringEditor{literal};

        static_assert(std::is_same_v<decltype(literal), const U8StringLiteral<char>>);
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
        REQUIRE_NOT_EQUAL(text.storageId(), view.storageId());
    }

    void testUtf8LiteralOperatorCreatesLiteral() {

        constexpr auto literal = u8"Hello"_el;
        const auto view = U8String{literal};
        const auto text = U8StringEditor{literal};

        static_assert(std::is_same_v<decltype(literal), const U8StringLiteral<char8_t>>);
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
        REQUIRE_NOT_EQUAL(text.storageId(), view.storageId());
    }

    void testNarrowLiteralCreatesReadOnlyStringExplicitly() {

        const auto view = U8String{"Hello"_el};

        static_assert(std::is_same_v<decltype(view), const U8String>);
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
    }

    void testUtf8LiteralCreatesReadOnlyStringExplicitly() {

        const auto view = U8String{u8"Hello"_el};

        static_assert(std::is_same_v<decltype(view), const U8String>);
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
    }

    void testNarrowLiteralCreatesEditorExplicitly() {

        const auto text = U8StringEditor{"Hello"_el};
        const auto view = U8String{"Hello"_el};

        static_assert(std::is_same_v<decltype(text), const U8StringEditor>);
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
        REQUIRE_NOT_EQUAL(text.storageId(), view.storageId());
    }

    void testUtf8LiteralCreatesEditorExplicitly() {

        const auto text = U8StringEditor{u8"Hello"_el};
        const auto view = U8String{u8"Hello"_el};

        static_assert(std::is_same_v<decltype(text), const U8StringEditor>);
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
        REQUIRE_NOT_EQUAL(text.storageId(), view.storageId());
    }

    void testStringsAreConstructibleFromPointersAndStdStringViews() {
        static_assert(std::is_constructible_v<U8String, const char *>);
        static_assert(std::is_constructible_v<U8String, const char8_t *>);
        static_assert(std::is_constructible_v<U8String, std::string_view>);
        static_assert(std::is_constructible_v<U8String, std::u8string_view>);
        static_assert(std::is_constructible_v<U8StringEditor, const char *>);
        static_assert(std::is_constructible_v<U8StringEditor, const char8_t *>);
        static_assert(std::is_constructible_v<U8StringEditor, std::string_view>);
        static_assert(std::is_constructible_v<U8StringEditor, std::u8string_view>);
    }

    void testConstCharPointersCreateStringCopies() {
        auto narrowSource = std::array<char, 8U>{'P', 'o', 'i', 'n', 't', 'e', 'r', '\0'};
        auto utf8Source = std::array<char8_t, 8U>{u8'P', u8'o', u8'i', u8'n', u8't', u8'e', u8'r', u8'\0'};
        const char *narrowPointer = narrowSource.data();
        const char8_t *utf8Pointer = utf8Source.data();
        const auto narrowText = U8String{narrowPointer};
        const auto utf8Text = U8String{utf8Pointer};

        narrowSource[0] = 'X';
        utf8Source[0] = u8'X';

        REQUIRE_EQUAL(StringConverter{narrowText}.toStdString(), std::string{"Pointer"});
        REQUIRE_EQUAL(StringConverter{utf8Text}.toStdString(), std::string{"Pointer"});
        REQUIRE_NOT_EQUAL(narrowText.storageId(), storageIdFor(narrowPointer, 7U));
        REQUIRE_NOT_EQUAL(utf8Text.storageId(), storageIdFor(utf8Pointer, 7U));
    }

    void testStdStringViewInputsCreateStringCopies() {
        auto narrowSource = std::string{"View"};
        auto utf8Source = std::u8string{u8"View"};
        const auto narrowText = U8String{std::string_view{narrowSource}};
        const auto utf8Text = U8String{std::u8string_view{utf8Source}};

        narrowSource[0] = 'X';
        utf8Source[0] = u8'X';

        REQUIRE_EQUAL(StringConverter{narrowText}.toStdString(), std::string{"View"});
        REQUIRE_EQUAL(StringConverter{utf8Text}.toStdString(), std::string{"View"});
        REQUIRE_NOT_EQUAL(narrowText.storageId(), storageIdFor(narrowSource.data(), narrowSource.size()));
        REQUIRE_NOT_EQUAL(utf8Text.storageId(), storageIdFor(utf8Source.data(), utf8Source.size()));
    }

private:
    template <typename T>
    [[nodiscard]] static auto storageIdFor(const T *data, const std::size_t size) noexcept -> StorageIdentifier {
        return StorageIdentifier::fromMemoryRange(data, data + size);
    }

    static constexpr auto cStaticNarrowLiteral = U8StringLiteral{"Static"};
    static constexpr auto cStaticUtf8Literal = U8StringLiteral{u8"Statisch"};
};
