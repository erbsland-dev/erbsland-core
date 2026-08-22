// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteSpan.hpp"
#include "../../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../../text/impl/UnsafeU8StringBuffer.hpp"
#include "../../../text/String.hpp"

#include <algorithm>
#include <cstring>

namespace erbsland::network::impl {

/// Converts opaque ALPN wire bytes to and from raw string storage.
/// @notest{Covered through the TLS protocol tests.}
class TlsAlpnProtocol final {
public:
    /// Raw-byte strict ordering for associative containers.
    struct Less final {
        /// Compare two protocol strings by their exact raw bytes.
        [[nodiscard]] auto operator()(const text::String &left, const text::String &right) const noexcept -> bool {
            const auto leftBytes = bytes(left);
            const auto rightBytes = bytes(right);
            return std::lexicographical_compare(
                leftBytes.begin(), leftBytes.end(), rightBytes.begin(), rightBytes.end());
        }
    };

public:
    /// Create a string preserving the exact wire bytes without UTF-8 validation.
    [[nodiscard]] static auto fromBytes(const mem::ConstByteSpan bytes) -> text::String {
        if (bytes.empty()) {
            return {};
        }
        auto buffer = text::impl::UnsafeU8StringBuffer{unit::ByteLength::fromSizeT(bytes.size())};
        std::memcpy(buffer.data(), bytes.data(), bytes.size());
        return buffer.takeString(unit::ByteLength::fromSizeT(bytes.size()));
    }
    /// Access the exact raw bytes stored in a protocol string.
    [[nodiscard]] static auto bytes(const text::String &protocol) noexcept -> mem::ConstByteSpan {
        const auto data = text::impl::UnsafeU8StringAccess{protocol}.dataSpan();
        return mem::toConstByteSpan(data);
    }
    /// Compare exact protocol bytes without decoded-text equivalence.
    [[nodiscard]] static auto equal(const text::String &left, const text::String &right) noexcept -> bool {
        const auto leftBytes = bytes(left);
        const auto rightBytes = bytes(right);
        return leftBytes.size() == rightBytes.size() &&
            (leftBytes.empty() || std::memcmp(leftBytes.data(), rightBytes.data(), leftBytes.size()) == 0);
    }
};

}
