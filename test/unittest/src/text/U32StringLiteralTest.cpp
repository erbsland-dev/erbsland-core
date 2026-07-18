// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/StorageIdentifier.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u32/U32StringLiteral.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

using el::mem::StorageIdentifier;
using el::text::StringConverter;
using el::text::U32String;
using el::text::U32StringEditor;
using el::text::U32StringLiteral;

TESTED_TARGETS(U32StringLiteral operator_el operator_el operator_el)
class U32StringLiteralTest final : public el::UnitTest {
public:
    void testConstexprUtf32LiteralConstructor() {
        constexpr auto literal = U32StringLiteral{U"Hello"};
        const auto view = U32String{literal};
        const auto text = U32StringEditor{literal};

        static_assert(std::is_same_v<decltype(literal), const U32StringLiteral>);
        REQUIRE_FALSE(view.isEmpty());
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU32String(), std::u32string{U"Hello"});
        REQUIRE_NOT_EQUAL(text.storageId(), view.storageId());
    }

    void testUtf32LiteralViewReferencesReadOnlyMemoryAndStringCopies() {
        static constexpr char32_t cLiteral[] = U"Literal";
        const auto literalStorageId = storageIdFor(cLiteral, std::size(cLiteral) - 1U);
        constexpr auto literal = U32StringLiteral{cLiteral};
        const auto view = U32String{literal};
        const auto text = U32StringEditor{literal};

        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Literal"});
        REQUIRE_EQUAL(view.storageId(), literalStorageId);
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Literal"});
        REQUIRE_NOT_EQUAL(text.storageId(), literalStorageId);
    }

    void testUtf32LiteralOperatorCreatesLiteralViewAndString() {
        using namespace el::text::literals;

        constexpr auto literal = U"Hello"_el;
        const auto literalView = U32String{literal};
        const auto literalText = U32StringEditor{literal};
        const auto view = U32String{U"Hello"_el};
        const auto text = U32StringEditor{U"Hello"_el};

        static_assert(std::is_same_v<decltype(literal), const U32StringLiteral>);
        static_assert(std::is_same_v<decltype(view), const U32String>);
        static_assert(std::is_same_v<decltype(text), const U32StringEditor>);
        REQUIRE_EQUAL(StringConverter{literalView}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
        REQUIRE_NOT_EQUAL(literalText.storageId(), literalView.storageId());
        REQUIRE_NOT_EQUAL(text.storageId(), view.storageId());
    }

    void testStringsAreConstructibleFromPointersAndStdStringViews() {
        static_assert(std::is_constructible_v<U32String, const char32_t *>);
        static_assert(std::is_constructible_v<U32String, std::u32string_view>);
        static_assert(std::is_constructible_v<U32StringEditor, const char32_t *>);
        static_assert(std::is_constructible_v<U32StringEditor, std::u32string_view>);
    }

    void testConstCharPointersCreateStringCopies() {
        auto source = std::array<char32_t, 8U>{U'P', U'o', U'i', U'n', U't', U'e', U'r', U'\0'};
        const char32_t *pointer = source.data();
        const auto text = U32String{pointer};

        source[0] = U'X';

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Pointer"});
        REQUIRE_NOT_EQUAL(text.storageId(), storageIdFor(pointer, 7U));
    }

    void testStdStringViewInputsCreateStringCopies() {
        auto source = std::u32string{U"View"};
        const auto text = U32String{std::u32string_view{source}};

        source[0] = U'X';

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"View"});
        REQUIRE_NOT_EQUAL(text.storageId(), storageIdFor(source.data(), source.size()));
    }

private:
    template <typename T>
    [[nodiscard]] static auto storageIdFor(const T *data, const std::size_t size) noexcept -> StorageIdentifier {
        return StorageIdentifier::fromMemoryRange(data, data + size);
    }
};
