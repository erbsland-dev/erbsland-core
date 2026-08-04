// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsUdpSocketState.hpp"

#include "SocketAddress.hpp"
#include "WindowsUdpSocketOperation.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../mem/impl/UnsafeByteBlockAccess.hpp"
#include "../../system/WindowsErrorContext.hpp"
#include "../../text/Literals.hpp"

#include <cstddef>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

WindowsUdpSocketState::WindowsUdpSocketState(event::EventLoopDriverPtr driver, UdpSocketDeviceCallbacks callbacks) :
    _runtime{WindowsNetworkRuntime::shared()},
    _driver{requireDriver(std::move(driver))},
    _callbacks{std::move(callbacks)} {
}

WindowsUdpSocketState::~WindowsUdpSocketState() = default;

auto WindowsUdpSocketState::bind(IpEndpoint localEndpoint, const unit::ByteLength maximumDatagramSize) -> IpEndpoint {
    const auto lock = std::scoped_lock{_operationMutex};
    const auto family = localEndpoint.address().isV4() ? AF_INET : AF_INET6;
    const auto socket = ::WSASocketW(family, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (socket == INVALID_SOCKET) {
        throw createError(
            ::WSAGetLastError(), "UDP socket creation failed"_el, "The native UDP socket could not be created."_el);
    }
    _socket.store(socket);
    if (family == AF_INET6) {
        constexpr auto enabled = DWORD{1U};
        if (::setsockopt(
                socket,
                IPPROTO_IPV6,
                IPV6_V6ONLY,
                reinterpret_cast<const char *>(&enabled),
                static_cast<int>(sizeof(enabled))) == SOCKET_ERROR) {
            const auto error = createError(
                ::WSAGetLastError(),
                "UDP socket setup failed"_el,
                "The IPv6-only socket mode could not be configured."_el);
            close();
            throw error;
        }
    }
    const auto nativeEndpoint = SocketAddress::fromEndpoint(localEndpoint);
    if (::bind(socket, nativeEndpoint.data(), nativeEndpoint.size()) == SOCKET_ERROR) {
        auto context = createContext(
            ::WSAGetLastError(), "UDP bind failed"_el, "The UDP socket could not bind to the local endpoint."_el);
        context.setLocalEndpoint(localEndpoint);
        close();
        throw NetworkError{std::move(context)};
    }
    auto actualAddress = SocketAddress{};
    if (::getsockname(socket, actualAddress.data(), actualAddress.sizePointer()) == SOCKET_ERROR) {
        const auto error = createError(
            ::WSAGetLastError(), "UDP endpoint query failed"_el, "The bound UDP endpoint could not be determined."_el);
        close();
        throw error;
    }
    const auto actualEndpoint = actualAddress.toEndpoint();
    if (!actualEndpoint.has_value()) {
        close();
        throw NetworkError{NetworkErrorContext{
            "UDP endpoint query failed"_el, "The platform returned an unsupported bound address family."_el}
                .setReason(NetworkErrorReason::SocketOperationFailed)
                .setLocalEndpoint(localEndpoint)};
    }
    _localEndpoint = *actualEndpoint;
    _maximumDatagramSize = maximumDatagramSize;
    const auto self = shared_from_this();
    try {
        _registration = _driver->registerHandle(
            reinterpret_cast<HANDLE>(socket),
            [self](const DWORD transferred, OVERLAPPED *overlapped, const DWORD error) -> void {
                self->complete(transferred, overlapped, error);
            });
    } catch (...) {
        close();
        throw;
    }
    return *actualEndpoint;
}

auto WindowsUdpSocketState::send(const UdpDatagram &datagram) -> UdpSocketDeviceSendStatus {
    const auto lock = std::scoped_lock{_operationMutex};
    const auto socket = _socket.load();
    if (socket == INVALID_SOCKET || _closed || _sendOperation != nullptr) {
        return UdpSocketDeviceSendStatus::WouldBlock;
    }
    auto operation = std::make_unique<WindowsUdpSocketOperation>(_generation);
    operation->remoteAddress = SocketAddress::fromEndpoint(datagram.remoteEndpoint());
    const auto bytes = mem::impl::UnsafeByteBlockAccess{datagram.data()}.dataView().dataSpan();
    operation->nativeBuffer.buf = reinterpret_cast<char *>(const_cast<mem::Byte *>(bytes.data()));
    operation->nativeBuffer.len = static_cast<ULONG>(bytes.size());
    auto bytesSent = DWORD{};
    const auto result = ::WSASendTo(
        socket,
        &operation->nativeBuffer,
        1,
        &bytesSent,
        0,
        operation->remoteAddress.data(),
        operation->remoteAddress.size(),
        &operation->overlapped,
        nullptr);
    if (result == SOCKET_ERROR) {
        const auto errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING) {
            throw createError(errorCode, "UDP send failed"_el, "The datagram could not be sent."_el);
        }
    }
    _sendOperation = std::move(operation);
    return UdpSocketDeviceSendStatus::Pending;
}

