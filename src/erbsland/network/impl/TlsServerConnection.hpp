// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerConnectionEventEditor.hpp"
#include "TlsServerProtocol.hpp"

#include "../source/NetworkErrorContext.hpp"
#include "../tcp/TcpConnection.hpp"
#include "../tls/TlsServerConnection.hpp"
#include "../tls/TlsServerConnectionCloseContext.hpp"

#include "../../cryptology/tls/TlsConfiguration.hpp"
#include "../../time/TimePoint.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// Built-in event-driven TLS 1.3 server facade over an accepted TCP connection.
/// @tested{TlsServerConnectionTest}
class TlsServerConnection final : public network::TlsServerConnection {
    friend class TlsServerConnectionEventEditor;

private:
    /// One resolved, immutable identity configuration snapshot.
    struct ResolvedIdentity final {
        text::String requestedLabel;                        ///< Requested registry label.
        text::String matchedLabel;                          ///< Exact fallback result.
        cryptology::TlsConfigurationConstPtr configuration; ///< Captured immutable configuration.
    };
    /// Optional deterministic protocol-start injection point.
    using StartProtocolFn = std::function<void(TlsServerProtocol &)>;

public:
    /// Create a facade over one inactive TCP connection.
    /// The optional start function is an internal deterministic-test injection point.
    TlsServerConnection(
        event::EventsPtr ownerEvents, network::TcpConnectionPtr tcpConnection, StartProtocolFn startProtocol = {});
    ~TlsServerConnection() override;

public: // implement network::TlsServerConnection
    [[nodiscard]] auto requestedConfigurationLabel() const -> std::optional<text::String> override;
    [[nodiscard]] auto matchedConfigurationLabel() const -> std::optional<text::String> override;
    [[nodiscard]] auto localEndpoint() const -> std::optional<IpEndpoint> override;
    [[nodiscard]] auto remoteEndpoint() const -> std::optional<IpEndpoint> override;
    [[nodiscard]] auto serverName() const -> std::optional<HostName> override;
    [[nodiscard]] auto offeredAlpn() const -> std::vector<mem::ByteBlock> override;
    [[nodiscard]] auto negotiatedAlpn() const -> mem::ByteBlock override;
    [[nodiscard]] auto cipherSuite() const -> std::optional<cryptology::TlsCipherSuite> override;
    [[nodiscard]] auto signatureScheme() const -> std::optional<cryptology::TlsSignatureScheme> override;
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits override;
    [[nodiscard]] auto state() const noexcept -> TlsServerConnectionState override;
    void accept(network::TcpConnectionRequestPtr request, TlsServerAcceptOptions options) override;
    [[nodiscard]] auto send(const mem::ByteBlock &data) -> NetworkSendStatus override;
    void pauseReceiving() override;
    void resumeReceiving() override;
    void close() override;
    void abort() noexcept override;
    [[nodiscard]] auto events() -> network::TlsServerConnectionEventEditor & override;

private:
    /// Install the internal TCP callbacks for the current facade instance.
    void configureTcpEvents();
    /// Validate every synchronous operational option.
    static void validateOptions(const TlsServerAcceptOptions &options);
    /// Resolve and snapshot the required default and exact-name identities.
    [[nodiscard]] static auto resolveIdentities(const TlsServerAcceptOptions &options) -> std::vector<ResolvedIdentity>;
    /// Start TLS after the accepted TCP transport checkpoint.
    void handleTransportConnected();
    /// Feed one TCP input block into TLS.
    void handleTransportData(mem::ByteBlock data);
    /// Retry a retained encrypted record and report application writability.
    void handleTransportWritable();
    /// Translate orderly or truncated TCP closure.
    void handleTransportClosed();
    /// Translate an asynchronous TCP failure.
    void handleTransportError(const NetworkErrorContext &context);
    /// Finish deferred explicit abort after TCP finalization.
    void handleTransportFinal();
    /// Service all pending protocol checkpoints in order.
    void serviceCheckpoints();
    /// Move complete TLS records atomically into TCP.
    void pumpTransportOutput();
    /// Deliver queued authenticated application blocks while receiving is enabled.
    void drainApplicationInput();
    /// Translate current protocol state after processing.
    void assessProtocolState();
    /// Start graceful TCP closure after TLS closure completes.
    void finishTransportClosure();
    /// Record non-empty authenticated application activity.
    void recordApplicationActivity();
    /// Ensure one idle timer is pending.
    void ensureIdleTimer();
    /// Deliver one generation-checked handshake timeout.
    void handleHandshakeTimeout(std::uint64_t generation, time::TimePoint deadline);
    /// Deliver or reschedule one generation-checked idle timeout.
    void handleIdleTimeout(std::uint64_t generation);
    /// Deliver one generation-checked graceful-close timeout.
    void handleCloseTimeout(std::uint64_t generation, time::TimePoint deadline);
    /// Enter closing state and schedule its independent deadline once.
    void enterClosing(TlsServerConnectionCloseOrigin origin);
    /// Translate a protocol failure into structured network context.
    [[nodiscard]] auto protocolErrorContext() const -> NetworkErrorContext;
    /// Attach known endpoints and the current phase to a context.
    [[nodiscard]] auto enrichTransportError(NetworkErrorContext context) const -> NetworkErrorContext;
    /// Finish one operational failure exactly once.
    void finishFailed(NetworkErrorContext context);
    /// Finish orderly closure exactly once.
    void finishClosed();
    /// Emit final once.
    void finishFinal();
    /// Post finalization for an abort that occurred before TCP startup.
    void postAbortFinal();
    /// Reject an unconsumed request and post an accepting-phase failure.
    void postAdmissionFailure(NetworkErrorContext context);
    /// Test whether callbacks may continue protocol transitions.
    [[nodiscard]] auto canContinue() const noexcept -> bool;

private:
    network::TcpConnectionPtr _tcpConnection;                     ///< Owned accepted transport.
    StartProtocolFn _startProtocol;                               ///< Normal or deterministic protocol start.
    std::unique_ptr<TlsServerConnectionEventEditor> _eventEditor; ///< Stable public event editor.
    std::unique_ptr<TlsServerProtocol> _protocol;                 ///< Active server protocol core.
    std::vector<ResolvedIdentity> _identities;                    ///< Default then exact-name snapshots.
    TlsServerAcceptOptions _options{ConnectionQuota::create(unit::ItemCount{1U})}; ///< Captured options.
    ConnectionQuotaLease _handshakeLease;                       ///< Incomplete-handshake reservation.
    mutable std::recursive_mutex _mutex;                        ///< Lifecycle and metadata serialization.
    std::optional<mem::ByteBlock> _pendingTransportRecord;      ///< One TCP-rejected complete record.
    std::optional<TlsServerConnectionCloseOrigin> _closeOrigin; ///< First close-notify initiator.
    time::TimePoint _idleDeadline;                              ///< Absolute application idle deadline.
    std::atomic<TlsServerConnectionState> _state{TlsServerConnectionState::Inactive};
    std::atomic<bool> _started{false};
    std::atomic<bool> _abortRequested{false};
    std::atomic<bool> _finalized{false};
    std::uint64_t _handshakeTimerGeneration{};
    std::uint64_t _idleTimerGeneration{};
    std::uint64_t _closeTimerGeneration{};
    bool _idleTimerPending{};
    bool _receivingPaused{};
    bool _applicationBlocked{};
    bool _transportClosing{};
    NetworkEventFn _onTransportConnected;
    NetworkEventFn _onClientHello;
    NetworkEventFn _onHandshakeCompleted;
    NetworkDataFn _onData;
    NetworkEventFn _onWritable;
    TlsServerConnectionCloseFn _onClosed;
    NetworkErrorFn _onError;
    NetworkEventFn _onFinal;
};

}
