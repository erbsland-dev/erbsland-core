// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AesBlockCipher.hpp"
#include "AesKeySchedule.hpp"

namespace erbsland::cryptology::impl {

/// ARM64 FEAT_AES implementation of AES-128 and AES-256.
/// AESE/AESMC and AESD/AESIMC map directly to the FIPS 197 round transformations.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
class ArmAesBlockCipher final : public AesBlockCipher {
public:
    /// Create the ARM64 cipher and expand its key.
    explicit ArmAesBlockCipher(mem::ConstByteSpan key) : _keySchedule{key} {}

    ~ArmAesBlockCipher() noexcept override;

    // defaults/deletions
    ArmAesBlockCipher(const ArmAesBlockCipher &) = delete;
    ArmAesBlockCipher(ArmAesBlockCipher &&) = delete;
    auto operator=(const ArmAesBlockCipher &) -> ArmAesBlockCipher & = delete;
    auto operator=(ArmAesBlockCipher &&) -> ArmAesBlockCipher & = delete;

public: // implement AesBlockCipher
    /// @copydoc AesBlockCipher::encrypt()
    [[nodiscard]] auto encrypt(const Block &input) const noexcept -> Block override;
    /// @copydoc AesBlockCipher::decrypt()
    [[nodiscard]] auto decrypt(const Block &input) const noexcept -> Block override;
    /// @copydoc AesBlockCipher::secureErase()
    void secureErase() noexcept override;
    /// @copydoc AesBlockCipher::isHardwareAccelerated()
    [[nodiscard]] auto isHardwareAccelerated() const noexcept -> bool override { return true; }

private:
    AesKeySchedule _keySchedule; ///< Expanded keys retained in `mem::ByteArray` storage.
};

}
