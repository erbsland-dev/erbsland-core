// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if !defined(__APPLE__) && !defined(__linux__)
#error "PosixTcpConnectionDevice.hpp is only available on macOS and Linux."
#endif

#include "TcpConnectionDevice.hpp"

#include "../../event/impl/EventLoopDriverRegistration.hpp"

#include <atomic>
#include <cstddef>
#include <functional>

namespace erbsland::network::impl {

/// POSIX non-blocking TCP connection device.
/// @tested{TcpSocketLiveTest}
class PosixTcpConnectionDevice final : public TcpConnectionDevice {
private:
    using RegistrationPtr = std::unique_ptr<event::impl::EventLoopDriverRegistration>;
    using NativeEventCallback = std::function<void(bool, bool, bool)>;
    using RegisterFn = std::function<RegistrationPtr(int, bool, bool, NativeEventCallback)>;

public:
    /// Create a POSIX TCP connection device.
    PosixTcpConnectionDevice(
        event::EventLoopDriverPtr driver, unit::ByteLength receiveChunkLimit, TcpConnectionDeviceCallbacks callbacks);
    ~PosixTcpConnectionDevice() override;

public: // implement TcpConnectionDevice
    void connect(IpEndpoint remoteEndpoint) override;
    void accept(TcpAcceptedSocketPtr socket) override;
    [[nodiscard]] auto canAccept(const TcpAcceptedSocket &socket) const noexcept -> bool override;
    [[nodiscard]] auto send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus override;
    void setReceiving(unit::ByteLength maximumBytes) override;
    void close() noexcept override;
    void abort() noexcept override;

private:
    /// Create a native readiness-registration function for an event-loop driver.
    [[nodiscard]] static auto createRegisterFn(event::EventLoopDriverPtr driver) -> RegisterFn;
    /// Configure a socket descriptor for non-blocking TCP operation.
    static void configureDescriptor(int descriptor);
    /// Update native readiness registration for the current connection state.
    void updateRegistration();
    /// Handle native socket readiness notifications.
    void handleReady(bool readable, bool writable, bool error);
    /// Complete a pending non-blocking connection.
    void finishConnect();
    /// Write retained outgoing data until it would block.
    [[nodiscard]] auto writePending() -> bool;
    /// Receive one available native socket chunk.
    void receiveOne();
    /// Query the connected socket's local endpoint.
    [[nodiscard]] auto queryLocalEndpoint() const -> IpEndpoint;
    /// Create context for a native socket error.
    [[nodiscard]] auto createContext(int errorCode, text::String title, text::String description) const
        -> NetworkErrorContext;
    /// Convert a native socket error code to a network error reason.
    [[nodiscard]] static auto errorReason(int errorCode) noexcept -> NetworkErrorReason;

private:
    RegisterFn _register;                      ///< Native driver registration function.
    TcpConnectionDeviceCallbacks _callbacks;   ///< Core callbacks.
    RegistrationPtr _registration;             ///< Current readiness registration.
    std::atomic<int> _descriptor{-1};          ///< Native socket descriptor.
    unit::ByteLength _receiveChunkLimit;       ///< Largest native receive chunk.
    std::optional<IpEndpoint> _localEndpoint;  ///< Local endpoint after connection.
    std::optional<IpEndpoint> _remoteEndpoint; ///< Remote endpoint after connection.
    mem::ByteBlock _sendData;                  ///< Retained partial send block.
    std::size_t _sendOffset{};                 ///< Bytes already written from retained block.
    bool _connecting{false};                   ///< Whether connect completion is pending.
    bool _connected{false};                    ///< Whether the socket is established.
    unit::ByteLength _maximumReceiveSize;      ///< Largest next native receive, or zero.
    bool _sendPending{false};                  ///< Whether output awaits write readiness.
};

}
