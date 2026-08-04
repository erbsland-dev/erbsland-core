// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AesBlockCipher.hpp"
#include "AesKeySchedule.hpp"

namespace erbsland::cryptology::impl {

/// x86-64 AES-NI implementation of AES-128 and AES-256.
/// AESENC/AESENCLAST and AESDEC/AESDECLAST map to the FIPS 197 round transformations.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
class X86AesBlockCipher final : public AesBlockCipher {
public:
    /// Create the AES-NI cipher and expand its key.
    explicit X86AesBlockCipher(mem::ConstByteSpan key) : _keySchedule{key} {}

    ~X86AesBlockCipher() noexcept override;

    // defaults/deletions
    X86AesBlockCipher(const X86AesBlockCipher &) = delete;
    X86AesBlockCipher(X86AesBlockCipher &&) = delete;
    auto operator=(const X86AesBlockCipher &) -> X86AesBlockCipher & = delete;
    auto operator=(X86AesBlockCipher &&) -> X86AesBlockCipher & = delete;

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
