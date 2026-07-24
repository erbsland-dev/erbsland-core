// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/cryptology/PasswordHasher.hpp>
#include <erbsland/path/Path.hpp>

namespace demo {

/// Creates and loads the demo's separately persisted password-hash keys.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
class PepperStore final {
public:
    /// Create a new pepper document with one identified 32-byte active key.
    static void create(const el::Path &path);
    /// Load the active key and optional fallback keys into a password hasher.
    [[nodiscard]] static auto loadHasher(const el::Path &path) -> el::PasswordHasher;
};

}
