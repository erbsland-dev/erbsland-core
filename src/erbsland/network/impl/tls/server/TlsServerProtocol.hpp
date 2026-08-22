// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerProtocolCheckpoint.hpp"
#include "TlsServerProtocolOptions.hpp"
#include "TlsServerProtocolState.hpp"
#include "TlsServerProtocolTestAccess_fwd.hpp"

#include "../TlsAlertDescription.hpp"
#include "../TlsHandshakeStream.hpp"
#include "../TlsRecordStream.hpp"

#include "../../../../cryptology/impl/tls/Tls13KeySchedule.hpp"
#include "../../../../cryptology/impl/tls/Tls13Transcript.hpp"
#include "../../../../cryptology/keys/KeyAgreementPrivateKey.hpp"
#include "../../../../cryptology/keys/KeyAgreementPublicKey.hpp"
#include "../../../../cryptology/tls/TlsSignatureScheme.hpp"
#include "../../../../cryptology/tls_record/TlsRecordDecryptor.hpp"
#include "../../../../cryptology/tls_record/TlsRecordEncryptor.hpp"
#include "../../../../mem/ByteBlock.hpp"
#include "../../../../mem/ByteSpan.hpp"
#include "../../../../text/String.hpp"
#include "../../../../util/List.hpp"
#include "../../../HostName.hpp"
#include "../../../source/NetworkSendStatus.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace erbsland::network::impl {

/// Transport-independent authenticated TLS 1.3 server protocol core.
/// The initial implementation follows RFC 8446 Sections 4--7 and Appendix A.2/D.4 for a non-PSK X25519 server,
/// RFC 6066 Section 3 for strict DNS SNI, and RFC 7301 Section 3.2 for ALPN. It uses one configured certificate
/// identity and deliberately omits HelloRetryRequest, early data, resumption, and client authentication.
/// Specification: https://www.rfc-editor.org/rfc/rfc8446.html
/// @tested{TlsServerProtocolTest}
class TlsServerProtocol final {
    friend class TlsServerProtocolTestAccess;

private:
    /// Expected next authenticated handshake message.
    enum class HandshakeStep : uint8_t {
        ClientHello,    ///< Await the plaintext initial ClientHello.
        ClientFinished, ///< Await client Finished under client handshake keys.
        PostHandshake,  ///< Main handshake completed.
    };
    /// Borrowed extension data from one complete enclosing message.
    struct ExtensionView {
        uint16_t type{};           ///< ExtensionType wire value.
        mem::ConstByteSpan data{}; ///< Exact extension_data bytes.
    };
    /// Callback invoked once for each parsed extension.
    using ExtensionFn = std::function<void(const ExtensionView &)>;
    /// Locally built output flight before atomic queue commitment.
    struct OutputFlight {
        std::deque<mem::ByteBlock> records; ///< Complete ordered transport records.
        std::size_t byteCount{};            ///< Aggregate record bytes.
    };

public:
    /// Create an inactive TLS server protocol core.
    /// @param options Sole identity, ALPN/suite preferences, and queue bounds.
    explicit TlsServerProtocol(TlsServerProtocolOptions options);
    /// Erase all secret protocol state.
    ~TlsServerProtocol();

