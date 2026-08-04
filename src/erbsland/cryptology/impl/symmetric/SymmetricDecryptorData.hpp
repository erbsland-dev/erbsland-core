// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymmetricDecryptorData_fwd.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../symmetric/SymmetricEncryptionType.hpp"
#include "../../symmetric/SymmetricTag.hpp"

namespace erbsland::cryptology::impl {

/// Private contract implemented by symmetric decryption workers.
/// Implementations retain their key and nonce or IV, securely erase all state on request and destruction, and enforce
/// primitive-specific total-length, padding, and authentication rules.
/// @tested{SymmetricEncryptionFrontendTest}
class SymmetricDecryptorData {
public:
    // defaults/deletions
    SymmetricDecryptorData() = default;
    virtual ~SymmetricDecryptorData() = default;
    SymmetricDecryptorData(const SymmetricDecryptorData &) = delete;
    SymmetricDecryptorData(SymmetricDecryptorData &&) = delete;
    auto operator=(const SymmetricDecryptorData &) -> SymmetricDecryptorData & = delete;
    auto operator=(SymmetricDecryptorData &&) -> SymmetricDecryptorData & = delete;

public:
    /// Get the encryption construction implemented by this worker.
    [[nodiscard]] virtual auto type() const noexcept -> SymmetricEncryptionType = 0;
    /// Add associated data before payload decryption begins.
    /// @param data The next associated-data bytes.
    virtual void addAuthenticatedData(mem::ConstByteSpan data) = 0;
    /// Decrypt the next payload bytes.
    /// @param data The next encrypted bytes.
    /// @return The plaintext currently available, unauthenticated for AEAD types.
    [[nodiscard]] virtual auto decrypt(mem::ConstByteSpan data) -> mem::ByteBlock = 0;
    /// Finalize and authenticate AEAD decryption.
    /// @param tag The expected authentication tag.
    /// @return Any final plaintext output.
    [[nodiscard]] virtual auto finalize(const SymmetricTag &tag) -> mem::ByteBlock = 0;
    /// Finalize unauthenticated decryption.
    /// @return Any final plaintext output after padding removal.
    [[nodiscard]] virtual auto finalize() -> mem::ByteBlock = 0;
    /// Securely erase the key and all worker state.
    virtual void secureErase() noexcept = 0;
};

}
