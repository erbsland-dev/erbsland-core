// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../TlsAlpnProtocol.hpp"
#include "../TlsWireWriter.hpp"

#include "../../../../cryptology/keys/KeyAgreementPublicKey.hpp"
#include "../../../../mem/ByteSpan.hpp"
#include "../../../../text/String.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../Host.hpp"

#include <vector>

namespace erbsland::network::impl {

/// Serializer for the initial RFC 8446 TLS 1.3 client ClientHello and plaintext record.
/// It offers only TLS 1.3, X25519/P-256, the library's three TLS 1.3 suites, supported CertificateVerify schemes, DNS
/// SNI, optional ALPN, and a non-empty legacy session ID for middlebox compatibility. Specification:
/// https://www.rfc-editor.org/rfc/rfc8446.html#section-4.1.2
/// @tested{TlsClientProtocolTest}
class TlsClientHelloBuilder final {
public:
    /// Create a builder for one unresolved reference identity and ALPN offer.
    /// @param host DNS or IP reference identity; only DNS names produce SNI.
    /// @param alpnProtocols At most 32 ALPN identifiers in preference order, each 1-255 bytes and together at most 4096
    /// encoded bytes including length prefixes.
    /// @param groups Nonempty, duplicate-free ordered key-agreement groups.
    TlsClientHelloBuilder(
        Host host,
        text::StringList alpnProtocols,
        std::vector<cryptology::KeyAgreementAlgorithm> groups = {
            cryptology::KeyAgreementAlgorithm::X25519, cryptology::KeyAgreementAlgorithm::P256});

    // defaults
    ~TlsClientHelloBuilder() = default;
    TlsClientHelloBuilder(const TlsClientHelloBuilder &) = default;
    TlsClientHelloBuilder(TlsClientHelloBuilder &&) noexcept = default;
    auto operator=(const TlsClientHelloBuilder &) -> TlsClientHelloBuilder & = default;
    auto operator=(TlsClientHelloBuilder &&) noexcept -> TlsClientHelloBuilder & = default;

public:
    /// Build a complete ClientHello handshake message.
    /// @param random Exact 32-byte ClientHello.random.
    /// @param legacySessionId Exact 32-byte compatibility-mode session ID.
    /// @param publicKey A non-empty public key from a configured group.
    /// @param cookie Optional opaque cookie echoed from HelloRetryRequest.
    /// @return Complete handshake header and body, without a record header.
    [[nodiscard]] auto buildHandshake(
        mem::ConstByteSpan random,
        mem::ConstByteSpan legacySessionId,
        const cryptology::KeyAgreementPublicKey &publicKey,
        mem::ConstByteSpan cookie = {}) const -> mem::ByteBlock;
    /// Wrap a complete ClientHello in its initial TLSPlaintext record.
    /// @param handshake Complete ClientHello header and body.
    /// @return Record type handshake, legacy version 0x0301, and exact fragment.
    [[nodiscard]] auto buildInitialRecord(mem::ConstByteSpan handshake) const -> mem::ByteBlock;

private:
    /// Encode all offered extensions in RFC field order.
    [[nodiscard]] auto buildExtensions(
        const cryptology::KeyAgreementPublicKey &publicKey, mem::ConstByteSpan cookie = {}) const -> mem::ByteBlock;
    /// Encode one extension header and body.
    static void appendExtension(TlsWireWriter &extensions, uint16_t type, mem::ConstByteSpan body);

private:
    std::vector<cryptology::KeyAgreementAlgorithm> _groups; ///< Ordered supported groups.
    Host _host;                                             ///< Original identity retained for DNS SNI selection.
    text::StringList _alpnProtocols;                        ///< Validated opaque ALPN identifiers.
};

}
