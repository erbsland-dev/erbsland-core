// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/PasswordHasher.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

namespace demo {

/// Creates and loads the demo's separately persisted password-hash keys.
/// The store is bound to one pepper-file path without retaining sensitive key material.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
class PepperStore final {
public:
    /// Create a pepper store for the given file path.
    explicit PepperStore(el::Path path);

public:
    /// Create a new pepper document with one identified 32-byte active key.
    void create() const;
    /// Load the active key and optional fallback keys into a password hasher.
    [[nodiscard]] auto loadHasher() const -> el::PasswordHasher;

private:
    /// Throw a pepper-format error associated with its source path.
    [[noreturn]] void throwInvalidPepper(const el::String &description) const;
    /// Read and mark a 32-byte key value from a pepper section.
    [[nodiscard]] auto readKeyBytes(const el::conf::ValuePtr &section) const -> el::ByteBlock;

private:
    el::Path _path; ///< The pepper file managed by this store.
};

}
