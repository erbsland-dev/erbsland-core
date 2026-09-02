// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixUdpSocketDevice.hpp"

#include "../../../../err/RuntimeError.hpp"
#include "../../../../event/impl/EventLoopDriverRegistration.hpp"
#include "../../../source/NetworkError.hpp"
#include "../../platform/SocketAddress.hpp"

#ifdef ERBSLAND_OS_MACOS
#include "../../../../event/impl/KqueueEventLoopDriver.hpp"
#elif defined(ERBSLAND_OS_LINUX)
#include "../../../../event/impl/EpollEventLoopDriver.hpp"
#endif
#include "../../../../mem/impl/UnsafeByteBlockAccess.hpp"
#include "../../../../mem/impl/UnsafeByteBlockBuffer.hpp"
#include "../../../../system/impl/PosixErrorContext.hpp"
#include "../../../../text/Literals.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

PosixUdpSocketDevice::PosixUdpSocketDevice(event::EventLoopDriverPtr driver, UdpSocketDeviceCallbacks callbacks) :
    _register{createRegisterFn(std::move(driver))}, _callbacks{std::move(callbacks)} {
}

PosixUdpSocketDevice::~PosixUdpSocketDevice() {
    close();
}

auto PosixUdpSocketDevice::bind(IpEndpoint localEndpoint, const unit::ByteLength maximumDatagramSize) -> IpEndpoint {
    const auto family = localEndpoint.address().isV4() ? AF_INET : AF_INET6;
    const auto descriptor = ::socket(family, SOCK_DGRAM, IPPROTO_UDP);
    if (descriptor < 0) {
        throw createError("UDP socket creation failed"_el, "The native UDP socket could not be created."_el);
    }
    _descriptor.store(descriptor);
    if (::fcntl(descriptor, F_SETFL, ::fcntl(descriptor, F_GETFL, 0) | O_NONBLOCK) < 0 ||
        ::fcntl(descriptor, F_SETFD, ::fcntl(descriptor, F_GETFD, 0) | FD_CLOEXEC) < 0) {
        const auto error = createError(
            "UDP socket setup failed"_el, "The native UDP socket could not be configured as non-blocking."_el);
        abort();
        throw error;
    }
    if (family == AF_INET6) {
        constexpr auto enabled = int{1};
        if (::setsockopt(descriptor, IPPROTO_IPV6, IPV6_V6ONLY, &enabled, sizeof(enabled)) < 0) {
            const auto error =
                createError("UDP socket setup failed"_el, "The IPv6-only socket mode could not be configured."_el);
            abort();
            throw error;
        }
    }
    const auto nativeEndpoint = SocketAddress::fromEndpoint(localEndpoint);
    if (::bind(descriptor, nativeEndpoint.data(), static_cast<socklen_t>(nativeEndpoint.size())) < 0) {
        const auto error = createError("UDP bind failed"_el, "The UDP socket could not bind to the local endpoint."_el);
        auto context = error.context();
        context.setLocalEndpoint(localEndpoint);
        abort();
        throw NetworkError{std::move(context)};
    }
    auto actualEndpoint = SocketAddress{};
    auto nativeSize = static_cast<socklen_t>(actualEndpoint.size());
    if (::getsockname(descriptor, actualEndpoint.data(), &nativeSize) < 0) {
        const auto error =
            createError("UDP endpoint query failed"_el, "The bound UDP endpoint could not be determined."_el);
        abort();
        throw error;
    }
    *actualEndpoint.sizePointer() = static_cast<int>(nativeSize);
    const auto convertedEndpoint = actualEndpoint.toEndpoint();
    if (!convertedEndpoint.has_value()) {
        abort();
        throw NetworkError{NetworkErrorContext{
            "UDP endpoint query failed"_el, "The platform returned an unsupported bound address family."_el}
                .setReason(NetworkErrorReason::SocketOperationFailed)
                .setLocalEndpoint(localEndpoint)};
    }
    _localEndpoint = *convertedEndpoint;
    _maximumDatagramSize = maximumDatagramSize;
    return *convertedEndpoint;
}

