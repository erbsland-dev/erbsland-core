// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixTcpConnectionDevice.hpp"

#include "PosixTcpSocket.hpp"
#include "SocketAddress.hpp"

#include "../source/NetworkError.hpp"

#include "../../err/RuntimeError.hpp"

#ifdef ERBSLAND_OS_MACOS
#include "../../event/impl/KqueueEventLoopDriver.hpp"
#elif defined(ERBSLAND_OS_LINUX)
#include "../../event/impl/EpollEventLoopDriver.hpp"
#endif
#include "../../mem/impl/UnsafeByteBlockAccess.hpp"
#include "../../mem/impl/UnsafeByteBlockBuffer.hpp"
#include "../../system/PosixErrorContext.hpp"
#include "../../text/Literals.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

PosixTcpConnectionDevice::PosixTcpConnectionDevice(
    event::EventLoopDriverPtr driver,
    const unit::ByteLength receiveChunkLimit,
    TcpConnectionDeviceCallbacks callbacks) :
    _register{createRegisterFn(std::move(driver))},
    _callbacks{std::move(callbacks)},
    _receiveChunkLimit{receiveChunkLimit} {
}

PosixTcpConnectionDevice::~PosixTcpConnectionDevice() {
    close();
}

void PosixTcpConnectionDevice::connect(IpEndpoint remoteEndpoint) {
    const auto family = remoteEndpoint.address().isV4() ? AF_INET : AF_INET6;
    const auto descriptor = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (descriptor < 0) {
        throw NetworkError{
            createContext(errno, "TCP socket creation failed"_el, "The native TCP socket could not be created."_el)};
    }
    _descriptor.store(descriptor);
    try {
        configureDescriptor(descriptor);
    } catch (...) {
        abort();
        throw;
    }
    _remoteEndpoint = remoteEndpoint;
    const auto nativeEndpoint = SocketAddress::fromEndpoint(remoteEndpoint);
    if (::connect(descriptor, nativeEndpoint.data(), static_cast<socklen_t>(nativeEndpoint.size())) == 0) {
        _connected = true;
        _localEndpoint = queryLocalEndpoint();
        if (_callbacks.connected) {
            _callbacks.connected(*_localEndpoint, remoteEndpoint);
        }
        return;
    }
    if (errno != EINPROGRESS) {
        const auto error = NetworkError{createContext(
            errno, "TCP connection failed"_el, "The native socket could not connect to the remote endpoint."_el)};
        abort();
        throw error;
    }
    _connecting = true;
    updateRegistration();
}

void PosixTcpConnectionDevice::accept(TcpAcceptedSocketPtr socket) {
    auto *posixSocket = dynamic_cast<PosixTcpSocket *>(socket.get());
    if (posixSocket == nullptr) {
        throw err::RuntimeError{"The accepted TCP socket is incompatible with the POSIX connection device."_el};
    }
    _localEndpoint = posixSocket->localEndpoint();
    _remoteEndpoint = posixSocket->remoteEndpoint();
    _descriptor.store(posixSocket->takeDescriptor());
    socket.reset();
    _connected = true;
    if (_callbacks.connected) {
        _callbacks.connected(*_localEndpoint, *_remoteEndpoint);
    }
}

auto PosixTcpConnectionDevice::canAccept(const TcpAcceptedSocket &socket) const noexcept -> bool {
    return dynamic_cast<const PosixTcpSocket *>(&socket) != nullptr;
}

auto PosixTcpConnectionDevice::send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus {
    if (_sendPending) {
        throw err::RuntimeError{"The native TCP device already retains an output block."_el};
    }
    _sendData = std::move(data);
    _sendOffset = 0U;
    if (writePending()) {
        _sendData = {};
        return TcpConnectionDeviceSendStatus::Complete;
    }
    _sendPending = true;
    updateRegistration();
    return TcpConnectionDeviceSendStatus::Pending;
}

void PosixTcpConnectionDevice::setReceiving(const unit::ByteLength maximumBytes) {
    _maximumReceiveSize = std::min(maximumBytes, _receiveChunkLimit);
    updateRegistration();
}

void PosixTcpConnectionDevice::close() noexcept {
    _registration.reset();
    abort();
}

void PosixTcpConnectionDevice::abort() noexcept {
    const auto descriptor = _descriptor.exchange(-1);
    if (descriptor >= 0) {
        // A close failure cannot be retried because the descriptor may already have been reused.
        ::close(descriptor);
    }
}

