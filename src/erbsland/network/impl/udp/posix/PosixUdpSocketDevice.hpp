// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../UdpSocketDevice.hpp"

#include "../../../../event/impl/EventLoopDriverRegistration.hpp"
#include "../../../../text/String.hpp"
#include "../../../source/NetworkError.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <optional>

namespace erbsland::network::impl {

/// POSIX implementation of a native UDP socket device.
/// @tested{UdpSocketLiveTest}
class PosixUdpSocketDevice final : public UdpSocketDevice {
    using RegistrationPtr = std::unique_ptr<event::impl::EventLoopDriverRegistration>;
    using RegisterFn = std::function<RegistrationPtr(int, bool, bool, std::function<void(bool, bool, bool)>)>;

public:
    /// Create a POSIX UDP socket device.
    PosixUdpSocketDevice(event::EventLoopDriverPtr driver, UdpSocketDeviceCallbacks callbacks);
    ~PosixUdpSocketDevice() override;

public: // implement UdpSocketDevice
    [[nodiscard]] auto bind(IpEndpoint localEndpoint, unit::ByteLength maximumDatagramSize) -> IpEndpoint override;
    [[nodiscard]] auto send(const UdpDatagram &datagram) -> UdpSocketDeviceSendStatus override;
    void setReceiving(bool enabled) override;
    void close() noexcept override;
    void abort() noexcept override;

private:
    /// Create native readiness registration for an event-loop driver.
    [[nodiscard]] static auto createRegisterFn(event::EventLoopDriverPtr driver) -> RegisterFn;
    /// Update readiness registration for the socket state.
    void updateRegistration();
    /// Handle native UDP socket readiness notifications.
    void handleReady(bool readable, bool writable, bool error);
    /// Receive one available UDP datagram.
    void receiveOne();
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
    std::atomic<int> _descriptor{-1};         ///< Native socket descriptor.
    RegistrationPtr _registration;            ///< Current readiness registration.
    std::optional<IpEndpoint> _localEndpoint; ///< Actual bound endpoint.
    unit::ByteLength _maximumDatagramSize;    ///< Native receive buffer size.
    bool _read{false};                        ///< Whether read readiness is registered.
    bool _write{false};                       ///< Whether write readiness is registered.
};

}
