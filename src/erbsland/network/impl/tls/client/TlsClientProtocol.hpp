// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientHelloBuilder.hpp"
#include "TlsClientProtocolCheckpoint.hpp"
#include "TlsClientProtocolOptions.hpp"
#include "TlsClientProtocolState.hpp"
#include "TlsClientProtocolTestAccess_fwd.hpp"

#include "../TlsAlertDescription.hpp"
#include "../TlsHandshakeStream.hpp"
#include "../TlsRecordStream.hpp"

#include "../../../../cryptology/impl/tls/Tls13KeySchedule.hpp"
#include "../../../../cryptology/impl/tls/Tls13Transcript.hpp"
#include "../../../../cryptology/keys/KeyAgreementPrivateKey.hpp"
#include "../../../../cryptology/keys/PublicKey.hpp"
#include "../../../../cryptology/tls/TlsCipherSuite.hpp"
#include "../../../../cryptology/tls_record/TlsRecordDecryptor.hpp"
#include "../../../../cryptology/tls_record/TlsRecordEncryptor.hpp"
#include "../../../../cryptology/x509/X509Certificate.hpp"
#include "../../../../mem/ByteBlock.hpp"
#include "../../../../mem/ByteSpan.hpp"
#include "../../../../text/String.hpp"
#include "../../../../util/List.hpp"
#include "../../../source/NetworkSendStatus.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace erbsland::network::impl {

/// Transport-independent authenticated TLS 1.3 client protocol core.
/// The initial implementation follows RFC 8446 for a non-PSK X25519 client, RFC 6066 for DNS SNI, and RFC 7301 for
/// ALPN. It owns record framing, handshake authentication, application protection, alerts, KeyUpdate, and closure,
/// while a future `TcpConnection` decorator owns transport I/O and the actual handshake deadline.
/// Specification: https://www.rfc-editor.org/rfc/rfc8446.html
/// @tested{TlsClientProtocolTest}
class TlsClientProtocol final {
    friend class TlsClientProtocolTestAccess;

private:
    /// Expected next authenticated handshake message.
    enum class HandshakeStep : uint8_t {
        ServerHello,          ///< Await plaintext ServerHello.
        EncryptedExtensions,  ///< Await encrypted EncryptedExtensions.
        CertificateOrRequest, ///< Await Certificate or optional CertificateRequest.
        Certificate,          ///< Await Certificate after CertificateRequest.
        CertificateVerify,    ///< Await server CertificateVerify.
        ServerFinished,       ///< Await server Finished.
        PostHandshake,        ///< Main handshake completed.
    };
    /// Borrowed extension data from one complete enclosing message.
    struct ExtensionView {
        uint16_t type{};           ///< ExtensionType wire value.
        mem::ConstByteSpan data{}; ///< Exact extension_data bytes.
    };
    /// Callback invoked once for each parsed extension.
    using ExtensionFn = std::function<void(const ExtensionView &)>;

public:
    /// Create an inactive TLS client protocol core.
    /// @param options Original identity, explicit trust policy, ALPN offer, time, and queue bounds.
    explicit TlsClientProtocol(TlsClientProtocolOptions options);
    /// Erase all secret protocol state.
    ~TlsClientProtocol();

