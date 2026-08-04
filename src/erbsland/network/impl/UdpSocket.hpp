// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpSocket_fwd.hpp"
#include "UdpSocketDevice.hpp"
#include "UdpSocketDeviceCreateFn.hpp"
#include "UdpSocketEventEditor.hpp"

#include "../udp/UdpSocket.hpp"

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// Native UDP socket source implementation.
/// @tested{UdpSocketTest UdpSocketLiveTest}
class UdpSocket final : public network::UdpSocket {
    friend class UdpSocketEventEditor;

public:
    /// Create an inactive UDP socket source.
    /// @param ownerEvents The owner event loop.
    /// @param driver The owner loop's native driver.
    /// @param createDevice The platform device factory.
    UdpSocket(
        event::EventsPtr ownerEvents,
        event::EventLoopDriverPtr driver,
        UdpSocketDeviceCreateFn createDevice = UdpSocketDevice::createUdpSocketDevice);
    ~UdpSocket() override;

public: // implement network::UdpSocket
    [[nodiscard]] auto localEndpoint() const -> std::optional<IpEndpoint> override;
    [[nodiscard]] auto state() const noexcept -> NetworkSourceState override;
    void start(IpEndpoint localEndpoint, UdpSocketOptions options) override;
    [[nodiscard]] auto send(const UdpDatagram &datagram) -> NetworkSendStatus override;
    void pauseReceiving() override;
    void resumeReceiving() override;
    void close() override;
    void abort() noexcept override;
    [[nodiscard]] auto events() -> network::UdpSocketEventEditor & override;

private:
    /// Create native-device callbacks for an operation generation.
    [[nodiscard]] auto createCallbacks(std::uint64_t generation) -> UdpSocketDeviceCallbacks;
    /// Queue delivery of a successful bind event.
    void postBound(std::uint64_t generation);
    /// Queue delivery of an incoming datagram.
    void postDatagram(std::uint64_t generation, UdpDatagram datagram);
    /// Queue delivery of a dropped incoming datagram.
    void postDatagramDropped(std::uint64_t generation, UdpDatagramDropContext context);
    /// Queue delivery of a completed send event.
    void postSendCompleted(std::uint64_t generation);
    /// Queue delivery of renewed native write capacity.
    void postWritable(std::uint64_t generation);
    /// Queue delivery of a closed event.
    void postClosed(std::uint64_t generation);
    /// Queue delivery of an operational error.
    void postError(std::uint64_t generation, NetworkErrorContext context);
    /// Deliver a successful bind event for a current generation.
    void deliverBound(std::uint64_t generation);
    /// Deliver an incoming datagram for a current generation.
    void deliverDatagram(std::uint64_t generation, UdpDatagram datagram);
    /// Deliver a dropped-datagram event for a current generation.
    void deliverDatagramDropped(std::uint64_t generation, UdpDatagramDropContext context);
    /// Deliver a completed-send event for a current generation.
    void deliverSendCompleted(std::uint64_t generation);
    /// Deliver renewed native write capacity for a current generation.
    void deliverNativeWritable(std::uint64_t generation);
    /// Deliver a closed event for a current generation.
    void deliverClosed(std::uint64_t generation);
    /// Deliver an operational error for a current generation.
    void deliverError(std::uint64_t generation, NetworkErrorContext context);
    /// Submit queued datagrams to the native device.
    void flushOutput();
    /// Complete processing for the accepted input datagram.
    void finishAcceptedDatagram();
    /// Notify the owner when output capacity becomes available.
    void notifyWritableIfReady();
    /// Finish a graceful socket close.
    void finishClose();
    /// Release the native device.
    void cleanupDevice() noexcept;
    /// Validate a request to start the socket.
    void validateStart(const IpEndpoint &localEndpoint, const UdpSocketOptions &options) const;
    /// Validate a datagram before it enters the output queue.
    void validateDatagram(const UdpDatagram &datagram) const;
    /// Calculate the output-queue charge of a datagram.
    [[nodiscard]] static auto queueCharge(const UdpDatagram &datagram) noexcept -> unit::ByteLength;
    /// Test whether a native callback belongs to the current generation.
    [[nodiscard]] auto isCurrent(std::uint64_t generation) const noexcept -> bool;

private:
    event::EventLoopDriverPtr _driver;                  ///< Owner loop's native driver.
    UdpSocketDeviceCreateFn _createDevice;              ///< Platform device factory.
    std::unique_ptr<UdpSocketEventEditor> _eventEditor; ///< Stable source-owned editor.
    mutable std::mutex _dataMutex;                      ///< Protects cross-thread visible data and device access.
    UdpSocketDevicePtr _device;                         ///< Bound native UDP device.
    std::optional<IpEndpoint> _localEndpoint;           ///< Actual bound endpoint.
    UdpSocketOptions _options;                          ///< Captured socket options.
    std::deque<UdpDatagram> _sendQueue;                 ///< Accepted datagrams awaiting completion.
    unit::ByteLength _sendQueueSize;                    ///< Current queued output charge.
    unit::ByteLength _blockedCharge;                    ///< Largest rejected charge awaiting writable capacity.
    std::atomic<NetworkSourceState> _state{NetworkSourceState::Inactive}; ///< Public lifecycle state.
    std::atomic<std::uint64_t> _generation{0U};                           ///< Current native operation generation.
    std::atomic<bool> _started{false};                                    ///< Whether this lifetime was consumed.
    bool _receivePaused{false};                                           ///< Whether receive delivery is paused.
    bool _sendPending{false};                                             ///< Whether one native send is pending.
    bool _nativeWriteBlocked{false};      ///< Whether POSIX output awaits write readiness.
    NetworkEventFn _onBound;              ///< Successful bind handler.
    UdpDatagramFn _onDatagram;            ///< Incoming datagram handler.
    UdpDatagramDropFn _onDatagramDropped; ///< Local datagram drop handler.
    NetworkEventFn _onWritable;           ///< Renewed send-capacity handler.
    NetworkEventFn _onClosed;             ///< Graceful close handler.
    NetworkErrorFn _onError;              ///< Operational error handler.
};

}
