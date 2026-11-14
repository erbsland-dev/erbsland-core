// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if !defined(__APPLE__) && !defined(__linux__)
#error "PosixTcpConnectionDevice.hpp is only available on macOS and Linux."
#endif

#include "PosixTcpConnectionDevice_fwd.hpp"

#include "../TcpConnectionDevice.hpp"

#include "../../../../event/impl/EventLoopDriverRegistration.hpp"
#include "../../platform/PosixSocketDevice.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// POSIX non-blocking TCP connection device.
/// Create through create(); callers retain shared ownership throughout each operation.
/// @tested{TcpSocketLiveTest PosixSocketLifecycleTest}
class PosixTcpConnectionDevice final : public TcpConnectionDevice,
                                       private PosixSocketDevice,
                                       public std::enable_shared_from_this<PosixTcpConnectionDevice> {
    struct PrivateTag {};

public:
    /// Owns a native readiness registration.
    using RegistrationPtr = std::unique_ptr<event::impl::EventLoopDriverRegistration>;
    /// Reports native readiness.
    using NativeEventCallback = std::function<void(bool, bool, bool)>;
    /// Creates a registration for descriptor readiness.
    using RegisterFn = std::function<RegistrationPtr(int, bool, bool, NativeEventCallback)>;

public:
    /// Construct a device through its factory.
    /// @param registerFn The registration operation, retained for the device lifetime.
    /// @param receiveChunkLimit The maximum native receive chunk size.
    /// @param callbacks The owning source callbacks.
    PosixTcpConnectionDevice(
        PrivateTag, RegisterFn registerFn, unit::ByteCount receiveChunkLimit, TcpConnectionDeviceCallbacks callbacks);
    ~PosixTcpConnectionDevice() override;

public: // implement TcpConnectionDevice
    void connect(IpEndpoint remoteEndpoint) override;
    void accept(TcpAcceptedSocketPtr socket) override;
    [[nodiscard]] auto canAccept(const TcpAcceptedSocket &socket) const noexcept -> bool override;
    [[nodiscard]] auto send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus override;
    void setReceiving(unit::ByteCount maximumBytes) override;
    void close() noexcept override;
    void abort() noexcept override;

public: // factory
    /// Create a device for the platform-native event-loop driver.
    /// @param driver The platform-native event-loop driver.
    /// @param receiveChunkLimit The maximum native receive chunk size.
    /// @param callbacks The owning source callbacks.
    /// @return The shared native socket device.
    [[nodiscard]] static auto create(
        event::EventLoopDriverPtr driver, unit::ByteCount receiveChunkLimit, TcpConnectionDeviceCallbacks callbacks)
        -> PosixTcpConnectionDevicePtr;
    /// Create a device with an injected registration operation.
    /// @param registerFn The registration operation, retained for the device lifetime.
    /// @param receiveChunkLimit The maximum native receive chunk size.
    /// @param callbacks The owning source callbacks.
    /// @return The shared native socket device.
    [[nodiscard]] static auto create(
        RegisterFn registerFn, unit::ByteCount receiveChunkLimit, TcpConnectionDeviceCallbacks callbacks)
        -> PosixTcpConnectionDevicePtr;

private:
    /// Close and unregister while holding the operation lock.
    void closeLocked() noexcept;
    /// Create a native readiness-registration function for an event-loop driver.
    [[nodiscard]] static auto createRegisterFn(event::EventLoopDriverPtr driver) -> RegisterFn;
    /// Configure a socket descriptor for non-blocking TCP operation.
    static void configureDescriptor(int descriptor);
    /// Update native readiness registration for the current connection state.
    void updateRegistration();
    /// Handle native socket readiness notifications.
    void handleReady(bool readable, bool writable, bool error);
    /// Complete a pending non-blocking connection.
    void finishConnect(std::unique_lock<std::mutex> &lock);
    /// Write retained outgoing data until it would block.
    [[nodiscard]] auto writePending() -> bool;
    /// Receive one available native socket chunk.
    void receiveOne(std::unique_lock<std::mutex> &lock);
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
    int _descriptor{-1};                       ///< Native socket descriptor.
    unit::ByteCount _receiveChunkLimit;        ///< Largest native receive chunk.
    std::optional<IpEndpoint> _localEndpoint;  ///< Local endpoint after connection.
    std::optional<IpEndpoint> _remoteEndpoint; ///< Remote endpoint after connection.
    mem::ByteBlock _sendData;                  ///< Retained partial send block.
    std::size_t _sendOffset{};                 ///< Bytes already written from retained block.
    bool _connecting{false};                   ///< Whether connect completion is pending.
    bool _connected{false};                    ///< Whether the socket is established.
    unit::ByteCount _maximumReceiveSize;       ///< Largest next native receive, or zero.
    bool _sendPending{false};                  ///< Whether output awaits write readiness.
};

}
