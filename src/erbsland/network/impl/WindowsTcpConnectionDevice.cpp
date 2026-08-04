// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsTcpConnectionDevice.hpp"

#include "SocketAddress.hpp"
#include "WindowsTcpSocket.hpp"

#include "../source/NetworkError.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../mem/impl/UnsafeByteBlockAccess.hpp"
#include "../../system/WindowsErrorContext.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

WindowsTcpConnectionState::WindowsTcpConnectionState(
    event::EventLoopDriverPtr driver,
    const unit::ByteLength receiveChunkLimit,
    TcpConnectionDeviceCallbacks callbacks) :
    _runtime{WindowsNetworkRuntime::shared()},
    _driver{requireDriver(std::move(driver))},
    _callbacks{std::move(callbacks)},
    _receiveChunkLimit{receiveChunkLimit} {
}

WindowsTcpConnectionState::~WindowsTcpConnectionState() = default;

void WindowsTcpConnectionState::connect(IpEndpoint remoteEndpoint) {
    const auto lock = std::scoped_lock{_operationMutex};
    const auto family = remoteEndpoint.address().isV4() ? AF_INET : AF_INET6;
    const auto socket = ::WSASocketW(family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (socket == INVALID_SOCKET) {
        throw createError(
            ::WSAGetLastError(), "TCP socket creation failed"_el, "The native TCP socket could not be created."_el);
    }
    _socket.store(socket);
    _remoteEndpoint = remoteEndpoint;
    try {
        registerSocket(socket);
        const auto anyAddress = remoteEndpoint.address().isV4() ? IpAddress::anyV4() : IpAddress::anyV6();
        const auto localAddress = SocketAddress::fromEndpoint(IpEndpoint{anyAddress, Port{}});
        if (::bind(socket, localAddress.data(), localAddress.size()) == SOCKET_ERROR) {
            throw createError(
                ::WSAGetLastError(), "TCP socket setup failed"_el, "The connecting socket could not be bound."_el);
        }
        const auto connectEx = loadConnectEx(socket);
        auto operation =
            std::make_unique<WindowsTcpConnectionOperation>(WindowsTcpConnectionOperation::Type::Connect, _generation);
        const auto nativeRemote = SocketAddress::fromEndpoint(remoteEndpoint);
        const auto result =
            connectEx(socket, nativeRemote.data(), nativeRemote.size(), nullptr, 0, nullptr, &operation->overlapped);
        if (result == FALSE) {
            const auto errorCode = ::WSAGetLastError();
            if (errorCode != WSA_IO_PENDING) {
                throw createError(
                    errorCode,
                    "TCP connection failed"_el,
                    "The native socket could not connect to the remote endpoint."_el);
            }
        }
        _connectOperation = std::move(operation);
    } catch (...) {
        close();
        throw;
    }
}

void WindowsTcpConnectionState::accept(TcpAcceptedSocketPtr socket) {
    const auto lock = std::scoped_lock{_operationMutex};
    auto *windowsSocket = dynamic_cast<WindowsTcpSocket *>(socket.get());
    if (windowsSocket == nullptr) {
        throw err::RuntimeError{"The accepted TCP socket is incompatible with the Windows connection device."_el};
    }
    _localEndpoint = windowsSocket->localEndpoint();
    _remoteEndpoint = windowsSocket->remoteEndpoint();
    const auto nativeSocket = windowsSocket->takeSocket();
    socket.reset();
    try {
        _socket.store(nativeSocket);
        registerSocket(nativeSocket);
        _connected = true;
    } catch (...) {
        // Preserve the registration error; ownership cannot be restored and cleanup failure is not actionable.
        ::closesocket(nativeSocket);
        _socket.store(INVALID_SOCKET);
        throw;
    }
    if (_callbacks.connected) {
        _callbacks.connected(*_localEndpoint, *_remoteEndpoint);
    }
}

auto WindowsTcpConnectionState::send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_closed || !_connected || _sendOperation != nullptr) {
        throw err::RuntimeError{"The native TCP device cannot retain another output block."_el};
    }
    _sendOperation =
        std::make_unique<WindowsTcpConnectionOperation>(WindowsTcpConnectionOperation::Type::Send, _generation);
    _sendOperation->sendData = std::move(data);
    try {
        submitSend();
    } catch (...) {
        _sendOperation.reset();
        throw;
    }
    return TcpConnectionDeviceSendStatus::Pending;
}