auto PosixUdpSocketDevice::send(const UdpDatagram &datagram) -> UdpSocketDeviceSendStatus {
    const auto descriptor = _descriptor.load();
    if (descriptor < 0) {
        return UdpSocketDeviceSendStatus::WouldBlock;
    }
    const auto destination = SocketAddress::fromEndpoint(datagram.remoteEndpoint());
    const auto data = mem::impl::UnsafeByteBlockAccess{datagram.data()}.dataView().dataSpan();
    auto vector = iovec{.iov_base = const_cast<mem::Byte *>(data.data()), .iov_len = data.size()};
    auto message = msghdr{};
    message.msg_name = const_cast<sockaddr *>(destination.data());
    message.msg_namelen = static_cast<socklen_t>(destination.size());
    message.msg_iov = &vector;
    message.msg_iovlen = 1;
#ifdef MSG_NOSIGNAL
    constexpr auto flags = MSG_NOSIGNAL;
#else
    constexpr auto flags = 0;
#endif
    const auto result = ::sendmsg(descriptor, &message, flags);
    if (result >= 0) {
        return UdpSocketDeviceSendStatus::Complete;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        _write = true;
        updateRegistration();
        return UdpSocketDeviceSendStatus::WouldBlock;
    }
    const auto error = createError("UDP send failed"_el, "The datagram could not be sent."_el);
    auto context = error.context();
    const auto &remote = datagram.remoteEndpoint();
    context.setRemoteEndpoint(HostEndpoint{remote.address(), remote.port(), remote.scopeId()});
    throw NetworkError{std::move(context)};
}

void PosixUdpSocketDevice::setReceiving(const bool enabled) {
    _read = enabled;
    updateRegistration();
}

void PosixUdpSocketDevice::close() noexcept {
    _registration.reset();
    abort();
}

void PosixUdpSocketDevice::abort() noexcept {
    const auto descriptor = _descriptor.exchange(-1);
    if (descriptor >= 0) {
        // A close failure cannot be retried because the descriptor may already have been reused.
        ::close(descriptor);
    }
}

auto PosixUdpSocketDevice::createRegisterFn(event::EventLoopDriverPtr driver) -> RegisterFn {
#ifdef ERBSLAND_OS_MACOS
    const auto nativeDriver = std::dynamic_pointer_cast<event::impl::KqueueEventLoopDriver>(std::move(driver));
#elif defined(ERBSLAND_OS_LINUX)
    const auto nativeDriver = std::dynamic_pointer_cast<event::impl::EpollEventLoopDriver>(std::move(driver));
#endif
    if (nativeDriver == nullptr) {
        throw err::RuntimeError{"The UDP socket requires the platform's default native event-loop driver."_el};
    }
    return [nativeDriver](const int descriptor, const bool read, const bool write, auto callback) {
        return nativeDriver->registerDescriptor(descriptor, read, write, std::move(callback));
    };
}

void PosixUdpSocketDevice::updateRegistration() {
    _registration.reset();
    const auto descriptor = _descriptor.load();
    if (descriptor < 0 || (!_read && !_write)) {
        return;
    }
    _registration = _register(
        descriptor, _read, _write, [this](const bool readable, const bool writable, const bool error) -> void {
            handleReady(readable, writable, error);
        });
}

void PosixUdpSocketDevice::handleReady(const bool readable, const bool writable, const bool error) {
    const auto descriptor = _descriptor.load();
    if (descriptor < 0) {
        return;
    }
    if (error) {
        auto errorCode = int{};
        auto size = static_cast<socklen_t>(sizeof(errorCode));
        if (::getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &errorCode, &size) == 0 && errorCode != 0) {
            if (_callbacks.error) {
                _callbacks.error(createContext(
                    errorCode, "UDP socket failed"_el, "The native UDP socket reported an operational error."_el));
            }
            return;
        }
    }
    if (readable && _read) {
        receiveOne();
    }
    if (writable && _write) {
        _write = false;
        updateRegistration();
        if (_callbacks.writable) {
            _callbacks.writable();
        }
    }
}

