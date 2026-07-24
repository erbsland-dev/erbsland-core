// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordVerification.hpp"

#include <utility>

namespace erbsland::cryptology {

PasswordVerification::PasswordVerification(const bool accepted, std::optional<PasswordHash> replacement) noexcept :
    _accepted{accepted}, _replacement{std::move(replacement)} {
}

}
