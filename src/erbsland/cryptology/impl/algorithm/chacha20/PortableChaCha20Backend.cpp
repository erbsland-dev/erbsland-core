// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PortableChaCha20Backend.hpp"

#include "ChaCha20Operations.hpp"

namespace erbsland::cryptology::impl {

PortableChaCha20Backend::PortableChaCha20Backend(
    const mem::ConstByteSpan key, const mem::ConstByteSpan nonce) noexcept {
    _key.overwrite(key);
    _nonce.overwrite(nonce);
}

PortableChaCha20Backend::~PortableChaCha20Backend() noexcept {
    secureErase();
}

auto PortableChaCha20Backend::generateBlocks(const uint32_t firstCounter, const std::size_t blockCount) noexcept
    -> Batch {
    auto result = Batch{};
    for (auto blockIndex = std::size_t{}; blockIndex < blockCount; ++blockIndex) {
        auto keyStream = chacha20::block(_key.span(), _nonce.span(), firstCounter + static_cast<uint32_t>(blockIndex));
        result.overwrite(unit::ByteIndex{blockIndex * 64U}, keyStream.span());
        keyStream.secureErase();
    }
    return result;
}

void PortableChaCha20Backend::secureErase() noexcept {
    _key.secureErase();
    _nonce.secureErase();
}

}
