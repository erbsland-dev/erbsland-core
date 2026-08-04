// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "WindowsTcpListenerState.hpp is only available on Windows."
#endif

#include "TcpListenerDevice.hpp"
#include "WindowsNetworkRuntime.hpp"
#include "WindowsTcpAcceptOperation.hpp"

#include "../source/NetworkError.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../event/impl/EventLoopDriverRegistration.hpp"
#include "../../event/impl/WindowsEventLoopDriver.hpp"

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

/// Completion-owned Windows TCP listener state.
/// @tested{TcpSocketLiveTest}
class WindowsTcpListenerState final : public std::enable_shared_from_this<WindowsTcpListenerState> {
private:
    using RegistrationPtr = std::unique_ptr<event::impl::EventLoopDriverRegistration>;

public:
    /// Creates Windows listener state.
    /// @param driver The owning event-loop driver.
    /// @param callbacks The listener callbacks.
    WindowsTcpListenerState(event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks);
    /// Closes the listener state and releases its native resources.
    ~WindowsTcpListenerState();

public:
    /// Bind the native socket and return its resolved local endpoint.
    /// @param localEndpoint The requested listening endpoint.
    /// @param backlog The maximum pending connection count.
    /// @return The actual bound endpoint.
    [[nodiscard]] auto start(IpEndpoint localEndpoint, unit::ItemCount backlog) -> IpEndpoint;
    /// Enable or pause submission of native accept operations.
    /// @param enabled `true` to accept connections.
    void setAccepting(bool enabled);
    /// Close the listener and release its native resources.
    void close() noexcept;

private:
    /// Validate and retain the Windows event-loop driver.
    /// @param driver The generic event-loop driver.
    /// @return The corresponding Windows driver.
    [[nodiscard]] static auto requireDriver(event::EventLoopDriverPtr driver)
        -> std::shared_ptr<event::impl::WindowsEventLoopDriver>;
    /// Register a socket with the IO completion port.
    /// @param socket The native socket to register.
    void registerSocket(SOCKET socket);
    /// Load the AcceptEx extension function for a socket.
    /// @param socket The socket whose provider supplies AcceptEx.
    /// @return The loaded native function.
    [[nodiscard]] auto loadAcceptEx(SOCKET socket) const -> LPFN_ACCEPTEX;
    /// Submit one asynchronous accept operation when accepting is enabled.
    void submitAccept();
    /// Process completion of an asynchronous accept operation.
    /// @param transferred The number of transferred bytes.
    /// @param overlapped The completed native operation.
    /// @param errorCode The native completion error code.
    void complete(DWORD transferred, OVERLAPPED *overlapped, DWORD errorCode);
    /// Query a socket's local or peer endpoint.
    /// @param socket The connected or listening socket.
    /// @param peer `true` to query the peer endpoint.
    /// @return The queried endpoint.
    [[nodiscard]] auto queryEndpoint(SOCKET socket, bool peer) const -> IpEndpoint;
    /// Release the IOCP registration after all operations are complete.
    void releaseRegistrationIfIdle() noexcept;
    /// Create a network error with listener context.
    /// @param errorCode The native error code.
    /// @param title The error title.
    /// @param description The error description.
    /// @return The constructed error.
    [[nodiscard]] auto createError(int errorCode, text::String title, text::String description) const -> NetworkError;
    /// Create context for a native listener error.
    /// @param errorCode The native error code.
    /// @param title The context title.
    /// @param description The context description.
    /// @return The constructed error context.
    [[nodiscard]] auto createContext(int errorCode, text::String title, text::String description) const
        -> NetworkErrorContext;
    /// Convert a native error code to a library error reason.
    /// @param errorCode The native error code.
    /// @return The corresponding error reason.
    [[nodiscard]] static auto errorReason(int errorCode) noexcept -> NetworkErrorReason;

private:
    std::shared_ptr<WindowsNetworkRuntime> _runtime;              ///< Shared Winsock lifetime.
    std::shared_ptr<event::impl::WindowsEventLoopDriver> _driver; ///< Owner IOCP driver.
    std::recursive_mutex _operationMutex;                         ///< Serializes completion and shutdown.
    TcpListenerDeviceCallbacks _callbacks;                        ///< Core callbacks.
    std::atomic<SOCKET> _socket{INVALID_SOCKET};                  ///< Native listening socket.
    RegistrationPtr _registration;                                ///< IOCP registration.
    std::unique_ptr<WindowsTcpAcceptOperation> _acceptOperation;  ///< Pending native accept.
    LPFN_ACCEPTEX _acceptEx{};                                    ///< Loaded AcceptEx entry point.
    std::optional<IpEndpoint> _localEndpoint;                     ///< Actual listening endpoint.
    int _family{AF_UNSPEC};                                       ///< Listener address family.
    std::uint64_t _generation{1U};                                ///< Current operation generation.
    bool _accepting{false};                                       ///< Whether native accepts are enabled.
    bool _closed{false};                                          ///< Whether shutdown started.
};

}