void WindowsTcpConnectionState::setReceiving(const unit::ByteLength maximumBytes) {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_closed) {
        return;
    }
    _maximumReceiveSize = std::min(maximumBytes, _receiveChunkLimit);
    if (_maximumReceiveSize.isZero()) {
        const auto socket = _socket.load();
        if (socket != INVALID_SOCKET && _receiveOperation != nullptr) {
            // ERROR_NOT_FOUND only means the operation completed before cancellation and needs no recovery.
            ::CancelIoEx(reinterpret_cast<HANDLE>(socket), &_receiveOperation->overlapped);
        }
        return;
    }
    submitReceive();
}

void WindowsTcpConnectionState::close() noexcept {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_closed) {
        return;
    }
    _closed = true;
    _maximumReceiveSize = {};
    _connected = false;
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

auto WindowsTcpConnectionState::requireDriver(event::EventLoopDriverPtr driver)
    -> std::shared_ptr<event::impl::WindowsEventLoopDriver> {
    auto result = std::dynamic_pointer_cast<event::impl::WindowsEventLoopDriver>(std::move(driver));
    if (result == nullptr) {
        throw err::RuntimeError{"TCP sockets require the platform's default native event-loop driver."_el};
    }
    return result;
}

void WindowsTcpConnectionState::registerSocket(const SOCKET socket) {
    const auto self = shared_from_this();
    _registration = _driver->registerHandle(
        reinterpret_cast<HANDLE>(socket),
        [self](const DWORD transferred, OVERLAPPED *overlapped, const DWORD errorCode) -> void {
            self->complete(transferred, overlapped, errorCode);
        });
}

auto WindowsTcpConnectionState::loadConnectEx(const SOCKET socket) const -> LPFN_CONNECTEX {
    const GUID functionId = WSAID_CONNECTEX;
    auto function = LPFN_CONNECTEX{};
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
            ::WSAGetLastError(), "TCP socket setup failed"_el, "The ConnectEx entry point is unavailable."_el);
    }
    return function;
}

void WindowsTcpConnectionState::submitReceive() {
    const auto socket = _socket.load();
    if (_closed || !_connected || _maximumReceiveSize.isZero() || socket == INVALID_SOCKET ||
        _receiveOperation != nullptr) {
        return;
    }
    auto operation =
        std::make_unique<WindowsTcpConnectionOperation>(WindowsTcpConnectionOperation::Type::Receive, _generation);
    operation->receiveBuffer = mem::impl::UnsafeByteBlockBuffer{_maximumReceiveSize};
    const auto bytes = operation->receiveBuffer.data();
    operation->nativeBuffer.buf = reinterpret_cast<char *>(bytes.data());
    operation->nativeBuffer.len =
        static_cast<ULONG>(std::min(bytes.size(), static_cast<std::size_t>(std::numeric_limits<ULONG>::max())));
    auto transferred = DWORD{};
    const auto result = ::WSARecv(
        socket, &operation->nativeBuffer, 1, &transferred, &operation->flags, &operation->overlapped, nullptr);
    if (result == SOCKET_ERROR) {
        const auto errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING) {
            throw createError(errorCode, "TCP receive failed"_el, "A receive operation could not be started."_el);
        }
    }
    _receiveOperation = std::move(operation);
}

void WindowsTcpConnectionState::submitSend() {
    const auto socket = _socket.load();
    if (_closed || socket == INVALID_SOCKET || _sendOperation == nullptr) {
        return;
    }
    const auto bytes = mem::impl::UnsafeByteBlockAccess{_sendOperation->sendData}.dataView().dataSpan();
    const auto remaining = bytes.subspan(_sendOperation->sendOffset);
    _sendOperation->nativeBuffer.buf = reinterpret_cast<char *>(const_cast<mem::Byte *>(remaining.data()));
    _sendOperation->nativeBuffer.len =
        static_cast<ULONG>(std::min(remaining.size(), static_cast<std::size_t>(std::numeric_limits<ULONG>::max())));
    auto transferred = DWORD{};
    const auto result =
        ::WSASend(socket, &_sendOperation->nativeBuffer, 1, &transferred, 0, &_sendOperation->overlapped, nullptr);
    if (result == SOCKET_ERROR) {
        const auto errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING) {
            throw createError(errorCode, "TCP send failed"_el, "Stream data could not be sent."_el);
        }
    }
}

