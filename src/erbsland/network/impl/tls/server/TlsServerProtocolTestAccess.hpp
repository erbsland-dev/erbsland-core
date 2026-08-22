// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerProtocol.hpp"
#include "TlsServerProtocolTestAccess_fwd.hpp"

#include "../TlsWireWriter.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../text/Literals.hpp"

#include <utility>

namespace erbsland::network::impl {

/// Deterministic and erasure test access for the internal TLS server core.
/// Specification: RFC 8446 Sections 4.1.3, 4.2.8, and 7.1.
/// @tested{TlsServerProtocolTest}
class TlsServerProtocolTestAccess final {
public:
    /// Create test access for one live protocol.
    explicit TlsServerProtocolTestAccess(TlsServerProtocol &protocol) noexcept : _protocol{protocol} {}

public:
    /// Resume ClientHello with deterministic server random and X25519 inputs.
    /// @param random Exact 32-byte ServerHello.random.
    /// @param privateKey Deterministic protected X25519 private key.
    void resume(mem::ByteBlock random, cryptology::KeyAgreementPrivateKey privateKey) {
        using namespace text::literals;
        if (_protocol._checkpoint != TlsServerProtocolCheckpoint::ClientHello) {
            throw err::LogicError{"TLS server test resume requires the ClientHello checkpoint."_el};
        }
        _protocol._checkpoint = TlsServerProtocolCheckpoint::None;
        _protocol.resumeWithInputs(std::move(random), std::move(privateKey));
    }
    /// Queue a valid server KeyUpdate and advance the server write generation at the exact message boundary.
    /// Specification: RFC 8446 Sections 4.6.3 and 7.2.
    /// @param requestPeerUpdate Whether the client must answer with its own KeyUpdate.
    void queueKeyUpdate(const bool requestPeerUpdate) {
        using namespace text::literals;
        if (_protocol._state != TlsServerProtocolState::Established || _protocol.hasCheckpoint()) {
            throw err::LogicError{"TLS server test KeyUpdate requires an unpaused established protocol."_el};
        }
        // RFC 8446 Section 4.6.3: encode request_update under the old server application write generation.
        auto writer = TlsWireWriter{};
        writer.writeU8(24U);
        writer.writeU24(1U);
        writer.writeU8(requestPeerUpdate ? 1U : 0U);
        const auto message = writer.finish();
        _protocol.queueProtectedHandshake(message.span());

        // RFC 8446 Sections 4.6.3 and 7.2: install server_application_traffic_secret_N+1 only after the KeyUpdate
        // record has been protected, so subsequent records use the new generation.
        _protocol._encryptor.updateApplicationTrafficKeys();
    }
    /// Create an independent client application sender from generation zero.
    /// Specification: RFC 8446 Sections 7.1 and 7.3.
    [[nodiscard]] auto clientApplicationEncryptor() const -> cryptology::TlsRecordEncryptor {
        return cryptology::TlsRecordEncryptor{
            *_protocol._cipherSuite, _protocol._keySchedule->applicationTrafficSecret(true)};
    }
    /// Create an independent client handshake sender at sequence number zero.
    /// Specification: RFC 8446 Sections 5.3 and 7.1.
    [[nodiscard]] auto clientHandshakeEncryptor() const -> cryptology::TlsRecordEncryptor {
        return cryptology::TlsRecordEncryptor{
            *_protocol._cipherSuite, _protocol._keySchedule->handshakeTrafficSecret(true)};
    }
    /// Test whether all owned secret protocol state has been erased.
    [[nodiscard]] auto securityStateIsEmpty() const noexcept -> bool {
        return _protocol._privateKey.isEmpty() && _protocol._keySchedule == nullptr &&
            _protocol._transcript == nullptr && _protocol._encryptor.isEmpty() && _protocol._decryptor.isEmpty() &&
            _protocol._handshakeStream.bufferedLength().isZero() && _protocol._recordStream.bufferedLength().isZero() &&
            _protocol._clientHello.isEmpty() && _protocol._legacySessionId.isEmpty() &&
            _protocol._clientKey.isEmpty() && _protocol._applicationInput.empty();
    }

private:
    TlsServerProtocol &_protocol; ///< Protocol under test.
};

}
