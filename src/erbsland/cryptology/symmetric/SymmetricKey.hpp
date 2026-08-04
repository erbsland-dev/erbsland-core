// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CryptographicDataBlock.hpp"

#include <utility>

namespace erbsland::cryptology {

/// Secret key material for symmetric encryption.
/// The public API deliberately provides no access to the stored bytes or their text representation.
/// @seedoc{/reference/cryptology/symmetric_encryption}
/// @tested{CryptographicDataBlockTest SymmetricEncryptionFrontendTest}
class SymmetricKey final : public CryptographicDataBlock {
public:
    /// Create an empty key placeholder.
    SymmetricKey() = default;
    /// Create a key sharing an owning byte block.
    /// @param data The key bytes whose shared allocation is marked as sensitive.
    explicit SymmetricKey(mem::ByteBlock data) noexcept : CryptographicDataBlock{std::move(data)} {}
    /// Create a key by copying borrowed bytes.
    /// @param data The key bytes to copy into sensitive storage.
    explicit SymmetricKey(mem::ConstByteSpan data) : CryptographicDataBlock{data} {}

    // defaults
    ~SymmetricKey() = default;
    SymmetricKey(const SymmetricKey &) = default;
    SymmetricKey(SymmetricKey &&) noexcept = default;
    auto operator=(const SymmetricKey &) -> SymmetricKey & = default;
    auto operator=(SymmetricKey &&) noexcept -> SymmetricKey & = default;
};

}
