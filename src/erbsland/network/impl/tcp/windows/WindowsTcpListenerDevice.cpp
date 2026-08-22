// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsTcpListenerDevice.hpp"

#include "WindowsTcpSocket.hpp"

#include "../../../../err/RuntimeError.hpp"
#include "../../../../system/WindowsErrorContext.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../source/NetworkError.hpp"
#include "../../platform/SocketAddress.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

WindowsTcpAcceptOperation::~WindowsTcpAcceptOperation() {
    if (socket != INVALID_SOCKET) {
        // A destructor cannot report failure and the operation no longer has an owner to recover the socket.
        ::closesocket(socket);
    }
}

WindowsTcpListenerState::WindowsTcpListenerState(
    event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks) :
    _runtime{WindowsNetworkRuntime::shared()},
    _driver{requireDriver(std::move(driver))},
    _callbacks{std::move(callbacks)} {
}

WindowsTcpListenerState::~WindowsTcpListenerState() = default;

auto WindowsTcpListenerState::start(IpEndpoint localEndpoint, const unit::ItemCount backlog) -> IpEndpoint {
    const auto lock = std::scoped_lock{_operationMutex};
    _family = localEndpoint.address().isV4() ? AF_INET : AF_INET6;
    const auto socket = ::WSASocketW(_family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (socket == INVALID_SOCKET) {
        throw createError(
            ::WSAGetLastError(),
            "TCP listener creation failed"_el,
            "The native listening socket could not be created."_el);
    }
    _socket.store(socket);
    try {
        constexpr auto enabled = DWORD{1U};
        if (::setsockopt(
                socket,
                SOL_SOCKET,
                SO_EXCLUSIVEADDRUSE,
                reinterpret_cast<const char *>(&enabled),
                static_cast<int>(sizeof(enabled))) == SOCKET_ERROR) {
            throw createError(
                ::WSAGetLastError(),
                "TCP listener setup failed"_el,
                "Exclusive address use could not be configured."_el);
        }
        if (_family == AF_INET6 &&
            ::setsockopt(
                socket,
                IPPROTO_IPV6,
                IPV6_V6ONLY,
                reinterpret_cast<const char *>(&enabled),
                static_cast<int>(sizeof(enabled))) == SOCKET_ERROR) {
            throw createError(
                ::WSAGetLastError(), "TCP listener setup failed"_el, "IPv6-only mode could not be configured."_el);
        }
        const auto nativeEndpoint = SocketAddress::fromEndpoint(localEndpoint);
        if (::bind(socket, nativeEndpoint.data(), nativeEndpoint.size()) == SOCKET_ERROR) {
            throw createError(
                ::WSAGetLastError(), "TCP listener bind failed"_el, "The listener could not bind its endpoint."_el);
        }
        const auto maximumBacklog = static_cast<std::size_t>(std::numeric_limits<int>::max());
        const auto nativeBacklog = static_cast<int>(std::min(backlog.toSizeT(), maximumBacklog));
        if (::listen(socket, nativeBacklog) == SOCKET_ERROR) {
            throw createError(
                ::WSAGetLastError(), "TCP listen failed"_el, "The native socket could not start listening."_el);
        }
        _localEndpoint = queryEndpoint(socket, false);
        _acceptEx = loadAcceptEx(socket);
        registerSocket(socket);
    } catch (...) {
        close();
        throw;
    }
    return *_localEndpoint;
}

void WindowsTcpListenerState::setAccepting(const bool enabled) {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_closed || _accepting == enabled) {
        return;
    }
    _accepting = enabled;
    if (!enabled) {
        const auto socket = _socket.load();
        if (socket != INVALID_SOCKET && _acceptOperation != nullptr) {
            // ERROR_NOT_FOUND only means the operation completed before cancellation and needs no recovery.
            ::CancelIoEx(reinterpret_cast<HANDLE>(socket), &_acceptOperation->overlapped);
        }
        return;
    }
    submitAccept();
}

