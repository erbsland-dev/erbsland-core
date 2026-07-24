// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/cryptology/PasswordHasher.hpp>
#include <erbsland/cryptology/PasswordHashKey.hpp>
#include <erbsland/util/List.hpp>

namespace demo {

/// Build a password hasher from one active pepper and its historical fallbacks.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
[[nodiscard]] auto buildPasswordHasher(
    const el::PasswordHashKey &activeKey, const el::List<el::PasswordHashKey> &fallbackKeys) -> el::PasswordHasher;

}
