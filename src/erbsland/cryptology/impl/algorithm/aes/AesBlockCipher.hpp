// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteArray.hpp"

namespace erbsland::cryptology::impl {

/// Internal AES block-cipher contract for portable and hardware implementations.
/// The block mapping and round behavior follow FIPS 197, Sections 3.4, 5.1, and 5.3.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
class AesBlockCipher {
public:
    /// The fixed 128-bit AES block type.
    using Block = mem::ByteArray<16>;

public:
    // defaults/deletions
    AesBlockCipher() = default;
    virtual ~AesBlockCipher() = default;
    AesBlockCipher(const AesBlockCipher &) = delete;
    AesBlockCipher(AesBlockCipher &&) = delete;
    auto operator=(const AesBlockCipher &) -> AesBlockCipher & = delete;
    auto operator=(AesBlockCipher &&) -> AesBlockCipher & = delete;

public:
    /// Encrypt one 128-bit block as specified by FIPS 197, Section 5.1.
    [[nodiscard]] virtual auto encrypt(const Block &input) const noexcept -> Block = 0;
    /// Decrypt one 128-bit block as specified by FIPS 197, Section 5.3.
    [[nodiscard]] virtual auto decrypt(const Block &input) const noexcept -> Block = 0;
    /// Securely erase all retained key material.
    virtual void secureErase() noexcept = 0;
    /// Test whether this implementation uses dedicated CPU cryptographic instructions.
    [[nodiscard]] virtual auto isHardwareAccelerated() const noexcept -> bool = 0;
};

}