void WindowsTcpListenerState::close() noexcept {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_closed) {
        return;
    }
    _closed = true;
    _accepting = false;
    _generation += 1U;
    const auto socket = _socket.exchange(INVALID_SOCKET);
    if (socket != INVALID_SOCKET) {
        // Cancellation may race with completion; closing the socket remains the definitive shutdown operation.
        ::CancelIoEx(reinterpret_cast<HANDLE>(socket), nullptr);
        // The state is already closed and cannot recover ownership if socket cleanup fails.
        ::closesocket(socket);
    }
    releaseRegistrationIfIdle();
}

auto WindowsTcpListenerState::requireDriver(event::EventLoopDriverPtr driver)
    -> std::shared_ptr<event::impl::WindowsEventLoopDriver> {
    auto result = std::dynamic_pointer_cast<event::impl::WindowsEventLoopDriver>(std::move(driver));
    if (result == nullptr) {
        throw err::RuntimeError{"TCP listeners require the platform's default native event-loop driver."_el};
    }
    return result;
}

void WindowsTcpListenerState::registerSocket(const SOCKET socket) {
    const auto self = shared_from_this();
    _registration = _driver->registerHandle(
        reinterpret_cast<HANDLE>(socket),
        [self](const DWORD transferred, OVERLAPPED *overlapped, const DWORD errorCode) -> void {
            self->complete(transferred, overlapped, errorCode);
        });
}

auto WindowsTcpListenerState::loadAcceptEx(const SOCKET socket) const -> LPFN_ACCEPTEX {
    const GUID functionId = WSAID_ACCEPTEX;
    auto function = LPFN_ACCEPTEX{};
    auto bytes = DWORD{};
    if (::WSAIoctl(
            socket,
            SIO_GET_EXTENSION_FUNCTION_POINTER,
            const_cast<GUID *>(&functionId),
            static_cast<DWORD>(sizeof(functionId)),
            &function,
            static_cast<DWORD>(sizeof(function)),
            &bytes,
            nullptr,
            nullptr) == SOCKET_ERROR) {
        throw createError(
            ::WSAGetLastError(), "TCP listener setup failed"_el, "The AcceptEx entry point is unavailable."_el);
    }
    return function;
}

void WindowsTcpListenerState::submitAccept() {
    const auto listener = _socket.load();
    if (_closed || !_accepting || listener == INVALID_SOCKET || _acceptOperation != nullptr) {
        return;
    }
    auto operation = std::make_unique<WindowsTcpAcceptOperation>(_generation);
    operation->socket = ::WSASocketW(_family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (operation->socket == INVALID_SOCKET) {
        throw createError(
            ::WSAGetLastError(),
            "TCP accept setup failed"_el,
            "The native accepted-socket placeholder could not be created."_el);
    }
    auto transferred = DWORD{};
    constexpr auto addressLength = static_cast<DWORD>(sizeof(sockaddr_storage) + 16U);
    const auto result = _acceptEx(
        listener,
        operation->socket,
        operation->addressBuffer.data(),
        0,
        addressLength,
        addressLength,
        &transferred,
        &operation->overlapped);
    if (result == FALSE) {
        const auto errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING) {
            throw createError(errorCode, "TCP accept failed"_el, "An accept operation could not be started."_el);
        }
    }
    _acceptOperation = std::move(operation);
}

