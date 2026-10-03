// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteSpan.hpp"
#include "../../../text/String.hpp"

/// Accesses and compares opaque ALPN wire bytes stored in strings.
namespace erbsland::network::impl::tls_alpn_protocol {

/// Access the exact raw bytes stored in a protocol string.
/// @param protocol The opaque protocol string.
/// @return The exact wire bytes without text normalization.
/// @tested{TlsServerProtocolTest}
[[nodiscard]] auto bytes(const text::String &protocol) noexcept -> mem::ConstByteSpan;

/// Compare the lexicographical order of exact protocol bytes.
/// @param left The first protocol.
/// @param right The second protocol.
/// @return Whether the first protocol sorts before the second.
/// @tested{TlsServerProtocolTest}
[[nodiscard]] auto less(const text::String &left, const text::String &right) noexcept -> bool;

/// Raw-byte strict ordering for associative containers.
/// @tested{TlsServerProtocolTest}
struct Less final {
    /// Compare two protocol strings by their exact raw bytes.
    [[nodiscard]] auto operator()(const text::String &left, const text::String &right) const noexcept -> bool {
        return less(left, right);
    }
};

/// Compare exact protocol bytes without decoded-text equivalence.
/// @param left The first protocol.
/// @param right The second protocol.
/// @return Whether both strings contain exactly the same bytes.
/// @tested{TlsServerProtocolTest}
[[nodiscard]] auto equal(const text::String &left, const text::String &right) noexcept -> bool;

}
