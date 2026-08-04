// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientConnection_fwd.hpp"
#include "TlsClientConnectionEventEditor.hpp"
#include "TlsClientProtocol.hpp"
#include "TlsClientProtocolStartFn.hpp"

#include "../source/NetworkErrorContext.hpp"
#include "../tcp/TcpConnection.hpp"
#include "../tls/TlsClientConnection.hpp"
#include "../tls/TlsClientConnectionCloseContext.hpp"

#include "../../cryptology/tls/TlsConfiguration.hpp"
#include "../../time/TimePoint.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// Built-in event-driven TLS 1.3 client facade over a TCP connection.
/// @tested{TlsClientConnectionTest}
class TlsClientConnection final : public network::TlsClientConnection {
    friend class TlsClientConnectionEventEditor;

public:
    /// Create a facade over one inactive TCP connection.
    /// The optional start function is an internal deterministic-test injection point.
    TlsClientConnection(
        event::EventsPtr ownerEvents,
        network::TcpConnectionPtr tcpConnection,
        TlsClientProtocolStartFn startProtocol = {});
    ~TlsClientConnection() override;

public: // implement network::TlsClientConnection
    [[nodiscard]] auto requestedConfigurationLabel() const -> text::String override;
    [[nodiscard]] auto matchedConfigurationLabel() const -> text::String override;
    [[nodiscard]] auto requestedEndpoint() const -> std::optional<HostEndpoint> override;
    [[nodiscard]] auto localEndpoint() const -> std::optional<IpEndpoint> override;
    [[nodiscard]] auto remoteEndpoint() const -> std::optional<IpEndpoint> override;
    [[nodiscard]] auto cipherSuite() const -> std::optional<cryptology::TlsCipherSuite> override;
    [[nodiscard]] auto negotiatedAlpn() const -> mem::ByteBlock override;
    [[nodiscard]] auto peerCertificatePath() const -> util::List<cryptology::X509Certificate> override;
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits override;
    [[nodiscard]] auto state() const noexcept -> TlsClientConnectionState override;
    void connect(HostEndpoint endpoint, TlsClientConnectOptions options) override;
    [[nodiscard]] auto send(const mem::ByteBlock &data) -> NetworkSendStatus override;
    void pauseReceiving() override;
    void resumeReceiving() override;
    void close() override;
    void abort() noexcept override;
    [[nodiscard]] auto events() -> network::TlsClientConnectionEventEditor & override;

private:
    /// Install the internal TCP callbacks for the current facade instance.
    void configureTcpEvents();
    /// Validate every synchronous operational option.
    static void validateOptions(const HostEndpoint &endpoint, const TlsClientConnectOptions &options);
    /// Deliver TCP resolution candidates.
    void handleHostResolved(const util::List<IpEndpoint> &endpoints);
    /// Start TLS after the TCP transport checkpoint.
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
    void enterClosing(TlsClientConnectionCloseOrigin origin);
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
    /// Test whether callbacks may continue protocol transitions.
    [[nodiscard]] auto canContinue() const noexcept -> bool;

private:
    network::TcpConnectionPtr _tcpConnection;                     ///< Owned transport source.
    TlsClientProtocolStartFn _startProtocol;                      ///< Normal or deterministic protocol start.
    std::unique_ptr<TlsClientConnectionEventEditor> _eventEditor; ///< Stable public event editor.
    std::unique_ptr<TlsClientProtocol> _protocol;                 ///< Active protocol core.
    cryptology::TlsConfigurationConstPtr _configuration;          ///< Captured immutable registry entry.
    mutable std::recursive_mutex _mutex;                          ///< Lifecycle and metadata serialization.
    std::optional<HostEndpoint> _requestedEndpoint;               ///< Original endpoint.
    text::String _requestedConfigurationLabel;                    ///< Original label.
    text::String _matchedConfigurationLabel;                      ///< Selected exact label.
    TlsClientConnectOptions _options;                             ///< Captured operational options.
    std::optional<mem::ByteBlock> _pendingTransportRecord;        ///< One TCP-rejected complete record.
    std::optional<TlsClientConnectionCloseOrigin> _closeOrigin;   ///< First close-notify initiator.
    time::TimePoint _idleDeadline;                                ///< Updated absolute application-idle deadline.
    std::atomic<TlsClientConnectionState> _state{TlsClientConnectionState::Inactive}; ///< Lifecycle state.
    std::atomic<bool> _started{false};                                                ///< One-shot consumption guard.
    std::atomic<bool> _abortRequested{false};  ///< Cross-thread immediate abort request.
    std::atomic<bool> _finalized{false};       ///< Exactly-once final guard.
    std::uint64_t _handshakeTimerGeneration{}; ///< Invalidates stale handshake timers.
    std::uint64_t _idleTimerGeneration{};      ///< Invalidates stale idle timers.
    std::uint64_t _closeTimerGeneration{};     ///< Invalidates stale close timers.
    bool _idleTimerPending{};                  ///< At most one pending idle timer.
    bool _receivingPaused{};                   ///< Application delivery pause.
    bool _applicationBlocked{};                ///< Whether a writable transition is owed.
    bool _transportClosing{};                  ///< Whether TCP close was requested.
    bool _hostResolved{};                      ///< Whether resolution checkpoint completed.
    TcpHostResolvedFn _onHostResolved;         ///< Host-resolution handler.
    NetworkEventFn _onTransportConnected;      ///< TCP connection handler.
    NetworkEventFn _onPeerHello;               ///< Negotiated-parameters checkpoint.
    NetworkEventFn _onPeerAuthenticated;       ///< Authenticated-peer checkpoint.
    NetworkEventFn _onHandshakeCompleted;      ///< Completed-handshake checkpoint.
    NetworkDataFn _onData;                     ///< Authenticated application-data handler.
    NetworkEventFn _onWritable;                ///< Renewed application capacity handler.
    TlsClientConnectionCloseFn _onClosed;      ///< Orderly TLS closure handler.
    NetworkErrorFn _onError;                   ///< Operational failure handler.
    NetworkEventFn _onFinal;                   ///< Exactly-once final handler.
};

}
