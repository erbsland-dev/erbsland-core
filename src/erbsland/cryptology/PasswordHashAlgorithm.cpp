// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordHashAlgorithm.hpp"

#include "../text/Literals.hpp"
#include "../text/String.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

auto PasswordHashAlgorithm::toString() const -> text::String {
    return _value == Argon2id ? "argon2id"_el : "scrypt"_el;
}

auto PasswordHashAlgorithm::fromString(const text::String &text) noexcept -> std::optional<PasswordHashAlgorithm> {
    if (text == "argon2id"_el) {
        return PasswordHashAlgorithm{Argon2id};
    }
    if (text == "scrypt"_el) {
        return PasswordHashAlgorithm{Scrypt};
    }
    return std::nullopt;
}

}
