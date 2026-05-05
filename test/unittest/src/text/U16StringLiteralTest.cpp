// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/StorageIdentifier.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringLiteral.hpp>
#include <erbsland/text/u16/U16StringView.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

using el::mem::StorageIdentifier;
using el::text::StringConverter;
using el::text::U16String;
using el::text::U16StringLiteral;
using el::text::U16StringView;

TESTED_TARGETS(U16StringLiteral operator_el operator_elv operator_els)
class U16StringLiteralTest final : public el::UnitTest {
public:
    void testConstexprUtf16LiteralConstructor() {
        constexpr auto literal = U16StringLiteral{u"Hello"};
        const auto view = U16StringView{literal};
        const auto text = U16String{literal};

        static_assert(std::is_same_v<decltype(literal), const U16StringLiteral>);
        REQUIRE_FALSE(view.isEmpty());
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU16String(), std::u16string{u"Hello"});
        REQUIRE_NOT_EQUAL(text.storageId(), view.storageId());
    }

    void testUtf16LiteralViewReferencesReadOnlyMemoryAndStringCopies() {
        static constexpr char16_t cLiteral[] = u"Literal";
        const auto literalStorageId = storageIdFor(cLiteral, std::size(cLiteral) - 1U);
        constexpr auto literal = U16StringLiteral{cLiteral};
        const auto view = U16StringView{literal};
        const auto text = U16String{literal};

        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Literal"});
        REQUIRE_EQUAL(view.storageId(), literalStorageId);
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Literal"});
        REQUIRE_NOT_EQUAL(text.storageId(), literalStorageId);
    }

    void testUtf16LiteralOperatorCreatesLiteralViewAndString() {
        using namespace el::text::literals;

        constexpr auto literal = u"Hello"_el;
        const auto literalView = U16StringView{literal};
        const auto literalText = U16String{literal};
        const auto view = u"Hello"_elv;
        const auto text = u"Hello"_els;

        static_assert(std::is_same_v<decltype(literal), const U16StringLiteral>);
        static_assert(std::is_same_v<decltype(view), const U16StringView>);
        static_assert(std::is_same_v<decltype(text), const U16String>);
        REQUIRE_EQUAL(StringConverter{literalView}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
        REQUIRE_NOT_EQUAL(literalText.storageId(), literalView.storageId());
        REQUIRE_NOT_EQUAL(text.storageId(), view.storageId());
    }

    void testViewsAreNotConstructibleFromPointersOrStringViews() {
        static_assert(!std::is_constructible_v<U16StringView, const char16_t *>);
        static_assert(!std::is_constructible_v<U16StringView, std::u16string_view>);
        static_assert(std::is_constructible_v<U16String, const char16_t *>);
        static_assert(std::is_constructible_v<U16String, std::u16string_view>);
    }

    void testConstCharPointersCreateStringCopies() {
        auto source = std::array<char16_t, 8U>{u'P', u'o', u'i', u'n', u't', u'e', u'r', u'\0'};
        const char16_t *pointer = source.data();
        const auto text = U16String{pointer};

        source[0] = u'X';

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Pointer"});
        REQUIRE_NOT_EQUAL(text.storageId(), storageIdFor(pointer, 7U));
    }

    void testStringViewInputsCreateStringCopies() {
        auto source = std::u16string{u"View"};
        const auto text = U16String{std::u16string_view{source}};

        source[0] = u'X';

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"View"});
        REQUIRE_NOT_EQUAL(text.storageId(), storageIdFor(source.data(), source.size()));
    }

    void testWideStringsAreConversionOnly() {
        const auto text = StringConverter{std::wstring{L"Hello"}}.toU16String();

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
    }

private:
    template <typename T>
    [[nodiscard]] static auto storageIdFor(const T *data, const std::size_t size) noexcept -> StorageIdentifier {
        return StorageIdentifier::fromMemoryRange(data, data + size);
    }
};
