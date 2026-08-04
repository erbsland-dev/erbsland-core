// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AesBlockCipher.hpp"
#include "AesKeySchedule.hpp"

namespace erbsland::cryptology::impl {

/// Portable AES-128/AES-256 implementation without secret-indexed lookup tables.
/// All round transformations use fixed iteration counts and follow FIPS 197, Sections 5.1 and 5.3.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
class PortableAesBlockCipher final : public AesBlockCipher {
public:
    /// Create the portable cipher and expand its key.
    explicit PortableAesBlockCipher(mem::ConstByteSpan key) : _keySchedule{key} {}

    ~PortableAesBlockCipher() noexcept override;

    // defaults/deletions
    PortableAesBlockCipher(const PortableAesBlockCipher &) = delete;
    PortableAesBlockCipher(PortableAesBlockCipher &&) = delete;
    auto operator=(const PortableAesBlockCipher &) -> PortableAesBlockCipher & = delete;
    auto operator=(PortableAesBlockCipher &&) -> PortableAesBlockCipher & = delete;

public: // implement AesBlockCipher
    /// @copydoc AesBlockCipher::encrypt()
    [[nodiscard]] auto encrypt(const Block &input) const noexcept -> Block override;
    /// @copydoc AesBlockCipher::decrypt()
    [[nodiscard]] auto decrypt(const Block &input) const noexcept -> Block override;
    /// @copydoc AesBlockCipher::secureErase()
    void secureErase() noexcept override;
    /// @copydoc AesBlockCipher::isHardwareAccelerated()
    [[nodiscard]] auto isHardwareAccelerated() const noexcept -> bool override { return false; }

private:
    /// Apply FIPS 197, Section 5.1.1, SubBytes to every state byte.
    static void substituteBytes(Block &state) noexcept;
    /// Apply FIPS 197, Section 5.3.2, InvSubBytes to every state byte.
    static void inverseSubstituteBytes(Block &state) noexcept;
    /// Apply FIPS 197, Section 5.1.2, ShiftRows.
    static void shiftRows(Block &state) noexcept;
    /// Apply FIPS 197, Section 5.3.1, InvShiftRows.
    static void inverseShiftRows(Block &state) noexcept;
    /// Apply FIPS 197, Section 5.1.3, MixColumns.
    static void mixColumns(Block &state) noexcept;
    /// Apply FIPS 197, Section 5.3.3, InvMixColumns.
    static void inverseMixColumns(Block &state) noexcept;

private:
    AesKeySchedule _keySchedule; ///< Expanded AES round keys.
};

}
