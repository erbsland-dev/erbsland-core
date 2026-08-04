// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AesCbcState.hpp"
#include "SymmetricEncryptorData.hpp"

namespace erbsland::cryptology::impl {

/// AES-256-CBC encryption worker for random-fill and ISO/IEC 9797-1 method 2 padding.
/// @tested{AesCbcTest AesCbcFullTest}
class AesCbcEncryptorData final : public SymmetricEncryptorData {
public:
    /// Create an AES-256-CBC encryption worker.
    /// @param type The selected CBC padding construction.
    /// @param key The validated 256-bit key.
    /// @param iv The validated 128-bit IV.
    AesCbcEncryptorData(SymmetricEncryptionType type, mem::ConstByteSpan key, mem::ConstByteSpan iv);

    ~AesCbcEncryptorData() noexcept override;

    // defaults/deletions
    AesCbcEncryptorData(const AesCbcEncryptorData &) = delete;
    AesCbcEncryptorData(AesCbcEncryptorData &&) = delete;
    auto operator=(const AesCbcEncryptorData &) -> AesCbcEncryptorData & = delete;
    auto operator=(AesCbcEncryptorData &&) -> AesCbcEncryptorData & = delete;

public:
    /// @copydoc SymmetricEncryptorData::type()
    [[nodiscard]] auto type() const noexcept -> SymmetricEncryptionType override;
    /// @copydoc SymmetricEncryptorData::addAuthenticatedData()
    void addAuthenticatedData(mem::ConstByteSpan data) override;
    /// @copydoc SymmetricEncryptorData::encrypt()
    [[nodiscard]] auto encrypt(mem::ConstByteSpan data) -> mem::ByteBlock override;
    /// @copydoc SymmetricEncryptorData::finalize()
    [[nodiscard]] auto finalize() -> mem::ByteBlock override;
    /// @copydoc SymmetricEncryptorData::tag()
    [[nodiscard]] auto tag() const -> SymmetricTag override;
    /// @copydoc SymmetricEncryptorData::secureErase()
    void secureErase() noexcept override;

private:
    SymmetricEncryptionType _type; ///< Selected legacy construction.
    AesCbcState _state;            ///< CBC chaining and padding state.
};

}