    // defaults/deletions
    TlsClientProtocol(const TlsClientProtocol &) = delete;
    TlsClientProtocol(TlsClientProtocol &&) = delete;
    auto operator=(const TlsClientProtocol &) -> TlsClientProtocol & = delete;
    auto operator=(TlsClientProtocol &&) -> TlsClientProtocol & = delete;

public:
    /// Generate ephemeral inputs, queue ClientHello, and enter handshaking state.
    /// @throws err::LogicError If the protocol is not inactive.
    void start();
    /// Feed arbitrary coalesced or fragmented TCP bytes.
    /// Peer-controlled failures are converted to terminal state and an alert where transmission remains possible.
    /// @param data The next exact transport bytes.
    void feedTransport(mem::ConstByteSpan data) noexcept;
    /// Continue parsing retained input after the current checkpoint was serviced.
    /// Calling this method without a pending checkpoint has no effect.
    void resume() noexcept;
    /// Atomically accept one application message for record protection.
    /// @param data The application bytes, at most 2^14 bytes in this initial record-oriented core.
    /// @return Accepted, WouldBlock, or Closed without partial acceptance.
    [[nodiscard]] auto sendApplication(mem::ConstByteSpan data) -> NetworkSendStatus;
    /// Start graceful closure by queueing one close_notify alert.
    void close();
    /// Abort immediately and erase protocol state without an alert.
    void abort() noexcept;
    /// Fail an incomplete handshake after the external event-loop deadline.
    void timeout() noexcept;
    /// Signal that TCP reached EOF.
    /// EOF without peer close_notify is treated as truncation.
    void transportClosed() noexcept;

public: // queues
    /// Take the oldest complete encrypted transport record.
    /// @return One record, or no value if no output is pending.
    [[nodiscard]] auto takeTransportOutput() -> std::optional<mem::ByteBlock>;
    /// Take the oldest authenticated application-data fragment.
    /// @return One fragment, or no value if none is pending.
    [[nodiscard]] auto takeApplicationData() -> std::optional<mem::ByteBlock>;

public: // tests
    /// Test whether encrypted transport output is pending.
    [[nodiscard]] auto hasTransportOutput() const noexcept -> bool { return !_transportOutput.empty(); }
    /// Test whether authenticated application data is pending.
    [[nodiscard]] auto hasApplicationData() const noexcept -> bool { return !_applicationInput.empty(); }
    /// Test whether processing is paused at a protocol checkpoint.
    [[nodiscard]] auto hasCheckpoint() const noexcept -> bool {
        return _checkpoint != TlsClientProtocolCheckpoint::None;
    }

public: // accessors
    /// Get the current protocol lifecycle state.
    [[nodiscard]] auto state() const noexcept -> TlsClientProtocolState { return _state; }
    /// Get the terminal alert category, if any.
    [[nodiscard]] auto failureAlert() const noexcept -> const std::optional<TlsAlertDescription> & {
        return _failureAlert;
    }
    /// Get the terminal local diagnostic text.
    [[nodiscard]] auto failureDiagnostic() const noexcept -> const text::String & { return _failureDiagnostic; }
    /// Test whether terminal failure was caused by a received peer alert.
    [[nodiscard]] auto failureWasPeerAlert() const noexcept -> bool { return _failureWasPeerAlert; }
    /// Test whether terminal failure was caused by unauthenticated transport EOF.
    [[nodiscard]] auto failureWasTruncation() const noexcept -> bool { return _failureWasTruncation; }
    /// Test whether authenticated peer close_notify was received.
    [[nodiscard]] auto peerCloseNotifyReceived() const noexcept -> bool { return _peerCloseNotify; }
    /// Get the pending protocol checkpoint.
    [[nodiscard]] auto checkpoint() const noexcept -> TlsClientProtocolCheckpoint { return _checkpoint; }
    /// Get the negotiated cipher suite, if ServerHello completed.
    [[nodiscard]] auto cipherSuite() const noexcept -> const std::optional<cryptology::TlsCipherSuite> & {
        return _cipherSuite;
    }
    /// Get the selected ALPN identifier, or an empty string when none was negotiated.
    [[nodiscard]] auto negotiatedAlpn() const noexcept -> const text::String & { return _negotiatedAlpn; }
    /// Get the validated target-to-anchor certificate path.
    [[nodiscard]] auto validatedPath() const noexcept -> const util::List<cryptology::X509Certificate> & {
        return _validatedPath;
    }

private:
    /// Start with exact deterministic values supplied by test access.
    void startWithInputs(
        mem::ByteBlock random, mem::ByteBlock legacySessionId, cryptology::KeyAgreementPrivateKey privateKey);
    /// Process all complete buffered records.
    void processRecords();
    /// Process one complete TLSPlaintext or TLSCiphertext record.
    void processRecord(const mem::ByteBlock &record);
    /// Process plaintext record content before ServerHello installs keys.
    void processPlaintextRecord(uint8_t type, mem::ConstByteSpan content);
    /// Process authenticated TLSInnerPlaintext.
    void processProtectedRecord(const mem::ByteBlock &record);
    /// Process all complete buffered handshake messages.
    void processHandshakeMessages();
    /// Process retained handshake messages and records until input is exhausted or a checkpoint is reached.
    void processRetainedInput();
    /// Convert an exception from retained-input processing into terminal protocol state.
    void processRetainedInputNoThrow() noexcept;
    /// Verify aggregate receive capacity before retaining more input.
    void requireReceiveCapacity(std::size_t additionalBytes) const;
    /// Process one complete handshake message in the current state.
    void processHandshake(const mem::ByteBlock &message);
    /// Process ServerHello and install handshake record keys.
    void processServerHello(const mem::ByteBlock &message);
    /// Process EncryptedExtensions and optional ALPN selection.
    void processEncryptedExtensions(const mem::ByteBlock &message);
    /// Process a main-handshake CertificateRequest.
    void processCertificateRequest(const mem::ByteBlock &message);
    /// Parse and validate the server Certificate message.
    void processCertificate(const mem::ByteBlock &message);
    /// Verify the server CertificateVerify message.
    void processCertificateVerify(const mem::ByteBlock &message);
    /// Verify server Finished and queue the complete client second flight.
    void processServerFinished(const mem::ByteBlock &message);
    /// Process authenticated post-handshake handshake messages.
    void processPostHandshake(const mem::ByteBlock &message);
    /// Process one authenticated alert.
    void processAlert(mem::ConstByteSpan content);
    /// Process authenticated application data.
    void processApplicationData(mem::ConstByteSpan content);
    /// Iterate a bounded extension block with duplicate rejection.
    void forEachExtension(mem::ConstByteSpan extensions, const ExtensionFn &function) const;
    /// Queue a complete transport record within the configured output bound.
    [[nodiscard]] auto queueTransport(mem::ByteBlock record) -> bool;
    /// Queue a protected handshake message under current sending keys.
    void queueProtectedHandshake(mem::ConstByteSpan message);
    /// Queue one alert under the best available record protection.
    void queueAlert(TlsAlertDescription description) noexcept;
    /// Enter terminal failure, queueing an alert when requested, then erase secret state.
    void fail(TlsAlertDescription alert, text::String diagnostic, bool sendAlert = true) noexcept;
    /// Erase retained secret and parser state while preserving queued output and diagnostics.
    void eraseSecurityState() noexcept;
    /// Map certificate-validation failure to a TLS alert.
    [[nodiscard]] static auto certificateAlert(cryptology::X509CertificateValidationFailureCategory category) noexcept
        -> TlsAlertDescription;

private:
    static constexpr std::size_t cMaximumCertificates{16U};      ///< Fixed Certificate-list entry bound.
    static constexpr std::size_t cMaximumExtensions{64U};        ///< Fixed per-message extension-count bound.
    static constexpr std::size_t cMaximumIgnoredCcs{8U};         ///< Compatibility-mode CCS tolerance bound.
    static constexpr std::size_t cMaximumHandshakeRecords{256U}; ///< Handshake CPU/work record bound.

