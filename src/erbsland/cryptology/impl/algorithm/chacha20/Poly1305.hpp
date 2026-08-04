// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteSpan.hpp"

namespace erbsland::cryptology::impl {

/// Private streaming authenticator contract for RFC 8439 Poly1305.
/// @tested{ChaCha20PrimitiveTest ChaCha20BackendFullTest}
class Poly1305 {
public:
    /// The fixed 128-bit authenticator output.
    /// @tested{ChaCha20PrimitiveTest ChaCha20Poly1305Test}
    using Tag = mem::ByteArray<16>;

public:
    // defaults/deletions
    Poly1305() = default;
    virtual ~Poly1305() = default;
    Poly1305(const Poly1305 &) = delete;
    Poly1305(Poly1305 &&) = delete;
    auto operator=(const Poly1305 &) -> Poly1305 & = delete;
    auto operator=(Poly1305 &&) -> Poly1305 & = delete;

public:
    /// Authenticate the next exact byte sequence.
    virtual void update(mem::ConstByteSpan data) noexcept = 0;
    /// Finalize the authenticator and return its tag.
    [[nodiscard]] virtual auto finalize() noexcept -> Tag = 0;
    /// Securely erase the one-time key and accumulated state.
    virtual void secureErase() noexcept = 0;
};

}
