// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteSpan.hpp>
#include <erbsland/KeyAgreementPrivateKey.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocol_fwd.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolTestAccess_fwd.hpp>
#include <erbsland/network/impl/tls/TlsWireWriter.hpp>
#include <erbsland/TlsRecordEncryptor.hpp>

#include <cstdint>
#include <span>

/// Exercise the TLS client protocol for one fuzz input.
class TlsClientProtocolFuzz final {
private:
    using Protocol = erbsland::network::impl::TlsClientProtocol;
    using TestAccess = erbsland::network::impl::TlsClientProtocolTestAccess;
    using Writer = erbsland::network::impl::TlsWireWriter;

public:
    /// Create a harness for one input.
    /// @param data The bytes supplied by libFuzzer.
    explicit TlsClientProtocolFuzz(const std::span<const uint8_t> data) : _data{data} {}

public: // operations
    /// Process the input once.
    /// @return Zero to continue fuzzing.
    [[nodiscard]] auto run() const -> int;

private:
    /// Create a client protocol using the shared certificate fixture.
    [[nodiscard]] static auto createProtocol() -> Protocol;

    /// Create a deterministic key for the fixture handshake.
    [[nodiscard]] static auto privateKey(const uint8_t value) -> erbsland::cryptology::KeyAgreementPrivateKey;

    /// Start the client protocol with deterministic parameters.
    static void start(Protocol &protocol, TestAccess &access);

    /// Advance the client to encrypted handshake messages.
    [[nodiscard]] static auto beginEncryptedHandshake(Protocol &protocol, TestAccess &access)
        -> erbsland::cryptology::TlsRecordEncryptor;

    /// Feed the supplied bytes during the certificate handshake.
    static void fuzzCertificateHandshake(
        Protocol &protocol, TestAccess &access, const std::span<const uint8_t> payload, const unsigned mode);

    /// Build a certificate message from the shared fixture.
    [[nodiscard]] static auto certificateMessage() -> erbsland::mem::ByteBlock;

    /// Drive the fixture handshake to the established state.
    static void establish(Protocol &protocol, TestAccess &access);

    /// Build the fixture server's hello record.
    [[nodiscard]] static auto serverHello(const erbsland::cryptology::KeyAgreementPrivateKey &serverPrivate)
        -> erbsland::mem::ByteBlock;

    /// Feed bytes to the protocol in fragments.
    static void feedFragmented(Protocol &protocol, const erbsland::mem::ConstByteSpan data, const uint8_t selector);

private:
    static constexpr auto cFragmentSelector = uint8_t{17U}; ///< Fragment selector for certificate handshake inputs.
    std::span<const uint8_t> _data; ///< Mode/fragment selector followed by arbitrary transport or inner content bytes.
};
