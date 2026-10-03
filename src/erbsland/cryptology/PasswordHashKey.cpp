// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordHashKey.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/AsciiCategory.hpp"
#include "../text/Literals.hpp"

#include <string_view>
#include <utility>

namespace erbsland::cryptology {

using namespace text;
using namespace text::literals;

PasswordHashKey::PasswordHashKey(mem::ByteBlock key) : PasswordHashKey{std::nullopt, std::move(key)} {
}

PasswordHashKey::PasswordHashKey(std::optional<String> identifier, mem::ByteBlock key) :
    _identifier{std::move(identifier)}, _key{std::move(key)} {
    _key.markAsSensitive();
    if (_key.length() < unit::ByteCount{32U}) {
        throw err::ParameterFailure{"A password-hash key must contain at least 32 bytes"_el, "key"_el};
    }
}

auto PasswordHashKey::identified(String identifier, mem::ByteBlock key) -> PasswordHashKey {
    if (identifier.isEmpty() || identifier.characterLength() > unit::CpCount{32U}) {
        throw err::ParameterFailure{
            "A password-hash key identifier must contain 1-32 ASCII characters"_el, "identifier"_el};
    }
    if (!isValidIdentifier(identifier)) {
        throw err::ParameterFailure{
            "A password-hash key identifier contains an unsupported character"_el, "identifier"_el};
    }
    return PasswordHashKey{std::move(identifier), std::move(key)};
}

auto PasswordHashKey::isValidIdentifier(const String &identifier) noexcept -> bool {
    if (identifier.isEmpty() || identifier.characterLength() > unit::CpCount{32U}) {
        return false;
    }
    return identifier.containsOnly(AsciiCategory::DottedName);
}

}
