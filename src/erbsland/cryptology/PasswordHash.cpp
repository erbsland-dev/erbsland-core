// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordHash.hpp"

#include "impl/PasswordHashData.hpp"

#include "../err/LogicFailure.hpp"
#include "../err/ParseError.hpp"

#include <utility>

namespace erbsland::cryptology {

using namespace text;

PasswordHash::PasswordHash(impl::PasswordHashDataPtr data) noexcept : _data{std::move(data)} {
}

auto PasswordHash::fromString(const String &text) noexcept -> PasswordHash {
    try {
        return PasswordHash{impl::PasswordHashData::fromStringOrThrow(text)};
    } catch (const err::ParseError &) {
        return {};
    }
}

auto PasswordHash::fromStringOrThrow(const String &text) -> PasswordHash {
    return PasswordHash{impl::PasswordHashData::fromStringOrThrow(text)};
}

auto PasswordHash::toString() const -> String {
    return _data == nullptr ? String{} : _data->toString();
}

auto PasswordHash::algorithm() const -> PasswordHashAlgorithm {
    if (_data == nullptr) {
        throw err::LogicFailure{"An invalid password hash has no algorithm"};
    }
    return _data->policy().algorithm();
}

auto PasswordHash::isKeyed() const noexcept -> bool {
    return _data != nullptr && _data->isKeyed();
}

auto PasswordHash::keyIdentifier() const noexcept -> std::optional<String> {
    return _data == nullptr ? std::nullopt : _data->keyIdentifier();
}

}