    TlsClientProtocolOptions _options;                           ///< Immutable protocol configuration.
    TlsClientHelloBuilder _clientHelloBuilder;                   ///< Validated deterministic ClientHello serializer.
    TlsClientProtocolState _state{TlsClientProtocolState::Inactive};            ///< Lifecycle state.
    TlsClientProtocolCheckpoint _checkpoint{TlsClientProtocolCheckpoint::None}; ///< Pending facade checkpoint.
    HandshakeStep _handshakeStep{HandshakeStep::ServerHello};                   ///< Expected handshake transition.
    TlsRecordStream _recordStream;                                              ///< Incremental TCP record deframer.
    TlsHandshakeStream _handshakeStream;                                        ///< Incremental handshake deframer.
    mem::ByteBlock _clientHello;                    ///< Complete ClientHello retained until suite hash selection.
    mem::ByteBlock _legacySessionId;                ///< Compatibility-mode ServerHello echo value.
    cryptology::KeyAgreementPrivateKey _privateKey; ///< Protected ephemeral X25519 private key.
    std::optional<cryptology::TlsCipherSuite> _cipherSuite;           ///< Negotiated supported suite.
    std::unique_ptr<cryptology::impl::Tls13Transcript> _transcript;   ///< Active handshake transcript.
    std::unique_ptr<cryptology::impl::Tls13KeySchedule> _keySchedule; ///< Active protected key schedule.
    cryptology::TlsRecordEncryptor _encryptor;                        ///< Current client write generation.
    cryptology::TlsRecordDecryptor _decryptor;                        ///< Current server write generation.
    cryptology::PublicKey _serverPublicKey;                           ///< Validated target key for CertificateVerify.
    util::List<cryptology::X509Certificate> _validatedPath;           ///< Accepted target-to-anchor path.
    text::String _negotiatedAlpn;                                     ///< Exact selected ALPN protocol.
    bool _certificateRequested{};                     ///< Whether an empty client Certificate is required.
    bool _peerCloseNotify{};                          ///< Whether peer orderly closure was authenticated.
    bool _localCloseNotify{};                         ///< Whether local orderly closure was queued.
    bool _failureWasPeerAlert{};                      ///< Whether a peer alert caused terminal failure.
    bool _failureWasTruncation{};                     ///< Whether bare TCP EOF caused terminal failure.
    std::size_t _ignoredCcsCount{};                   ///< Compatibility CCS work counter.
    std::size_t _handshakeRecordCount{};              ///< Handshake record work counter.
    std::deque<mem::ByteBlock> _transportOutput;      ///< Ordered complete records for TCP.
    std::size_t _transportOutputBytes{};              ///< Aggregate queued encrypted bytes.
    std::deque<mem::ByteBlock> _applicationInput;     ///< Ordered authenticated application fragments.
    std::size_t _applicationInputBytes{};             ///< Aggregate queued authenticated bytes.
    std::optional<TlsAlertDescription> _failureAlert; ///< Terminal alert category.
    text::String _failureDiagnostic;                  ///< Terminal local diagnostic.
};

}
