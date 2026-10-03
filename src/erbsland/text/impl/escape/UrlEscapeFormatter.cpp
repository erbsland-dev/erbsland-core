// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UrlEscapeFormatter.hpp"

#include "../../IntegerFormat.hpp"
#include "../../Literals.hpp"
#include "../../u8/impl/U8Writer.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace erbsland::text::impl {

using namespace text::literals;

auto UrlEscapeFormatter::needsEscape(const Char character, const EscapeAmount amount) const noexcept -> bool {
    return needsEscapeByAmount(character, amount, !_allowedCharacters.contains(character), false);
}

void UrlEscapeFormatter::escape(const Char character, AnyStringBuilder &builder) const {
    static const auto cByteFormat = IntegerFormat{IntegerBase::Hexadecimal}
                                        .setFlags(IntegerFormatFlag::ZeroFill)
                                        .setLetterCase(LetterCase::Uppercase)
                                        .setFieldWidth(unit::CpCount{2U});
    auto bytes = std::array<char8_t, 4>{};
    auto writer = U8Writer{std::span<char8_t>{bytes}};
    writer.write(character);
    for (const auto byte : std::span{bytes}.first(writer.position())) {
        builder.append(U'%').appendInteger(static_cast<uint8_t>(byte), cByteFormat);
    }
}

auto UrlEscapeFormatter::escapeSize(const Char character, [[maybe_unused]] const StringKind stringKind) const noexcept
    -> std::size_t {
    return character.utf8Size().toSizeT() * 3U;
}

auto UrlEscapeFormatter::instance() noexcept -> const EscapeFormatterPtr & {
    static const auto cAllowedCharacters = CharSet::fromPattern("-A-Za-z0-9._~"_el);
    static const EscapeFormatterPtr result = std::make_shared<UrlEscapeFormatter>(cAllowedCharacters);
    return result;
}

auto UrlEscapeFormatter::pathInstance() noexcept -> const EscapeFormatterPtr & {
    static const auto cAllowedCharacters = CharSet::fromPattern("-A-Za-z0-9._~!$&'()*+,;=:@/"_el);
    static const EscapeFormatterPtr result = std::make_shared<UrlEscapeFormatter>(cAllowedCharacters);
    return result;
}

auto UrlEscapeFormatter::queryInstance() noexcept -> const EscapeFormatterPtr & {
    static const auto cAllowedCharacters = CharSet::fromPattern("-A-Za-z0-9._~!$&'()*+,;=:@/?"_el);
    static const EscapeFormatterPtr result = std::make_shared<UrlEscapeFormatter>(cAllowedCharacters);
    return result;
}

auto UrlEscapeFormatter::userInfoInstance() noexcept -> const EscapeFormatterPtr & {
    static const auto cAllowedCharacters = CharSet::fromPattern("-A-Za-z0-9._~!$&'()*+,;="_el);
    static const EscapeFormatterPtr result = std::make_shared<UrlEscapeFormatter>(cAllowedCharacters);
    return result;
}

}