void WindowsUdpSocketState::setReceiving(const bool enabled) {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_closed || _receiving == enabled) {
        return;
    }
    _receiving = enabled;
    if (!enabled) {
        cancelReceive();
        return;
    }
    submitReceive();
}

void WindowsUdpSocketState::close() noexcept {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_closed) {
        return;
    }
    _closed = true;
    _receiving = false;
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

auto WindowsUdpSocketState::requireDriver(event::EventLoopDriverPtr driver)
    -> std::shared_ptr<event::impl::WindowsEventLoopDriver> {
    auto result = std::dynamic_pointer_cast<event::impl::WindowsEventLoopDriver>(std::move(driver));
    if (result == nullptr) {
        throw err::RuntimeError{"The UDP socket requires the platform's default native event-loop driver."_el};
    }
    return result;
}

void WindowsUdpSocketState::submitReceive() {
    const auto socket = _socket.load();
    if (!_receiving || _closed || socket == INVALID_SOCKET || _receiveOperation != nullptr) {
        return;
    }
    auto operation = std::make_unique<WindowsUdpSocketOperation>(_generation);
    operation->receiveBuffer = mem::impl::UnsafeByteBlockBuffer{_maximumDatagramSize};
    const auto bytes = operation->receiveBuffer.data();
    operation->nativeBuffer.buf = reinterpret_cast<char *>(bytes.data());
    operation->nativeBuffer.len = static_cast<ULONG>(bytes.size());
    auto transferred = DWORD{};
    const auto result = ::WSARecvFrom(
        socket,
        &operation->nativeBuffer,
        1,
        &transferred,
        &operation->flags,
        operation->remoteAddress.data(),
        &operation->remoteAddressSize,
        &operation->overlapped,
        nullptr);
    if (result == SOCKET_ERROR) {
        const auto errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING) {
            throw createError(errorCode, "UDP receive failed"_el, "A receive operation could not be started."_el);
        }
    }
    _receiveOperation = std::move(operation);
}

void WindowsUdpSocketState::cancelReceive() noexcept {
    const auto socket = _socket.load();
    if (socket == INVALID_SOCKET || _receiveOperation == nullptr) {
        return;
    }
    // ERROR_NOT_FOUND only means the operation completed before cancellation and needs no recovery.
    ::CancelIoEx(reinterpret_cast<HANDLE>(socket), &_receiveOperation->overlapped);
}

void WindowsUdpSocketState::complete(const DWORD transferred, OVERLAPPED *overlapped, const DWORD errorCode) {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_receiveOperation != nullptr && &_receiveOperation->overlapped == overlapped) {
        completeReceive(transferred, errorCode);
        return;
    }
    if (_sendOperation != nullptr && &_sendOperation->overlapped == overlapped) {
        completeSend(errorCode);
    }
}

