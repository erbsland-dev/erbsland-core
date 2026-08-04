// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordHashKey.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"
#include "../text/StringCharReader.hpp"

#include <string_view>
#include <utility>

namespace erbsland::cryptology {

using namespace text::literals;

PasswordHashKey::PasswordHashKey(mem::ByteBlock key) : PasswordHashKey{std::nullopt, std::move(key)} {
}

PasswordHashKey::PasswordHashKey(std::optional<text::String> identifier, mem::ByteBlock key) :
    _identifier{std::move(identifier)}, _key{std::move(key)} {
    _key.markAsSensitive();
    if (_key.length() < unit::ByteLength{32U}) {
        throw err::ParameterError{"A password-hash key must contain at least 32 bytes"_el, "key"_el};
    }
}

auto PasswordHashKey::identified(text::String identifier, mem::ByteBlock key) -> PasswordHashKey {
    if (identifier.isEmpty() || identifier.characterLength() > unit::CpLength{32U}) {
        throw err::ParameterError{
            "A password-hash key identifier must contain 1-32 ASCII characters"_el, "identifier"_el};
    }
    if (!isValidIdentifier(identifier)) {
        throw err::ParameterError{
            "A password-hash key identifier contains an unsupported character"_el, "identifier"_el};
    }
    return PasswordHashKey{std::move(identifier), std::move(key)};
}

auto PasswordHashKey::isValidIdentifier(const text::String &identifier) noexcept -> bool {
    if (identifier.isEmpty() || identifier.characterLength() > unit::CpLength{32U}) {
        return false;
    }
    static const auto validCharacters = []() -> text::CharSet {
        auto result = text::CharSet::from(text::AsciiCategory::Alphanumeric);
        result.add({U'.', U'_', U'-'});
        return result;
    }();
    return identifier.containsOnly(validCharacters);
}

}
