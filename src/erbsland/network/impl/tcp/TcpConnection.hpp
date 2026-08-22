// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnection_fwd.hpp"
#include "TcpConnectionDevice.hpp"
#include "TcpConnectionDeviceCreateFn.hpp"
#include "TcpConnectionEventEditor.hpp"

#include "../host/HostLookup_fwd.hpp"
#include "../host/HostResolver_fwd.hpp"

#include "../../../time/TimePoint.hpp"
#include "../../../util/List.hpp"
#include "../../source/ConnectionCloseContext.hpp"
#include "../../source/ConnectionQuota.hpp"
#include "../../tcp/TcpConnection.hpp"

#include <atomic>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// Native TCP connection source implementation.
/// @tested{TcpConnectionTest TcpSocketLiveTest}
class TcpConnection final : public network::TcpConnection {
    friend class TcpConnectionEventEditor;

public:
    /// Create an inactive TCP connection source.
    TcpConnection(
        event::EventsPtr ownerEvents,
        event::EventLoopDriverPtr driver,
        HostResolverPtr resolver,
        TcpConnectionDeviceCreateFn createDevice = TcpConnectionDevice::create);
    ~TcpConnection() override;

public: // implement network::TcpConnection
    [[nodiscard]] auto localEndpoint() const -> std::optional<IpEndpoint> override;
    [[nodiscard]] auto remoteEndpoint() const -> std::optional<IpEndpoint> override;
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits override;
    [[nodiscard]] auto state() const noexcept -> ConnectionState override;
    void connect(HostEndpoint remoteEndpoint, TcpConnectOptions options) override;
    void accept(network::TcpConnectionRequestPtr request, TcpAcceptOptions options) override;
    [[nodiscard]] auto send(const mem::ByteBlock &data) -> NetworkSendStatus override;
    void pauseReceiving() override;
    void resumeReceiving() override;
    void close() override;
    void abort() noexcept override;
    [[nodiscard]] auto events() -> network::TcpConnectionEventEditor & override;

private:
    /// Create native-device callbacks bound to `generation`.
    [[nodiscard]] auto createCallbacks(std::uint64_t generation) -> TcpConnectionDeviceCallbacks;
    /// Start resolving the requested host for `generation`.
    void startLookup(std::uint64_t generation);
    /// Deliver the resolved host addresses for `generation`.
    void deliverResolved(std::uint64_t generation, const util::List<IpAddress> &addresses);
    /// Start the next resolved endpoint for `generation`.
    void startNextEndpoint(std::uint64_t generation);
    /// Post a successful connection notification.
    void postConnected(std::uint64_t generation, IpEndpoint localEndpoint, IpEndpoint remoteEndpoint);
    /// Post received input data.
    void postData(std::uint64_t generation, mem::ByteBlock data);
    /// Post a completed-send notification.
    void postSendCompleted(std::uint64_t generation);
    /// Post notification that the peer closed its output.
    void postRemoteClosed(std::uint64_t generation);
    /// Post an operational error.
    void postError(std::uint64_t generation, NetworkErrorContext context);
    /// Deliver a successful connection notification.
    void deliverConnected(std::uint64_t generation, IpEndpoint localEndpoint, IpEndpoint remoteEndpoint);
    /// Deliver queued input data.
    void deliverData(std::uint64_t generation);
    /// Deliver a completed-send notification.
    void deliverSendCompleted(std::uint64_t generation);
    /// Deliver notification that the peer closed its output.
    void deliverRemoteClosed(std::uint64_t generation);
    /// Deliver an operational error.
    void deliverError(std::uint64_t generation, NetworkErrorContext context);
    /// Deliver the connection timeout for `generation`.
    void deliverTimeout(std::uint64_t generation);
    /// Submit queued output to the native device.
    void flushOutput();
    /// Complete bookkeeping for the accepted output block.
    void finishAcceptedBlock();
    /// Notify the client when output capacity becomes available.
    void notifyWritableIfReady();
    /// Update native receiving according to the current state.
    void updateReceiving();
    /// Schedule delivery of queued input data.
    void scheduleDataDelivery();
    /// Finish normal closure when all work has completed.
    void finishCloseIfReady();
    /// Finish normal closure with `origin`.
    void finishClosed(ConnectionCloseOrigin origin);
    /// Finish connection failure with `context`.
    void finishFailed(NetworkErrorContext context);
    /// Post normal closure followed by final notification.
    void postClosedAndFinal(std::uint64_t generation, ConnectionCloseContext context);
    /// Post an error followed by final notification.
    void postErrorAndFinal(std::uint64_t generation, NetworkErrorContext context);
    /// Post the final lifecycle notification.
    void postFinal(std::uint64_t generation);
    /// Release the native device, optionally aborting it first.
    void cleanupDevice(bool abortDevice) noexcept;
    /// Validate that the requested buffer limits are usable.
    void validateLimits(const SocketBufferLimits &limits) const;
    /// Test whether `generation` is still current.
    [[nodiscard]] auto isCurrent(std::uint64_t generation) const noexcept -> bool;
    /// Test whether connection establishment is active.
    [[nodiscard]] auto isStarting() const noexcept -> bool;

private:
    event::EventLoopDriverPtr _driver;                      ///< Owner loop's native driver.
    HostResolverPtr _resolver;                              ///< Native host resolver.
    TcpConnectionDeviceCreateFn _createDevice;              ///< Injectable native-device factory.
    std::unique_ptr<TcpConnectionEventEditor> _eventEditor; ///< Stable source-owned editor.
    mutable std::recursive_mutex _lifecycleMutex;           ///< Serializes lifecycle changes with thread-safe abort.
    mutable std::mutex _dataMutex;                          ///< Protects endpoints and the native device.
    TcpConnectionDevicePtr _device;                         ///< Current native connection device.
    std::shared_ptr<HostLookup> _lookup;                    ///< Current named-host lookup.
    std::optional<HostEndpoint> _requestedEndpoint;         ///< Requested outgoing endpoint.
    std::optional<IpEndpoint> _localEndpoint;               ///< Established local endpoint.
    std::optional<IpEndpoint> _remoteEndpoint;              ///< Established remote endpoint.
    TcpConnectOptions _connectOptions;                      ///< Captured outgoing options.
    SocketBufferLimits _bufferLimits;                       ///< Captured stream limits.
    network::ConnectionQuotaLease _quotaLease;              ///< Accepted-connection lifetime lease.
    time::TimePoint _deadline;                              ///< Overall outgoing deadline.
    util::List<IpEndpoint> _resolvedEndpoints;              ///< Ordered outgoing candidates.
    std::size_t _nextEndpoint{};                            ///< Next candidate index.
    std::optional<NetworkErrorContext> _lastConnectError;   ///< Most recent candidate failure.
    std::deque<mem::ByteBlock> _sendQueue;                  ///< Accepted output blocks.
    unit::ByteLength _sendQueueSize;                        ///< Queued output bytes.
    unit::ByteLength _blockedCharge;                        ///< Largest rejected output block.
    std::deque<mem::ByteBlock> _receiveQueue;               ///< Received input awaiting delivery.
    unit::ByteLength _receiveQueueSize;                     ///< Queued input bytes.
    std::atomic<ConnectionState> _state{ConnectionState::Inactive}; ///< Lifecycle state.
    std::atomic<std::uint64_t> _generation{0U};                     ///< Native operation generation.
    std::atomic<bool> _started{false};                              ///< Whether this source was consumed.
    std::atomic<bool> _finalPosted{false};                          ///< Whether final delivery was posted.
    bool _receivePaused{false};                                     ///< Whether application delivery is paused.
    bool _receiveDeliveryPosted{false};                             ///< Whether input delivery is queued.
    bool _sendPending{false};                                       ///< Whether the device retained one block.
    std::optional<ConnectionCloseOrigin> _closeOrigin;              ///< First normal-close initiator.
    TcpHostResolvedFn _onHostResolved;                              ///< Resolution handler.
    NetworkEventFn _onConnected;                                    ///< Connected handler.
    NetworkDataFn _onData;                                          ///< Input handler.
    NetworkEventFn _onWritable;                                     ///< Renewed-capacity handler.
    ConnectionCloseFn _onClosed;                                    ///< Normal-close handler.
    NetworkErrorFn _onError;                                        ///< Operational-error handler.
    NetworkEventFn _onFinal;                                        ///< Final handler.
};

}