void WindowsUdpSocketState::completeReceive(const DWORD transferred, const DWORD errorCode) {
    auto operation = std::move(_receiveOperation);
    const auto current = operation->generation == _generation && !_closed;
    const auto cancelled = errorCode == ERROR_OPERATION_ABORTED || errorCode == WSA_OPERATION_ABORTED;
    if (!current || cancelled) {
        if (current && _receiving) {
            submitReceiveSafely();
        }
        releaseRegistrationIfIdle();
        return;
    }
    *operation->remoteAddress.sizePointer() = operation->remoteAddressSize;
    const auto remoteEndpoint = operation->remoteAddress.toEndpoint();
    if (errorCode == WSAEMSGSIZE || errorCode == ERROR_MORE_DATA) {
        submitReceiveSafely();
        if (_callbacks.datagramDropped) {
            _callbacks.datagramDropped(
                UdpDatagramDropContext{UdpDatagramDropReason::TooLarge, _maximumDatagramSize, remoteEndpoint});
        }
        return;
    }
    if (errorCode != ERROR_SUCCESS) {
        reportError(errorCode, "UDP receive failed"_el, "A datagram could not be received."_el);
        return;
    }
    if (!remoteEndpoint.has_value()) {
        reportError(
            WSAEAFNOSUPPORT, "UDP receive failed"_el, "The platform returned an unsupported remote address family."_el);
        return;
    }
    auto datagram = UdpDatagram{
        *remoteEndpoint,
        operation->receiveBuffer.take(unit::ByteLength::fromSizeT(static_cast<std::size_t>(transferred)))};
    submitReceiveSafely();
    if (_callbacks.datagram) {
        _callbacks.datagram(std::move(datagram));
    }
}

void WindowsUdpSocketState::completeSend(const DWORD errorCode) {
    const auto operation = std::move(_sendOperation);
    const auto current = operation->generation == _generation && !_closed;
    if (!current || errorCode == ERROR_OPERATION_ABORTED || errorCode == WSA_OPERATION_ABORTED) {
        releaseRegistrationIfIdle();
        return;
    }
    if (errorCode != ERROR_SUCCESS) {
        reportError(errorCode, "UDP send failed"_el, "The datagram could not be sent."_el);
        return;
    }
    if (_callbacks.sendCompleted) {
        _callbacks.sendCompleted();
    }
}

void WindowsUdpSocketState::submitReceiveSafely() {
    try {
        submitReceive();
    } catch (const NetworkError &error) {
        if (_callbacks.error) {
            _callbacks.error(error.context());
        }
    }
}

void WindowsUdpSocketState::reportError(const int errorCode, text::String title, text::String description) {
    if (_callbacks.error) {
        _callbacks.error(createContext(errorCode, std::move(title), std::move(description)));
    }
}

void WindowsUdpSocketState::releaseRegistrationIfIdle() noexcept {
    if (_closed && _receiveOperation == nullptr && _sendOperation == nullptr) {
        _registration.reset();
    }
}

auto WindowsUdpSocketState::createError(const int errorCode, text::String title, text::String description) const
    -> NetworkError {
    return NetworkError{createContext(errorCode, std::move(title), std::move(description))};
}

auto WindowsUdpSocketState::createContext(const int errorCode, text::String title, text::String description) const
    -> NetworkErrorContext {
    auto context = NetworkErrorContext{std::move(title), std::move(description)};
    context.setReason(errorReason(errorCode))
        .setPlatformContext(system::WindowsErrorContext::fromErrorCode(static_cast<DWORD>(errorCode)));
    if (_localEndpoint.has_value()) {
        context.setLocalEndpoint(*_localEndpoint);
    }
    return context;
}

auto WindowsUdpSocketState::errorReason(const int errorCode) noexcept -> NetworkErrorReason {
    switch (errorCode) {
    case WSAEADDRINUSE:
        return NetworkErrorReason::AddressInUse;
    case WSAEACCES:
        return NetworkErrorReason::PermissionDenied;
    case WSAENETUNREACH:
    case WSAEHOSTUNREACH:
        return NetworkErrorReason::NetworkUnreachable;
    case WSAECONNREFUSED:
        return NetworkErrorReason::ConnectionRefused;
    case WSAEMSGSIZE:
        return NetworkErrorReason::MessageTooLarge;
    default:
        return NetworkErrorReason::SocketOperationFailed;
    }
}

}
