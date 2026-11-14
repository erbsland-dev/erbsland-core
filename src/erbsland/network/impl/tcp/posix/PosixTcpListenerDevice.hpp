// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if !defined(__APPLE__) && !defined(__linux__)
#error "PosixTcpListenerDevice.hpp is only available on macOS and Linux."
#endif

#include "PosixTcpListenerDevice_fwd.hpp"

#include "../TcpListenerDevice.hpp"

#include "../../../../event/impl/EventLoopDriverRegistration.hpp"
#include "../../platform/PosixSocketDevice.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// POSIX non-blocking TCP listener device.
/// Create through create(); callers retain shared ownership throughout each operation.
/// @tested{TcpSocketLiveTest PosixSocketLifecycleTest}
class PosixTcpListenerDevice final : public TcpListenerDevice,
                                     private PosixSocketDevice,
                                     public std::enable_shared_from_this<PosixTcpListenerDevice> {
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
    /// @param callbacks The owning source callbacks.
    PosixTcpListenerDevice(PrivateTag, RegisterFn registerFn, TcpListenerDeviceCallbacks callbacks);
    ~PosixTcpListenerDevice() override;

public: // implement TcpListenerDevice
    [[nodiscard]] auto start(IpEndpoint localEndpoint, unit::ItemCount backlog) -> IpEndpoint override;
    void setAccepting(bool enabled) override;
    void close() noexcept override;
    void abort() noexcept override;

public: // factory
    /// Create a device for the platform-native event-loop driver.
    /// @param driver The platform-native event-loop driver.
    /// @param callbacks The owning source callbacks.
    /// @return The shared native socket device.
    [[nodiscard]] static auto create(event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks)
        -> PosixTcpListenerDevicePtr;
    /// Create a device with an injected registration operation.
    /// @param registerFn The registration operation, retained for the device lifetime.
    /// @param callbacks The owning source callbacks.
    /// @return The shared native socket device.
    [[nodiscard]] static auto create(RegisterFn registerFn, TcpListenerDeviceCallbacks callbacks)
        -> PosixTcpListenerDevicePtr;

private:
    /// Close and unregister while holding the operation lock.
    void closeLocked() noexcept;
    /// Create native readiness registration for an event-loop driver.
    [[nodiscard]] static auto createRegisterFn(event::EventLoopDriverPtr driver) -> RegisterFn;
    /// Configure a socket descriptor for non-blocking listening.
    static void configureDescriptor(int descriptor);
    /// Update readiness registration for the listener state.
    void updateRegistration();
    /// Handle native listener readiness notifications.
    void handleReady(bool readable, bool error);
    /// Accept all available pending connections.
    void acceptReady(std::unique_lock<std::mutex> &lock);
    /// Create context for a native listener error.
    [[nodiscard]] auto createContext(int errorCode, text::String title, text::String description) const
        -> NetworkErrorContext;
    /// Convert a native socket error code to a network error reason.
    [[nodiscard]] static auto errorReason(int errorCode) noexcept -> NetworkErrorReason;

private:
    RegisterFn _register;                     ///< Native driver registration function.
    TcpListenerDeviceCallbacks _callbacks;    ///< Core callbacks.
    RegistrationPtr _registration;            ///< Current readiness registration.
    int _descriptor{-1};                      ///< Native listener descriptor.
    std::optional<IpEndpoint> _localEndpoint; ///< Actual local endpoint.
    bool _accepting{false};                   ///< Whether read readiness is enabled.
};

}