auto PosixTcpConnectionDevice::createRegisterFn(event::EventLoopDriverPtr driver) -> RegisterFn {
#ifdef ERBSLAND_OS_MACOS
    const auto nativeDriver = std::dynamic_pointer_cast<event::impl::KqueueEventLoopDriver>(std::move(driver));
#elif defined(ERBSLAND_OS_LINUX)
    const auto nativeDriver = std::dynamic_pointer_cast<event::impl::EpollEventLoopDriver>(std::move(driver));
#endif
    if (nativeDriver == nullptr) {
        throw err::RuntimeError{"TCP sockets require the platform's default native event-loop driver."_el};
    }
    return [nativeDriver](const int descriptor, const bool read, const bool write, auto callback) {
        return nativeDriver->registerDescriptor(descriptor, read, write, std::move(callback));
    };
}

void PosixTcpConnectionDevice::configureDescriptor(const int descriptor) {
    if (::fcntl(descriptor, F_SETFL, ::fcntl(descriptor, F_GETFL, 0) | O_NONBLOCK) < 0 ||
        ::fcntl(descriptor, F_SETFD, ::fcntl(descriptor, F_GETFD, 0) | FD_CLOEXEC) < 0) {
        throw err::RuntimeError{"The native TCP socket could not be configured as non-blocking."_el};
    }
#ifdef SO_NOSIGPIPE
    constexpr auto enabled = int{1};
    if (::setsockopt(descriptor, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) < 0) {
        throw err::RuntimeError{"SIGPIPE suppression could not be enabled for the TCP socket."_el};
    }
#endif
}

void PosixTcpConnectionDevice::updateRegistration() {
    _registration.reset();
    const auto descriptor = _descriptor.load();
    const auto read = _connected && !_maximumReceiveSize.isZero();
    const auto write = _connecting || _sendPending;
    if (descriptor < 0 || (!read && !write)) {
        return;
    }
    _registration =
        _register(descriptor, read, write, [this](const bool readable, const bool writable, const bool error) -> void {
            handleReady(readable, writable, error);
        });
}

void PosixTcpConnectionDevice::handleReady(const bool readable, const bool writable, const bool error) {
    const auto descriptor = _descriptor.load();
    if (descriptor < 0) {
        return;
    }
    if (_connecting && (writable || error)) {
        finishConnect();
        return;
    }
    if (error) {
        auto errorCode = int{};
        auto size = static_cast<socklen_t>(sizeof(errorCode));
        if (::getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &errorCode, &size) == 0 && errorCode != 0) {
            if (_callbacks.error) {
                _callbacks.error(createContext(
                    errorCode, "TCP socket failed"_el, "The native TCP socket reported an operational error."_el));
            }
            return;
        }
    }
    if (readable && !_maximumReceiveSize.isZero()) {
        receiveOne();
    }
    if (writable && _sendPending) {
        try {
            if (writePending()) {
                _sendPending = false;
                _sendData = {};
                updateRegistration();
                if (_callbacks.sendCompleted) {
                    _callbacks.sendCompleted();
                }
            }
        } catch (const NetworkError &sendError) {
            if (_callbacks.error) {
                _callbacks.error(sendError.context());
            }
        }
    }
}

void PosixTcpConnectionDevice::finishConnect() {
    const auto descriptor = _descriptor.load();
    auto errorCode = int{};
    auto size = static_cast<socklen_t>(sizeof(errorCode));
    if (::getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &errorCode, &size) < 0) {
        errorCode = errno;
    }
    _connecting = false;
    if (errorCode != 0) {
        updateRegistration();
        if (_callbacks.error) {
            _callbacks.error(createContext(
                errorCode, "TCP connection failed"_el, "The remote endpoint rejected or failed the connection."_el));
        }
        return;
    }
    _connected = true;
    try {
        _localEndpoint = queryLocalEndpoint();
    } catch (const NetworkError &endpointError) {
        if (_callbacks.error) {
            _callbacks.error(endpointError.context());
        }
        return;
    }
    updateRegistration();
    if (_callbacks.connected) {
        _callbacks.connected(*_localEndpoint, *_remoteEndpoint);
    }
}

