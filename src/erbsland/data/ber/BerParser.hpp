// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../asn1/Asn1Limits.hpp"
#include "../asn1/Asn1Node.hpp"

namespace erbsland::data::ber {

/// Decode a complete LDAP-profile BER document.
/// @tested{BerCodecTest}
class BerParser final {
public:
    /// Parse one complete document.
    /// @param bytes Encoded document.
    /// @param limits Resource budgets.
    /// @return The parsed root or no value for malformed input.
    [[nodiscard]] static auto parse(const mem::ByteBlock &bytes, asn1::Asn1Limits limits = {}) noexcept
        -> std::optional<asn1::Asn1Node>;
    /// Parse one complete document with positioned diagnostics.
    /// @param bytes Encoded document.
    /// @param limits Resource budgets.
    /// @return The parsed root.
    /// @throws err::ParseError For invalid input, excess resource use or trailing bytes.
    [[nodiscard]] static auto parseOrThrow(const mem::ByteBlock &bytes, asn1::Asn1Limits limits = {}) -> asn1::Asn1Node;
    /// Probe the outer header and return the first complete frame length.
    /// @param bytes Receive buffer, possibly partial or containing several frames.
    /// @param limits Resource budgets for one frame.
    /// @return The first frame length or no value if more bytes are needed.
    /// @throws err::ParseError For an invalid or oversized header.
    [[nodiscard]] static auto frameLength(const mem::ByteBlock &bytes, asn1::Asn1Limits limits = {})
        -> std::optional<unit::ByteCount>;
};

}
