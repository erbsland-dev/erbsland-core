// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ChaCha20Backend.hpp"

#include "../../../../mem/ByteArray.hpp"

namespace erbsland::cryptology::impl {

/// Portable scalar RFC 8439 ChaCha20 block generator.
/// @tested{ChaCha20PrimitiveTest ChaCha20BackendFullTest}
class PortableChaCha20Backend : public ChaCha20Backend {
public:
    /// Retain a validated key and nonce.
    PortableChaCha20Backend(mem::ConstByteSpan key, mem::ConstByteSpan nonce) noexcept;

    ~PortableChaCha20Backend() noexcept override;

    // defaults/deletions
    PortableChaCha20Backend(const PortableChaCha20Backend &) = delete;
    PortableChaCha20Backend(PortableChaCha20Backend &&) = delete;
    auto operator=(const PortableChaCha20Backend &) -> PortableChaCha20Backend & = delete;
    auto operator=(PortableChaCha20Backend &&) -> PortableChaCha20Backend & = delete;

public: // implement ChaCha20Backend
    /// Generate one through four consecutive scalar RFC 8439 blocks.
    [[nodiscard]] auto generateBlocks(uint32_t firstCounter, std::size_t blockCount) noexcept -> Batch override;
    /// Securely erase the retained key and nonce bytes.
    void secureErase() noexcept override;

protected:
    mem::ByteArray<32> _key;   ///< Retained 256-bit key.
    mem::ByteArray<12> _nonce; ///< Retained 96-bit nonce.
};

}