    // defaults/deletions
    TlsServerProtocol(const TlsServerProtocol &) = delete;
    TlsServerProtocol(TlsServerProtocol &&) = delete;
    auto operator=(const TlsServerProtocol &) -> TlsServerProtocol & = delete;
    auto operator=(TlsServerProtocol &&) -> TlsServerProtocol & = delete;

public:
    /// Enter the handshaking state and await ClientHello.
    /// @throws err::LogicError If the protocol is not inactive.
    void start();
    /// Feed arbitrary coalesced or fragmented TCP bytes.
    /// Peer-controlled failures are converted to terminal state and an alert where transmission remains possible.
    /// @param data The next exact transport bytes.
    void feedTransport(mem::ConstByteSpan data) noexcept;
    /// Accept the current policy checkpoint and continue protocol processing.
    /// ClientHello resumption creates secrets and queues the server flight; completion resumption releases retained
    /// authenticated application input.
    void resume() noexcept;
    /// Atomically accept one application message for record protection.
    /// @param data Application bytes, at most 2^14 bytes.
    /// @return Accepted, WouldBlock, or Closed without partial acceptance.
    [[nodiscard]] auto sendApplication(mem::ConstByteSpan data) -> NetworkSendStatus;
    /// Start graceful closure by queueing one close_notify alert.
    void close();
    /// Abort immediately and erase protocol state without an alert.
    void abort() noexcept;
    /// Fail an incomplete handshake after the external event-loop deadline.
    void timeout() noexcept;
    /// Signal that TCP reached EOF; EOF without close_notify is truncation.
    void transportClosed() noexcept;

public: // queues
    /// Take the oldest complete encrypted transport record.
    [[nodiscard]] auto takeTransportOutput() -> std::optional<mem::ByteBlock>;
    /// Take the oldest authenticated application-data fragment.
    [[nodiscard]] auto takeApplicationData() -> std::optional<mem::ByteBlock>;

public: // tests
    /// Test whether encrypted transport output is pending.
    [[nodiscard]] auto hasTransportOutput() const noexcept -> bool { return !_transportOutput.empty(); }
    /// Test whether authenticated application data is pending.
    [[nodiscard]] auto hasApplicationData() const noexcept -> bool { return !_applicationInput.empty(); }
    /// Test whether processing is paused at a protocol checkpoint.
    [[nodiscard]] auto hasCheckpoint() const noexcept -> bool {
        return _checkpoint != TlsServerProtocolCheckpoint::None;
    }

public: // accessors
    /// Get the current protocol lifecycle state.
    [[nodiscard]] auto state() const noexcept -> TlsServerProtocolState { return _state; }
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
    [[nodiscard]] auto checkpoint() const noexcept -> TlsServerProtocolCheckpoint { return _checkpoint; }
    /// Get the negotiated cipher suite, if ClientHello completed.
    [[nodiscard]] auto cipherSuite() const noexcept -> const std::optional<cryptology::TlsCipherSuite> & {
        return _cipherSuite;
    }
    /// Get the selected CertificateVerify signature scheme, if ClientHello completed.
    [[nodiscard]] auto signatureScheme() const noexcept -> const std::optional<cryptology::TlsSignatureScheme> & {
        return _signatureScheme;
    }
    /// Get the validated SNI host name, if offered.
    [[nodiscard]] auto serverName() const noexcept -> const std::optional<HostName> & { return _serverName; }
    /// Get the bounded client ALPN offer in wire order.
    [[nodiscard]] auto offeredAlpn() const noexcept -> const std::vector<text::String> & { return _offeredAlpn; }
    /// Get the selected ALPN identifier, or an empty string when none was negotiated.
    [[nodiscard]] auto negotiatedAlpn() const noexcept -> const text::String & { return _negotiatedAlpn; }
    /// Get the selected identity index, zero for the default and one-based for named mappings.
    [[nodiscard]] auto selectedIdentityIndex() const noexcept -> const std::optional<std::size_t> & {
        return _selectedIdentityIndex;
    }

private:
    /// Resume ClientHello with deterministic server inputs supplied by test access.
    void resumeWithInputs(mem::ByteBlock random, cryptology::KeyAgreementPrivateKey privateKey);
    /// Build and atomically queue the complete first server flight.
    void buildServerFlight(mem::ByteBlock random, cryptology::KeyAgreementPrivateKey privateKey);
    /// Process all complete buffered records.
    void processRecords();
    /// Process one complete TLSPlaintext or TLSCiphertext record.
    void processRecord(const mem::ByteBlock &record);
    /// Process plaintext content before and during the handshake.
    void processPlaintextRecord(uint8_t type, mem::ConstByteSpan content);
    /// Process authenticated TLSInnerPlaintext.
    void processProtectedRecord(const mem::ByteBlock &record);
    /// Process all complete buffered handshake messages.
    void processHandshakeMessages();
    /// Process retained messages and records until input is exhausted or a checkpoint is reached.
    void processRetainedInput();
    /// Convert an exception from retained-input processing into terminal state.
    void processRetainedInputNoThrow() noexcept;
    /// Verify aggregate receive capacity before retaining more input.
    void requireReceiveCapacity(std::size_t additionalBytes) const;
    /// Process one complete handshake message in the current state.
    void processHandshake(const mem::ByteBlock &message);
    /// Parse and negotiate the initial ClientHello without creating server secrets.
    void processClientHello(const mem::ByteBlock &message);
    /// Verify client Finished and install client application read keys.
    void processClientFinished(const mem::ByteBlock &message);
    /// Process authenticated post-handshake messages.
    void processPostHandshake(const mem::ByteBlock &message);
    /// Process one authenticated or plaintext alert.
    void processAlert(mem::ConstByteSpan content);
    /// Process authenticated application data.
    void processApplicationData(mem::ConstByteSpan content);
    /// Iterate a bounded extension block with duplicate rejection.
    void forEachExtension(mem::ConstByteSpan extensions, const ExtensionFn &function) const;
    /// Add one record to a temporary flight within the configured aggregate output bound.
    void appendFlight(OutputFlight &flight, mem::ByteBlock record) const;
    /// Protect and fragment a complete handshake message into a temporary flight.
    void appendProtectedHandshake(OutputFlight &flight, mem::ConstByteSpan message);
    /// Commit a complete preflighted output flight.
    void commitFlight(OutputFlight &&flight);
    /// Queue a complete transport record within the configured output bound.
    [[nodiscard]] auto queueTransport(mem::ByteBlock record) -> bool;
    /// Queue one protected post-handshake message.
    void queueProtectedHandshake(mem::ConstByteSpan message);
    /// Queue one alert under the best available record protection.
    void queueAlert(TlsAlertDescription description) noexcept;
    /// Enter terminal failure, optionally queueing an alert, then erase secret state.
    void fail(TlsAlertDescription alert, text::String diagnostic, bool sendAlert = true) noexcept;
    /// Erase retained secret and parser state while preserving queued output and diagnostics.
    void eraseSecurityState() noexcept;

private:
    static constexpr std::size_t cMaximumExtensions{64U};            ///< Per-message extension-count bound.
    static constexpr std::size_t cMaximumHandshakeRecords{256U};     ///< Handshake CPU/work record bound.
    static constexpr std::size_t cMaximumIgnoredCcs{8U};             ///< Compatibility CCS tolerance bound.
    static constexpr std::size_t cMaximumListEntries{256U};          ///< Suite, scheme, and group work bound.
    static constexpr std::size_t cMaximumKeyShares{64U};             ///< Client key-share work bound.
    static constexpr std::size_t cMaximumAlpnProtocols{64U};         ///< Client ALPN protocol-count bound.
    static constexpr std::size_t cMaximumAlpnBytes{4096U};           ///< Client ALPN aggregate encoded bound.

