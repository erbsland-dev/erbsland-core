// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PosixUdpSocketDevice_fwd.hpp"

#include "../UdpSocketDevice.hpp"

#include "../../../../event/impl/EventLoopDriverRegistration.hpp"
#include "../../../../text/String.hpp"
#include "../../../source/NetworkError.hpp"
#include "../../platform/PosixSocketDevice.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// POSIX non-blocking datagram device.
/// Create through create(); callers retain shared ownership throughout each operation.
/// @tested{UdpSocketLiveTest PosixSocketLifecycleTest}
class PosixUdpSocketDevice final : public UdpSocketDevice,
                                   private PosixSocketDevice,
                                   public std::enable_shared_from_this<PosixUdpSocketDevice> {
    struct PrivateTag {};

public:
    /// Owns a native readiness registration.
    using RegistrationPtr = std::unique_ptr<event::impl::EventLoopDriverRegistration>;
    /// Creates a registration for descriptor readiness.
    using RegisterFn = std::function<RegistrationPtr(int, bool, bool, std::function<void(bool, bool, bool)>)>;

public:
    /// Construct a device through its factory.
    /// @param registerFn The registration operation, retained for the device lifetime.
    /// @param callbacks The owning source callbacks.
    PosixUdpSocketDevice(PrivateTag, RegisterFn registerFn, UdpSocketDeviceCallbacks callbacks);
    ~PosixUdpSocketDevice() override;

public: // implement UdpSocketDevice
    [[nodiscard]] auto bind(IpEndpoint localEndpoint, unit::ByteCount maximumDatagramSize) -> IpEndpoint override;
    [[nodiscard]] auto send(const UdpDatagram &datagram) -> UdpSocketDeviceSendStatus override;
    void setReceiving(bool enabled) override;
    void close() noexcept override;
    void abort() noexcept override;

public: // factory
    /// Create a device for the platform-native event-loop driver.
    /// @param driver The platform-native event-loop driver.
    /// @param callbacks The owning source callbacks.
    /// @return The shared native socket device.
    [[nodiscard]] static auto create(event::EventLoopDriverPtr driver, UdpSocketDeviceCallbacks callbacks)
        -> PosixUdpSocketDevicePtr;
    /// Create a device with an injected registration operation.
    /// @param registerFn The registration operation, retained for the device lifetime.
    /// @param callbacks The owning source callbacks.
    /// @return The shared native socket device.
    [[nodiscard]] static auto create(RegisterFn registerFn, UdpSocketDeviceCallbacks callbacks)
        -> PosixUdpSocketDevicePtr;

private:
    /// Close and unregister while holding the operation lock.
    void closeLocked() noexcept;
    /// Create native readiness registration for an event-loop driver.
    [[nodiscard]] static auto createRegisterFn(event::EventLoopDriverPtr driver) -> RegisterFn;
    /// Update readiness registration for the socket state.
    void updateRegistration();
    /// Handle native UDP socket readiness notifications.
    void handleReady(bool readable, bool writable, bool error);
    /// Receive one available UDP datagram.
    void receiveOne(std::unique_lock<std::mutex> &lock);
    /// Create a network error for the current socket state.
    [[nodiscard]] auto createError(text::String title, text::String description) const -> NetworkError;
    /// Create context for a native UDP socket error.
    [[nodiscard]] auto createContext(int errorCode, text::String title, text::String description) const
        -> NetworkErrorContext;
    /// Convert a native socket error code to a network error reason.
    [[nodiscard]] static auto errorReason(int errorCode) noexcept -> NetworkErrorReason;

private:
    RegisterFn _register;                     ///< Platform driver registration operation.
    UdpSocketDeviceCallbacks _callbacks;      ///< Source callbacks.
    int _descriptor{-1};                      ///< Native socket descriptor.
    RegistrationPtr _registration;            ///< Current readiness registration.
    std::optional<IpEndpoint> _localEndpoint; ///< Actual bound endpoint.
    unit::ByteCount _maximumDatagramSize;     ///< Native receive buffer size.
    bool _read{false};                        ///< Whether read readiness is registered.
    bool _write{false};                       ///< Whether write readiness is registered.
};

}