void WindowsTcpConnectionState::complete(const DWORD transferred, OVERLAPPED *overlapped, const DWORD errorCode) {
    const auto lock = std::scoped_lock{_operationMutex};
    if (_connectOperation != nullptr && &_connectOperation->overlapped == overlapped) {
        completeConnect(errorCode);
        return;
    }
    if (_receiveOperation != nullptr && &_receiveOperation->overlapped == overlapped) {
        completeReceive(transferred, errorCode);
        return;
    }
    if (_sendOperation != nullptr && &_sendOperation->overlapped == overlapped) {
        completeSend(transferred, errorCode);
    }
}

void WindowsTcpConnectionState::completeConnect(const DWORD errorCode) {
    const auto operation = std::move(_connectOperation);
    const auto current = operation->generation == _generation && !_closed;
    if (!current || errorCode == ERROR_OPERATION_ABORTED || errorCode == WSA_OPERATION_ABORTED) {
        releaseRegistrationIfIdle();
        return;
    }
    if (errorCode != ERROR_SUCCESS) {
        if (_callbacks.error) {
            _callbacks.error(createContext(
                static_cast<int>(errorCode),
                "TCP connection failed"_el,
                "The remote endpoint rejected or failed the connection."_el));
        }
        return;
    }
    const auto socket = _socket.load();
    if (::setsockopt(socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0) == SOCKET_ERROR) {
        if (_callbacks.error) {
            _callbacks.error(createContext(
                ::WSAGetLastError(),
                "TCP connection setup failed"_el,
                "The connected socket context could not be updated."_el));
        }
        return;
    }
    try {
        _localEndpoint = queryLocalEndpoint();
    } catch (const NetworkError &error) {
        if (_callbacks.error) {
            _callbacks.error(error.context());
        }
        return;
    }
    _connected = true;
    if (_callbacks.connected) {
        _callbacks.connected(*_localEndpoint, *_remoteEndpoint);
    }
}

void WindowsTcpConnectionState::completeReceive(const DWORD transferred, const DWORD errorCode) {
    auto operation = std::move(_receiveOperation);
    const auto current = operation->generation == _generation && !_closed;
    const auto cancelled = errorCode == ERROR_OPERATION_ABORTED || errorCode == WSA_OPERATION_ABORTED;
    if (!current || cancelled) {
        if (current && !_maximumReceiveSize.isZero()) {
            try {
                submitReceive();
            } catch (const NetworkError &error) {
                if (_callbacks.error) {
                    _callbacks.error(error.context());
                }
            }
        }
        releaseRegistrationIfIdle();
        return;
    }
    if (errorCode != ERROR_SUCCESS) {
        if (_callbacks.error) {
            _callbacks.error(createContext(
                static_cast<int>(errorCode), "TCP receive failed"_el, "Stream data could not be received."_el));
        }
        return;
    }
    if (transferred == 0U) {
        _maximumReceiveSize = {};
        if (_callbacks.remoteClosed) {
            _callbacks.remoteClosed();
        }
        return;
    }
    auto data = operation->receiveBuffer.take(unit::ByteLength::fromSizeT(static_cast<std::size_t>(transferred)));
    if (_callbacks.data) {
        _callbacks.data(std::move(data));
    } else {
        submitReceive();
    }
}

void WindowsTcpConnectionState::completeSend(const DWORD transferred, const DWORD errorCode) {
    auto operation = std::move(_sendOperation);
    const auto current = operation->generation == _generation && !_closed;
    if (!current || errorCode == ERROR_OPERATION_ABORTED || errorCode == WSA_OPERATION_ABORTED) {
        releaseRegistrationIfIdle();
        return;
    }
    if (errorCode != ERROR_SUCCESS || transferred == 0U) {
        if (_callbacks.error) {
            _callbacks.error(createContext(
                errorCode == ERROR_SUCCESS ? WSAECONNRESET : static_cast<int>(errorCode),
                "TCP send failed"_el,
                "Stream data could not be sent."_el));
        }
        return;
    }
    operation->sendOffset += static_cast<std::size_t>(transferred);
    if (operation->sendOffset < operation->sendData.length().toSizeT()) {
        _sendOperation = std::move(operation);
        ZeroMemory(&_sendOperation->overlapped, sizeof(_sendOperation->overlapped));
        try {
            submitSend();
        } catch (const NetworkError &error) {
            _sendOperation.reset();
            if (_callbacks.error) {
                _callbacks.error(error.context());
            }
        }
        return;
    }
    if (_callbacks.sendCompleted) {
        _callbacks.sendCompleted();
    }
}

void WindowsTcpConnectionState::releaseRegistrationIfIdle() noexcept {
    if (_closed && _connectOperation == nullptr && _receiveOperation == nullptr && _sendOperation == nullptr) {
        _registration.reset();
    }
}

