// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordHashKey.hpp"

#include "../err/ParameterError.hpp"
#include "../text/impl/UnsafeU8StringAccess.hpp"

#include <string_view>
#include <utility>

namespace erbsland::cryptology {

PasswordHashKey::PasswordHashKey(mem::ByteBlock key) : PasswordHashKey{std::nullopt, std::move(key)} {
}

PasswordHashKey::PasswordHashKey(std::optional<text::String> identifier, mem::ByteBlock key) :
    _identifier{std::move(identifier)}, _key{std::move(key)} {
    _key.markAsSensitive();
    if (_key.length() < unit::ByteLength{32U}) {
        throw err::ParameterError{"A password-hash key must contain at least 32 bytes", "key"};
    }
}

auto PasswordHashKey::identified(text::String identifier, mem::ByteBlock key) -> PasswordHashKey {
    const auto bytes = text::impl::UnsafeU8StringAccess{identifier}.dataView().dataSpan();
    if (bytes.empty() || bytes.size() > 32U) {
        throw err::ParameterError{"A password-hash key identifier must contain 1-32 ASCII characters", "identifier"};
    }
    for (const auto character : bytes) {
        const auto valid = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') || character == '.' || character == '_' || character == '-';
        if (!valid) {
            throw err::ParameterError{"A password-hash key identifier contains an unsupported character", "identifier"};
        }
    }
    return PasswordHashKey{std::move(identifier), std::move(key)};
}

}
