// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpSocketDeviceCallbacks.hpp"
#include "UdpSocketDeviceSendStatus.hpp"
#include "WindowsNetworkRuntime.hpp"
#include "WindowsUdpSocketOperation_fwd.hpp"
#include "WindowsUdpSocketState_fwd.hpp"

#include "../IpEndpoint.hpp"
#include "../source/NetworkError.hpp"
#include "../udp/UdpDatagram.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../event/EventLoopDriver_fwd.hpp"
#include "../../event/impl/EventLoopDriverRegistration.hpp"
#include "../../event/impl/WindowsEventLoopDriver.hpp"
#include "../../text/String.hpp"

#include <winsock2.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// Completion-owned state for a Windows native UDP socket.
/// @tested{UdpSocketLiveTest}
class WindowsUdpSocketState final : public std::enable_shared_from_this<WindowsUdpSocketState> {
    using RegistrationPtr = std::unique_ptr<event::impl::EventLoopDriverRegistration>;

public:
    /// Create completion-owned state for a Windows UDP socket.
    /// @param driver The Windows event loop driver.
    /// @param callbacks The socket callbacks to invoke.
    WindowsUdpSocketState(event::EventLoopDriverPtr driver, UdpSocketDeviceCallbacks callbacks);
    /// Close native state and release its driver registration.
    ~WindowsUdpSocketState();

public:
    /// Bind the socket to a local endpoint.
    [[nodiscard]] auto bind(IpEndpoint localEndpoint, unit::ByteLength maximumDatagramSize) -> IpEndpoint;
    /// Submit a datagram for sending.
    [[nodiscard]] auto send(const UdpDatagram &datagram) -> UdpSocketDeviceSendStatus;
    /// Enable or disable pending receive operations.
    void setReceiving(bool enabled);
    /// Close the native socket and cancel pending operations.
    void close() noexcept;

private:
    /// Validate and convert an event loop driver for Windows UDP operations.
    [[nodiscard]] static auto requireDriver(event::EventLoopDriverPtr driver)
        -> std::shared_ptr<event::impl::WindowsEventLoopDriver>;
    /// Submit one native receive operation.
    void submitReceive();
    /// Cancel the pending native receive operation.
    void cancelReceive() noexcept;
    /// Dispatch a native IO completion to its operation handler.
    void complete(DWORD transferred, OVERLAPPED *overlapped, DWORD errorCode);
    /// Complete the pending native receive operation.
    void completeReceive(DWORD transferred, DWORD errorCode);
    /// Complete the pending native send operation.
    void completeSend(DWORD errorCode);
    /// Submit a receive operation and report any submission error.
    void submitReceiveSafely();
    /// Report a native socket error through the configured callbacks.
    void reportError(int errorCode, text::String title, text::String description);
    /// Release the IOCP registration after all operations have completed.
    void releaseRegistrationIfIdle() noexcept;
    /// Create a network error for a native socket error code.
    [[nodiscard]] auto createError(int errorCode, text::String title, text::String description) const -> NetworkError;
    /// Create the context for a native socket error code.
    [[nodiscard]] auto createContext(int errorCode, text::String title, text::String description) const
        -> NetworkErrorContext;
    /// Map a native socket error code to its public reason.
    [[nodiscard]] static auto errorReason(int errorCode) noexcept -> NetworkErrorReason;

private:
    std::shared_ptr<WindowsNetworkRuntime> _runtime;              ///< Shared Winsock lifetime.
    std::shared_ptr<event::impl::WindowsEventLoopDriver> _driver; ///< Owner IOCP driver.
    std::recursive_mutex _operationMutex;                         ///< Serializes abort and native completion.
    UdpSocketDeviceCallbacks _callbacks;                          ///< Source callbacks.
    std::atomic<SOCKET> _socket{INVALID_SOCKET};                  ///< Native datagram socket.
    RegistrationPtr _registration;                                ///< IOCP handle registration.
    std::unique_ptr<WindowsUdpSocketOperation> _receiveOperation; ///< Stable pending receive record.
    std::unique_ptr<WindowsUdpSocketOperation> _sendOperation;    ///< Stable pending send record.
    std::optional<IpEndpoint> _localEndpoint;                     ///< Actual bound endpoint.
    unit::ByteLength _maximumDatagramSize;                        ///< Configured receive size.
    std::uint64_t _generation{1U};                                ///< Current operation generation.
    bool _receiving{false};                                       ///< Whether receive operations are enabled.
    bool _closed{false};                                          ///< Whether native shutdown started.
};

}
