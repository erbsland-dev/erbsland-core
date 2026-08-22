// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "WindowsTcpConnectionState.hpp is only available on Windows."
#endif

#include "WindowsTcpConnectionOperation.hpp"

#include "../TcpConnectionDevice.hpp"

#include "../../../../core/impl/WindowsApi.hpp"
#include "../../../../event/impl/EventLoopDriverRegistration.hpp"
#include "../../../../event/impl/WindowsEventLoopDriver.hpp"
#include "../../../source/NetworkError.hpp"
#include "../../platform/WindowsNetworkRuntime.hpp"

// The Windows SDK requires winsock2.h before mswsock.h.
// clang-format off
#include <winsock2.h>
#include <mswsock.h>
// clang-format on

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// Completion-owned Windows TCP connection state.
/// @tested{TcpSocketLiveTest}
class WindowsTcpConnectionState final : public std::enable_shared_from_this<WindowsTcpConnectionState> {
private:
    /// Own an event-loop driver registration.
    using RegistrationPtr = std::unique_ptr<event::impl::EventLoopDriverRegistration>;

public:
    /// Create state for a Windows TCP connection.
    WindowsTcpConnectionState(
        event::EventLoopDriverPtr driver, unit::ByteLength receiveChunkLimit, TcpConnectionDeviceCallbacks callbacks);
    /// Release the connection state and native operations.
    ~WindowsTcpConnectionState();

public:
    /// Begin connecting to `remoteEndpoint`.
    void connect(IpEndpoint remoteEndpoint);
    /// Adopt an accepted TCP socket.
    void accept(TcpAcceptedSocketPtr socket);
    /// Queue `data` for sending.
    [[nodiscard]] auto send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus;
    /// Configure the maximum number of bytes received next.
    void setReceiving(unit::ByteLength maximumBytes);
    /// Begin graceful connection shutdown.
    void close() noexcept;

private:
    /// Validate and downcast the event-loop driver.
    [[nodiscard]] static auto requireDriver(event::EventLoopDriverPtr driver)
        -> std::shared_ptr<event::impl::WindowsEventLoopDriver>;
    /// Register `socket` with the IOCP driver.
    void registerSocket(SOCKET socket);
    /// Resolve the ConnectEx extension for `socket`.
    [[nodiscard]] auto loadConnectEx(SOCKET socket) const -> LPFN_CONNECTEX;
    /// Submit a pending asynchronous receive.
    void submitReceive();
    /// Submit a pending asynchronous send.
    void submitSend();
    /// Dispatch a completed native operation.
    void complete(DWORD transferred, OVERLAPPED *overlapped, DWORD errorCode);
    /// Complete a connect operation.
    void completeConnect(DWORD errorCode);
    /// Complete a receive operation.
    void completeReceive(DWORD transferred, DWORD errorCode);
    /// Complete a send operation.
    void completeSend(DWORD transferred, DWORD errorCode);
    /// Release the IOCP registration when no operation needs it.
    void releaseRegistrationIfIdle() noexcept;
    /// Query the established local endpoint.
    [[nodiscard]] auto queryLocalEndpoint() const -> IpEndpoint;
    /// Create a network error for a native error code.
    [[nodiscard]] auto createError(int errorCode, text::String title, text::String description) const -> NetworkError;
    /// Create an error context for a native error code.
    [[nodiscard]] auto createContext(int errorCode, text::String title, text::String description) const
        -> NetworkErrorContext;
    /// Translate a native error code to its library reason.
    [[nodiscard]] static auto errorReason(int errorCode) noexcept -> NetworkErrorReason;

private:
    std::shared_ptr<WindowsNetworkRuntime> _runtime;                  ///< Shared Winsock lifetime.
    std::shared_ptr<event::impl::WindowsEventLoopDriver> _driver;     ///< Owner IOCP driver.
    std::recursive_mutex _operationMutex;                             ///< Serializes completion and shutdown.
    TcpConnectionDeviceCallbacks _callbacks;                          ///< Core callbacks.
    std::atomic<SOCKET> _socket{INVALID_SOCKET};                      ///< Native stream socket.
    RegistrationPtr _registration;                                    ///< IOCP registration.
    std::unique_ptr<WindowsTcpConnectionOperation> _connectOperation; ///< Pending connect.
    std::unique_ptr<WindowsTcpConnectionOperation> _receiveOperation; ///< Pending receive.
    std::unique_ptr<WindowsTcpConnectionOperation> _sendOperation;    ///< Pending send.
    unit::ByteLength _receiveChunkLimit;                              ///< Largest receive chunk.
    std::optional<IpEndpoint> _localEndpoint;                         ///< Established local endpoint.
    std::optional<IpEndpoint> _remoteEndpoint;                        ///< Established remote endpoint.
    std::uint64_t _generation{1U};                                    ///< Current operation generation.
    bool _connected{false};                                           ///< Whether the socket is established.
    unit::ByteLength _maximumReceiveSize;                             ///< Largest next native receive, or zero.
    bool _closed{false};                                              ///< Whether shutdown started.
};

}
