// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AesCbcState.hpp"
#include "SymmetricDecryptorData.hpp"

namespace erbsland::cryptology::impl {

/// AES-256-CBC decryption worker for random-fill and ISO/IEC 9797-1 method 2 padding.
/// @tested{AesCbcTest AesCbcFullTest}
class AesCbcDecryptorData final : public SymmetricDecryptorData {
public:
    /// Create an AES-256-CBC decryption worker.
    /// @param type The selected CBC padding construction.
    /// @param key The validated 256-bit key.
    /// @param iv The validated 128-bit IV.
    AesCbcDecryptorData(SymmetricEncryptionType type, mem::ConstByteSpan key, mem::ConstByteSpan iv);

    ~AesCbcDecryptorData() noexcept override;

    // defaults/deletions
    AesCbcDecryptorData(const AesCbcDecryptorData &) = delete;
    AesCbcDecryptorData(AesCbcDecryptorData &&) = delete;
    auto operator=(const AesCbcDecryptorData &) -> AesCbcDecryptorData & = delete;
    auto operator=(AesCbcDecryptorData &&) -> AesCbcDecryptorData & = delete;

public:
    /// @copydoc SymmetricDecryptorData::type()
    [[nodiscard]] auto type() const noexcept -> SymmetricEncryptionType override;
    /// @copydoc SymmetricDecryptorData::addAuthenticatedData()
    void addAuthenticatedData(mem::ConstByteSpan data) override;
    /// @copydoc SymmetricDecryptorData::decrypt()
    [[nodiscard]] auto decrypt(mem::ConstByteSpan data) -> mem::ByteBlock override;
    /// @copydoc SymmetricDecryptorData::finalize(const SymmetricTag &)
    [[nodiscard]] auto finalize(const SymmetricTag &tag) -> mem::ByteBlock override;
    /// @copydoc SymmetricDecryptorData::finalize()
    [[nodiscard]] auto finalize() -> mem::ByteBlock override;
    /// @copydoc SymmetricDecryptorData::secureErase()
    void secureErase() noexcept override;

private:
    SymmetricEncryptionType _type; ///< Selected legacy construction.
    AesCbcState _state;            ///< CBC chaining and padding state.
};

}
