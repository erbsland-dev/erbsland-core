// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeSymmetricKeyAccess_fwd.hpp"

#include "../../symmetric/SymmetricKey.hpp"

namespace erbsland::cryptology::impl {

/// Gives symmetric backends explicit borrowed access to secret key bytes.
/// Returned views become invalid when the key is modified or destroyed.
/// @warning Never retain or expose the returned view beyond the immediate low-level operation.
/// @tested{CryptographicDataBlockTest}
class UnsafeSymmetricKeyAccess final {
public:
    /// Create scoped read-only access to a symmetric key.
    explicit UnsafeSymmetricKeyAccess(const SymmetricKey &key) noexcept : _key{key} {}

    // defaults/deletions
    UnsafeSymmetricKeyAccess() = delete;
    ~UnsafeSymmetricKeyAccess() = default;
    UnsafeSymmetricKeyAccess(const UnsafeSymmetricKeyAccess &) = default;
    UnsafeSymmetricKeyAccess(UnsafeSymmetricKeyAccess &&) noexcept = default;
    auto operator=(const UnsafeSymmetricKeyAccess &) -> UnsafeSymmetricKeyAccess & = delete;
    auto operator=(UnsafeSymmetricKeyAccess &&) -> UnsafeSymmetricKeyAccess & = delete;

public:
    /// Access the key bytes as a borrowed span.
    [[nodiscard]] auto span() const noexcept -> mem::ConstByteSpan { return _key._data.span(); }

private:
    const SymmetricKey &_key; ///< The borrowed key wrapper.
};

}