    TlsServerProtocolOptions _options;                               ///< Immutable protocol configuration.
    TlsServerProtocolState _state{TlsServerProtocolState::Inactive}; ///< Lifecycle state.
    TlsServerProtocolCheckpoint _checkpoint{TlsServerProtocolCheckpoint::None}; ///< Pending facade checkpoint.
    HandshakeStep _handshakeStep{HandshakeStep::ClientHello};                   ///< Expected handshake transition.
    TlsRecordStream _recordStream;                                              ///< Incremental TCP record deframer.
    TlsHandshakeStream _handshakeStream;                                        ///< Incremental handshake deframer.
    mem::ByteBlock _clientHello;                            ///< Exact ClientHello retained until transcript creation.
    mem::ByteBlock _legacySessionId;                        ///< Client compatibility session identifier.
    cryptology::KeyAgreementPublicKey _clientKey;           ///< Validated public X25519 ClientHello key share.
    cryptology::KeyAgreementPrivateKey _privateKey;         ///< Protected ephemeral server X25519 key.
    std::optional<cryptology::TlsCipherSuite> _cipherSuite; ///< Negotiated suite.
    std::optional<cryptology::TlsSignatureScheme> _signatureScheme;   ///< Selected CertificateVerify scheme.
    cryptology::TlsServerIdentityConstPtr _selectedIdentity;          ///< Selected default or exact-SNI identity.
    std::optional<std::size_t> _selectedIdentityIndex;                ///< Default zero or named one-based index.
    std::unique_ptr<cryptology::impl::Tls13Transcript> _transcript;   ///< Active handshake transcript.
    std::unique_ptr<cryptology::impl::Tls13KeySchedule> _keySchedule; ///< Active protected key schedule.
    cryptology::TlsRecordEncryptor _encryptor;                        ///< Current server write generation.
    cryptology::TlsRecordDecryptor _decryptor;                        ///< Current client read generation.
    std::optional<HostName> _serverName;                              ///< Strict parsed DNS SNI.
    std::vector<text::String> _offeredAlpn;                           ///< Bounded ALPN offer in client order.
    text::String _negotiatedAlpn;                                     ///< Selected ALPN protocol.
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