void PosixUdpSocketDevice::receiveOne() {
    const auto descriptor = _descriptor.load();
    if (descriptor < 0) {
        return;
    }
    auto buffer = mem::impl::UnsafeByteBlockBuffer{_maximumDatagramSize};
    auto bytes = buffer.data();
    auto remoteAddress = SocketAddress{};
    auto vector = iovec{.iov_base = bytes.data(), .iov_len = bytes.size()};
    auto message = msghdr{};
    message.msg_name = remoteAddress.data();
    message.msg_namelen = static_cast<socklen_t>(remoteAddress.size());
    message.msg_iov = &vector;
    message.msg_iovlen = 1;
    const auto result = ::recvmsg(descriptor, &message, 0);
    if (result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        if (_callbacks.error) {
            _callbacks.error(createContext(
                errno, "UDP receive failed"_el, "A datagram could not be received from the native socket."_el));
        }
        return;
    }
    *remoteAddress.sizePointer() = static_cast<int>(message.msg_namelen);
    const auto remoteEndpoint = remoteAddress.toEndpoint();
    if (!remoteEndpoint.has_value()) {
        if (_callbacks.error) {
            _callbacks.error(
                NetworkErrorContext{
                    "UDP receive failed"_el, "The platform returned an unsupported remote address family."_el}
                    .setReason(NetworkErrorReason::SocketOperationFailed));
        }
        return;
    }
    if ((message.msg_flags & MSG_TRUNC) != 0 || static_cast<std::size_t>(result) > bytes.size()) {
        if (_callbacks.datagramDropped) {
            const auto actualSize = static_cast<std::size_t>(result) > bytes.size()
                ? std::optional<unit::ByteLength>{unit::ByteLength::fromSizeT(static_cast<std::size_t>(result))}
                : std::optional<unit::ByteLength>{};
            _callbacks.datagramDropped(
                UdpDatagramDropContext{
                    UdpDatagramDropReason::TooLarge, _maximumDatagramSize, *remoteEndpoint, actualSize});
        }
        return;
    }
    if (_callbacks.datagram) {
        _callbacks.datagram(
            UdpDatagram{*remoteEndpoint, buffer.take(unit::ByteLength::fromSizeT(static_cast<std::size_t>(result)))});
    }
}

auto PosixUdpSocketDevice::createError(text::String title, text::String description) const -> NetworkError {
    return NetworkError{createContext(errno, std::move(title), std::move(description))};
}

auto PosixUdpSocketDevice::createContext(const int errorCode, text::String title, text::String description) const
    -> NetworkErrorContext {
    auto context = NetworkErrorContext{std::move(title), std::move(description)};
    context.setReason(errorReason(errorCode))
        .setPlatformContext(system::impl::PosixErrorContext::fromErrorCode(errorCode));
    if (_localEndpoint.has_value()) {
        context.setLocalEndpoint(*_localEndpoint);
    }
    return context;
}

auto PosixUdpSocketDevice::errorReason(const int errorCode) noexcept -> NetworkErrorReason {
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
    case EMSGSIZE:
        return NetworkErrorReason::MessageTooLarge;
    default:
        return NetworkErrorReason::SocketOperationFailed;
    }
}

auto UdpSocketDevice::createUdpSocketDevice(event::EventLoopDriverPtr driver, UdpSocketDeviceCallbacks callbacks)
    -> UdpSocketDevicePtr {
    return std::make_unique<PosixUdpSocketDevice>(std::move(driver), std::move(callbacks));
}

}
