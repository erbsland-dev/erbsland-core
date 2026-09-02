// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CryptographicDataBlock.hpp"

#include "../../text/String.hpp"

#include <utility>

namespace erbsland::cryptology {

/// A nonce for an authenticated symmetric encryption construction.
/// @seedoc{/reference/cryptology/cryptographic_operations}
/// @tested{CryptographicDataBlockTest SymmetricEncryptionFrontendTest}
class SymmetricNonce final : public CryptographicDataBlock {
public:
    /// Create an empty nonce placeholder.
    SymmetricNonce() = default;
    /// Create a nonce sharing an owning byte block.
    /// @param data The nonce bytes whose shared allocation is marked as sensitive.
    explicit SymmetricNonce(mem::ByteBlock data) noexcept : CryptographicDataBlock{std::move(data)} {}
    /// Create a nonce by copying borrowed bytes.
    /// @param data The nonce bytes to copy into sensitive storage.
    explicit SymmetricNonce(mem::ConstByteSpan data) : CryptographicDataBlock{data} {}

    // defaults
    ~SymmetricNonce() = default;
    SymmetricNonce(const SymmetricNonce &) = default;
    SymmetricNonce(SymmetricNonce &&) noexcept = default;
    auto operator=(const SymmetricNonce &) -> SymmetricNonce & = default;
    auto operator=(SymmetricNonce &&) noexcept -> SymmetricNonce & = default;

public: // accessors
    /// Access the owning nonce bytes.
    [[nodiscard]] auto data() const noexcept -> const mem::ByteBlock & { return dataBlock(); }
    /// Access a borrowed view of the nonce bytes.
    [[nodiscard]] auto span() const noexcept -> mem::ConstByteSpan { return dataSpan(); }

public: // conversion
    /// Convert the nonce to compact lowercase hexadecimal text for display or diagnostics.
    [[nodiscard]] auto toString() const -> text::String { return dataToString(); }
};

}