void WindowsTcpListenerState::complete(
    [[maybe_unused]] const DWORD transferred, OVERLAPPED *overlapped, const DWORD errorCode) {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_acceptOperation == nullptr || &_acceptOperation->overlapped != overlapped) {
        return;
    }
    auto operation = std::move(_acceptOperation);
    const auto current = operation->generation == _generation && !_closed;
    const auto cancelled = errorCode == ERROR_OPERATION_ABORTED || errorCode == WSA_OPERATION_ABORTED;
    if (!current || cancelled) {
        if (current && _accepting) {
            submitAccept();
        }
        releaseRegistrationIfIdle();
        return;
    }
    if (errorCode != ERROR_SUCCESS) {
        if (_callbacks.error) {
            _callbacks.error(createContext(
                static_cast<int>(errorCode),
                "TCP accept failed"_el,
                "An incoming connection could not be accepted."_el));
        }
        return;
    }
    const auto listener = _socket.load();
    if (::setsockopt(
            operation->socket,
            SOL_SOCKET,
            SO_UPDATE_ACCEPT_CONTEXT,
            reinterpret_cast<const char *>(&listener),
            static_cast<int>(sizeof(listener))) == SOCKET_ERROR) {
        if (_callbacks.error) {
            _callbacks.error(createContext(
                ::WSAGetLastError(),
                "TCP accept setup failed"_el,
                "The accepted socket context could not be updated."_el));
        }
        return;
    }
    auto localEndpoint = IpEndpoint{};
    auto remoteEndpoint = IpEndpoint{};
    try {
        localEndpoint = queryEndpoint(operation->socket, false);
        remoteEndpoint = queryEndpoint(operation->socket, true);
    } catch (const NetworkError &endpointError) {
        if (_callbacks.error) {
            _callbacks.error(endpointError.context());
        }
        return;
    }
    auto accepted = std::make_unique<WindowsTcpSocket>(_runtime, operation->socket, localEndpoint, remoteEndpoint);
    operation->socket = INVALID_SOCKET;
    if (_accepting) {
        submitAccept();
    }
    if (_callbacks.accepted) {
        _callbacks.accepted(std::move(accepted));
    }
}

auto WindowsTcpListenerState::queryEndpoint(const SOCKET socket, const bool peer) const -> IpEndpoint {
    auto address = SocketAddress{};
    const auto result = peer ? ::getpeername(socket, address.data(), address.sizePointer())
                             : ::getsockname(socket, address.data(), address.sizePointer());
    if (result == SOCKET_ERROR) {
        throw createError(
            ::WSAGetLastError(), "TCP endpoint query failed"_el, "A native endpoint could not be determined."_el);
    }
    const auto endpoint = address.toEndpoint();
    if (!endpoint.has_value()) {
        throw NetworkError{NetworkErrorContext{
            "TCP endpoint query failed"_el, "The platform returned an unsupported address family."_el}
                .setReason(NetworkErrorReason::SocketOperationFailed)};
    }
    return *endpoint;
}

void WindowsTcpListenerState::releaseRegistrationIfIdle() noexcept {
    if (_closed && _acceptOperation == nullptr) {
        _registration.reset();
    }
}

auto WindowsTcpListenerState::createError(const int errorCode, text::String title, text::String description) const
    -> NetworkError {
    return NetworkError{createContext(errorCode, std::move(title), std::move(description))};
}

auto WindowsTcpListenerState::createContext(const int errorCode, text::String title, text::String description) const
    -> NetworkErrorContext {
    auto context = NetworkErrorContext{std::move(title), std::move(description)};
    context.setReason(errorReason(errorCode))
        .setPlatformContext(system::WindowsErrorContext::fromErrorCode(static_cast<DWORD>(errorCode)));
    if (_localEndpoint.has_value()) {
        context.setLocalEndpoint(*_localEndpoint);
    }
    return context;
}

auto WindowsTcpListenerState::errorReason(const int errorCode) noexcept -> NetworkErrorReason {
    switch (errorCode) {
    case WSAEADDRINUSE:
        return NetworkErrorReason::AddressInUse;
    case WSAEACCES:
        return NetworkErrorReason::PermissionDenied;
    default:
        return NetworkErrorReason::SocketOperationFailed;
    }
}

WindowsTcpListenerDevice::WindowsTcpListenerDevice(
    event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks) :
    _state{std::make_shared<WindowsTcpListenerState>(std::move(driver), std::move(callbacks))} {
}

WindowsTcpListenerDevice::~WindowsTcpListenerDevice() {
    close();
}

auto WindowsTcpListenerDevice::start(IpEndpoint localEndpoint, const unit::ItemCount backlog) -> IpEndpoint {
    return _state->start(std::move(localEndpoint), backlog);
}

void WindowsTcpListenerDevice::setAccepting(const bool enabled) {
    _state->setAccepting(enabled);
}

void WindowsTcpListenerDevice::close() noexcept {
    _state->close();
}

void WindowsTcpListenerDevice::abort() noexcept {
    _state->close();
}

auto TcpListenerDevice::create(event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks)
    -> TcpListenerDevicePtr {
    return std::make_unique<WindowsTcpListenerDevice>(std::move(driver), std::move(callbacks));
}

}
