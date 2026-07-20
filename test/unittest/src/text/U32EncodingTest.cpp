// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/u32/impl/U32Encoding.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>

using el::text::Char;
using el::text::EncodingErrorMode;

TESTED_TARGETS(U32Encoding)
class U32EncodingTest final : public el::UnitTest {
public:
    void testForEachValidatedCharacterPreservesBoundaryValues() {
        const auto input = std::u32string{
            char32_t{0x0U},
            char32_t{0x7FU},
            char32_t{0x80U},
            char32_t{0x7FFU},
            char32_t{0x800U},
            char32_t{0xFFFFU},
            char32_t{0x10000U},
            char32_t{0x10FFFFU}};

        REQUIRE_EQUAL(collectCharacters(input, EncodingErrorMode::Throw), input);
    }

    void testReplaceModeHandlesInvalidCodePoints() {
        const auto input = std::u32string{U'A', char32_t{0xD800U}, char32_t{0xFEFFU}, char32_t{0x110000U}, U'B'};

        REQUIRE_EQUAL(collectCharacters(input, EncodingErrorMode::Replace), std::u32string{U"A\uFFFD\uFFFD\uFFFDB"});
    }

    void testIgnoreModeHandlesInvalidCodePoints() {
        const auto input = std::u32string{U'A', char32_t{0xD800U}, char32_t{0xFEFFU}, char32_t{0x110000U}, U'B'};

        REQUIRE_EQUAL(collectCharacters(input, EncodingErrorMode::Ignore), std::u32string{U"AB"});
    }

    void testThrowModeRejectsInvalidCodePoints() {
        REQUIRE_THROWS(
            el::text::impl::utf32::forEachValidatedCharacter(
                std::u32string{char32_t{0xD800U}}, EncodingErrorMode::Throw, [&](const Char) -> void {}));
        REQUIRE_THROWS(
            el::text::impl::utf32::forEachValidatedCharacter(
                std::u32string{char32_t{0x110000U}}, EncodingErrorMode::Throw, [&](const Char) -> void {}));
        REQUIRE_THROWS(
            el::text::impl::utf32::forEachValidatedCharacter(
                std::u32string{char32_t{0xFEFFU}}, EncodingErrorMode::Throw, [&](const Char) -> void {}));
    }

private:
    [[nodiscard]] static auto collectCharacters(const std::u32string_view text, const EncodingErrorMode errorMode)
        -> std::u32string {
        auto result = std::u32string{};
        el::text::impl::utf32::forEachValidatedCharacter(
            text, errorMode, [&](const Char character) -> void { result.push_back(character.toRawValue()); });
        return result;
    }
};