auto PosixTcpConnectionDevice::writePending() -> bool {
    const auto descriptor = _descriptor.load();
    const auto bytes = mem::impl::UnsafeByteBlockAccess{_sendData}.dataView().dataSpan();
    while (_sendOffset < bytes.size()) {
#ifdef MSG_NOSIGNAL
        constexpr auto flags = MSG_NOSIGNAL;
#else
        constexpr auto flags = 0;
#endif
        const auto result = ::send(descriptor, bytes.data() + _sendOffset, bytes.size() - _sendOffset, flags);
        if (result > 0) {
            _sendOffset += static_cast<std::size_t>(result);
            continue;
        }
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return false;
        }
        throw NetworkError{
            createContext(result < 0 ? errno : EPIPE, "TCP send failed"_el, "Stream data could not be sent."_el)};
    }
    return true;
}

void PosixTcpConnectionDevice::receiveOne() {
    const auto descriptor = _descriptor.load();
    auto buffer = mem::impl::UnsafeByteBlockBuffer{_maximumReceiveSize};
    auto bytes = buffer.data();
    const auto result = ::recv(descriptor, bytes.data(), bytes.size(), 0);
    if (result > 0) {
        if (_callbacks.data) {
            _callbacks.data(buffer.take(unit::ByteLength::fromSizeT(static_cast<std::size_t>(result))));
        }
        return;
    }
    if (result == 0) {
        _maximumReceiveSize = {};
        updateRegistration();
        if (_callbacks.remoteClosed) {
            _callbacks.remoteClosed();
        }
        return;
    }
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
    }
    if (_callbacks.error) {
        _callbacks.error(createContext(errno, "TCP receive failed"_el, "Stream data could not be received."_el));
    }
}

auto PosixTcpConnectionDevice::queryLocalEndpoint() const -> IpEndpoint {
    const auto descriptor = _descriptor.load();
    auto address = SocketAddress{};
    auto size = static_cast<socklen_t>(address.size());
    if (::getsockname(descriptor, address.data(), &size) < 0) {
        throw NetworkError{createContext(
            errno, "TCP endpoint query failed"_el, "The connected local endpoint could not be determined."_el)};
    }
    *address.sizePointer() = static_cast<int>(size);
    const auto endpoint = address.toEndpoint();
    if (!endpoint.has_value()) {
        throw NetworkError{NetworkErrorContext{
            "TCP endpoint query failed"_el, "The platform returned an unsupported local address family."_el}
                .setReason(NetworkErrorReason::SocketOperationFailed)};
    }
    return *endpoint;
}

auto PosixTcpConnectionDevice::createContext(const int errorCode, text::String title, text::String description) const
    -> NetworkErrorContext {
    auto context = NetworkErrorContext{std::move(title), std::move(description)};
    context.setReason(errorReason(errorCode)).setPlatformContext(system::PosixErrorContext::fromErrorCode(errorCode));
    if (_localEndpoint.has_value()) {
        context.setLocalEndpoint(*_localEndpoint);
    }
    if (_remoteEndpoint.has_value()) {
        const auto &remote = *_remoteEndpoint;
        context.setRemoteEndpoint(HostEndpoint{remote.address(), remote.port(), remote.scopeId()});
    }
    return context;
}

auto PosixTcpConnectionDevice::errorReason(const int errorCode) noexcept -> NetworkErrorReason {
    switch (errorCode) {
    case EADDRINUSE:
        return NetworkErrorReason::AddressInUse;
    case EACCES:
    case EPERM:
        return NetworkErrorReason::PermissionDenied;
    case ENETUNREACH:
    case EHOSTUNREACH:
        return NetworkErrorReason::NetworkUnreachable;
    case ECONNREFUSED:
        return NetworkErrorReason::ConnectionRefused;
    case ECONNRESET:
    case EPIPE:
        return NetworkErrorReason::ConnectionReset;
    case ETIMEDOUT:
        return NetworkErrorReason::Timeout;
    default:
        return NetworkErrorReason::SocketOperationFailed;
    }
}

auto TcpConnectionDevice::create(
    event::EventLoopDriverPtr driver, const unit::ByteLength receiveChunkLimit, TcpConnectionDeviceCallbacks callbacks)
    -> TcpConnectionDevicePtr {
    return std::make_unique<PosixTcpConnectionDevice>(std::move(driver), receiveChunkLimit, std::move(callbacks));
}

}
