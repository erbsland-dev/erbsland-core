// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymmetricEncryptorData_fwd.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../symmetric/SymmetricEncryptionType.hpp"
#include "../../symmetric/SymmetricTag.hpp"

namespace erbsland::cryptology::impl {

/// Private contract implemented by symmetric encryption workers.
/// Implementations retain their key and nonce or IV, securely erase all state on request and destruction, and enforce
/// primitive-specific total-length and padding rules.
/// @tested{SymmetricEncryptionFrontendTest}
class SymmetricEncryptorData {
public:
    // defaults/deletions
    SymmetricEncryptorData() = default;
    virtual ~SymmetricEncryptorData() = default;
    SymmetricEncryptorData(const SymmetricEncryptorData &) = delete;
    SymmetricEncryptorData(SymmetricEncryptorData &&) = delete;
    auto operator=(const SymmetricEncryptorData &) -> SymmetricEncryptorData & = delete;
    auto operator=(SymmetricEncryptorData &&) -> SymmetricEncryptorData & = delete;

public:
    /// Get the encryption construction implemented by this worker.
    [[nodiscard]] virtual auto type() const noexcept -> SymmetricEncryptionType = 0;
    /// Add associated data before payload encryption begins.
    /// @param data The next associated-data bytes.
    virtual void addAuthenticatedData(mem::ConstByteSpan data) = 0;
    /// Encrypt the next payload bytes.
    /// @param data The next plaintext bytes.
    /// @return The encrypted output currently available.
    [[nodiscard]] virtual auto encrypt(mem::ConstByteSpan data) -> mem::ByteBlock = 0;
    /// Finalize encryption.
    /// @return Any final encrypted output.
    [[nodiscard]] virtual auto finalize() -> mem::ByteBlock = 0;
    /// Get the cached authentication tag after finalization.
    /// @return The authentication tag.
    [[nodiscard]] virtual auto tag() const -> SymmetricTag = 0;
    /// Securely erase the key and all worker state.
    virtual void secureErase() noexcept = 0;
};

}
