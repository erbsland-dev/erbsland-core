// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if !defined(__APPLE__) && !defined(__linux__)
#error "PosixTcpListenerDevice.hpp is only available on macOS and Linux."
#endif

#include "TcpListenerDevice.hpp"

#include "../../event/impl/EventLoopDriverRegistration.hpp"

#include <atomic>
#include <functional>

namespace erbsland::network::impl {

/// POSIX non-blocking TCP listener device.
/// @tested{TcpSocketLiveTest}
class PosixTcpListenerDevice final : public TcpListenerDevice {
private:
    using RegistrationPtr = std::unique_ptr<event::impl::EventLoopDriverRegistration>;
    using NativeEventCallback = std::function<void(bool, bool, bool)>;
    using RegisterFn = std::function<RegistrationPtr(int, bool, bool, NativeEventCallback)>;

public:
    /// Create a POSIX TCP listener device.
    PosixTcpListenerDevice(event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks);
    ~PosixTcpListenerDevice() override;

public: // implement TcpListenerDevice
    [[nodiscard]] auto start(IpEndpoint localEndpoint, unit::ItemCount backlog) -> IpEndpoint override;
    void setAccepting(bool enabled) override;
    void close() noexcept override;
    void abort() noexcept override;

private:
    /// Create native readiness registration for an event-loop driver.
    [[nodiscard]] static auto createRegisterFn(event::EventLoopDriverPtr driver) -> RegisterFn;
    /// Configure a socket descriptor for non-blocking listening.
    static void configureDescriptor(int descriptor);
    /// Update readiness registration for the listener state.
    void updateRegistration();
    /// Handle native listener readiness notifications.
    void handleReady(bool readable, bool error);
    /// Accept all available pending connections.
    void acceptReady();
    /// Create context for a native listener error.
    [[nodiscard]] auto createContext(int errorCode, text::String title, text::String description) const
        -> NetworkErrorContext;
    /// Convert a native socket error code to a network error reason.
    [[nodiscard]] static auto errorReason(int errorCode) noexcept -> NetworkErrorReason;

private:
    RegisterFn _register;                     ///< Native driver registration function.
    TcpListenerDeviceCallbacks _callbacks;    ///< Core callbacks.
    RegistrationPtr _registration;            ///< Current readiness registration.
    std::atomic<int> _descriptor{-1};         ///< Native listener descriptor.
    std::optional<IpEndpoint> _localEndpoint; ///< Actual local endpoint.
    bool _accepting{false};                   ///< Whether read readiness is enabled.
};

}