auto WindowsTcpConnectionState::queryLocalEndpoint() const -> IpEndpoint {
    const auto socket = _socket.load();
    auto address = SocketAddress{};
    if (::getsockname(socket, address.data(), address.sizePointer()) == SOCKET_ERROR) {
        throw createError(
            ::WSAGetLastError(), "TCP endpoint query failed"_el, "The local endpoint could not be determined."_el);
    }
    const auto endpoint = address.toEndpoint();
    if (!endpoint.has_value()) {
        throw NetworkError{NetworkErrorContext{
            "TCP endpoint query failed"_el, "The platform returned an unsupported local address family."_el}
                .setReason(NetworkErrorReason::SocketOperationFailed)};
    }
    return *endpoint;
}

auto WindowsTcpConnectionState::createError(const int errorCode, text::String title, text::String description) const
    -> NetworkError {
    return NetworkError{createContext(errorCode, std::move(title), std::move(description))};
}

auto WindowsTcpConnectionState::createContext(const int errorCode, text::String title, text::String description) const
    -> NetworkErrorContext {
    auto context = NetworkErrorContext{std::move(title), std::move(description)};
    context.setReason(errorReason(errorCode))
        .setPlatformContext(system::WindowsErrorContext::fromErrorCode(static_cast<DWORD>(errorCode)));
    if (_localEndpoint.has_value()) {
        context.setLocalEndpoint(*_localEndpoint);
    }
    if (_remoteEndpoint.has_value()) {
        const auto &remote = *_remoteEndpoint;
        context.setRemoteEndpoint(HostEndpoint{remote.address(), remote.port(), remote.scopeId()});
    }
    return context;
}

auto WindowsTcpConnectionState::errorReason(const int errorCode) noexcept -> NetworkErrorReason {
    switch (errorCode) {
    case WSAEADDRINUSE:
        return NetworkErrorReason::AddressInUse;
    case WSAEACCES:
        return NetworkErrorReason::PermissionDenied;
    case WSAENETUNREACH:
    case WSAEHOSTUNREACH:
    case ERROR_NETWORK_UNREACHABLE:
    case ERROR_HOST_UNREACHABLE:
        return NetworkErrorReason::NetworkUnreachable;
    case WSAECONNREFUSED:
    case ERROR_CONNECTION_REFUSED:
        return NetworkErrorReason::ConnectionRefused;
    case WSAECONNRESET:
    case WSAECONNABORTED:
    case WSAESHUTDOWN:
        return NetworkErrorReason::ConnectionReset;
    case WSAETIMEDOUT:
    case WAIT_TIMEOUT:
        return NetworkErrorReason::Timeout;
    default:
        return NetworkErrorReason::SocketOperationFailed;
    }
}

WindowsTcpConnectionDevice::WindowsTcpConnectionDevice(
    event::EventLoopDriverPtr driver,
    const unit::ByteLength receiveChunkLimit,
    TcpConnectionDeviceCallbacks callbacks) :
    _state{std::make_shared<WindowsTcpConnectionState>(std::move(driver), receiveChunkLimit, std::move(callbacks))} {
}

WindowsTcpConnectionDevice::~WindowsTcpConnectionDevice() {
    close();
}

void WindowsTcpConnectionDevice::connect(IpEndpoint remoteEndpoint) {
    _state->connect(std::move(remoteEndpoint));
}

void WindowsTcpConnectionDevice::accept(TcpAcceptedSocketPtr socket) {
    _state->accept(std::move(socket));
}

auto WindowsTcpConnectionDevice::canAccept(const TcpAcceptedSocket &socket) const noexcept -> bool {
    return dynamic_cast<const WindowsTcpSocket *>(&socket) != nullptr;
}

auto WindowsTcpConnectionDevice::send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus {
    return _state->send(std::move(data));
}

void WindowsTcpConnectionDevice::setReceiving(const unit::ByteLength maximumBytes) {
    _state->setReceiving(maximumBytes);
}

void WindowsTcpConnectionDevice::close() noexcept {
    _state->close();
}

void WindowsTcpConnectionDevice::abort() noexcept {
    _state->close();
}

auto TcpConnectionDevice::create(
    event::EventLoopDriverPtr driver, const unit::ByteLength receiveChunkLimit, TcpConnectionDeviceCallbacks callbacks)
    -> TcpConnectionDevicePtr {
    return std::make_unique<WindowsTcpConnectionDevice>(std::move(driver), receiveChunkLimit, std::move(callbacks));
}

}
