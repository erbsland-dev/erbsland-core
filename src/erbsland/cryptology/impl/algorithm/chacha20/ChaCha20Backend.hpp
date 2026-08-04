// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteSpan.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::cryptology::impl {

/// Private block-generation contract for RFC 8439 ChaCha20 implementations.
/// @tested{ChaCha20PrimitiveTest ChaCha20BackendFullTest}
class ChaCha20Backend {
public:
    /// Storage for the maximum four-block backend batch.
    /// @tested{ChaCha20BackendFullTest}
    using Batch = mem::ByteArray<256>;

public:
    // defaults/deletions
    ChaCha20Backend() = default;
    virtual ~ChaCha20Backend() = default;
    ChaCha20Backend(const ChaCha20Backend &) = delete;
    ChaCha20Backend(ChaCha20Backend &&) = delete;
    auto operator=(const ChaCha20Backend &) -> ChaCha20Backend & = delete;
    auto operator=(ChaCha20Backend &&) -> ChaCha20Backend & = delete;

public:
    /// Generate consecutive 64-byte key-stream blocks.
    /// @param firstCounter The counter for the first block.
    /// @param blockCount The number of blocks; callers use values from one through four.
    /// @return Four-block storage whose first `blockCount * 64` bytes contain the generated stream.
    [[nodiscard]] virtual auto generateBlocks(uint32_t firstCounter, std::size_t blockCount) noexcept -> Batch = 0;
    /// Securely erase the retained key and nonce.
    virtual void secureErase() noexcept = 0;
};

}
