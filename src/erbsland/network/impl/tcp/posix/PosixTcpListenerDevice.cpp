// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixTcpListenerDevice.hpp"

#include "PosixTcpSocket.hpp"

#include "../../../../err/LogicFailure.hpp"
#include "../../../../err/ParameterFailure.hpp"
#include "../../../source/NetworkError.hpp"
#include "../../platform/SocketAddress.hpp"

#ifdef ERBSLAND_OS_MACOS
#include "../../../../event/impl/KqueueEventLoopDriver.hpp"
#elif defined(ERBSLAND_OS_LINUX)
#include "../../../../event/impl/EpollEventLoopDriver.hpp"
#endif
#include "../../../../system/impl/PosixErrorContext.hpp"
#include "../../../../text/Literals.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <limits>
#include <memory>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

PosixTcpListenerDevice::PosixTcpListenerDevice(
    PrivateTag, RegisterFn registerFn, TcpListenerDeviceCallbacks callbacks) :
    _register{std::move(registerFn)}, _callbacks{std::move(callbacks)} {
}

PosixTcpListenerDevice::~PosixTcpListenerDevice() {
    close();
}

auto PosixTcpListenerDevice::start(IpEndpoint localEndpoint, const unit::ItemCount backlog) -> IpEndpoint {
    auto lock = std::unique_lock{_operationMutex};
    if (_closed) {
        throw err::LogicFailure{"Cannot start a closed native socket."_el};
    }
    const auto family = localEndpoint.address().isV4() ? AF_INET : AF_INET6;
    const auto descriptor = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (descriptor < 0) {
        throw NetworkError{createContext(
            errno, "TCP listener creation failed"_el, "The native listening socket could not be created."_el)};
    }
    _descriptor = descriptor;
    try {
        configureDescriptor(descriptor);
    } catch (...) {
        closeLocked();
        throw;
    }
    constexpr auto enabled = int{1};
    if (::setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)) < 0) {
        const auto error = NetworkError{
            createContext(errno, "TCP listener setup failed"_el, "Address reuse could not be configured."_el)};
        closeLocked();
        throw error;
    }
    if (family == AF_INET6 && ::setsockopt(descriptor, IPPROTO_IPV6, IPV6_V6ONLY, &enabled, sizeof(enabled)) < 0) {
        const auto error = NetworkError{
            createContext(errno, "TCP listener setup failed"_el, "IPv6-only mode could not be configured."_el)};
        closeLocked();
        throw error;
    }
    const auto nativeEndpoint = SocketAddress::fromEndpoint(localEndpoint);
    if (::bind(descriptor, nativeEndpoint.data(), static_cast<socklen_t>(nativeEndpoint.size())) < 0) {
        auto context = createContext(
            errno, "TCP listener bind failed"_el, "The listener could not bind to the local endpoint."_el);
        context.setLocalEndpoint(localEndpoint);
        closeLocked();
        throw NetworkError{std::move(context)};
    }
    const auto maximumBacklog = static_cast<std::size_t>(std::numeric_limits<int>::max());
    const auto nativeBacklog = static_cast<int>(std::min(backlog.toSizeT(), maximumBacklog));
    if (::listen(descriptor, nativeBacklog) < 0) {
        const auto error = NetworkError{
            createContext(errno, "TCP listen failed"_el, "The native socket could not enter listening mode."_el)};
        closeLocked();
        throw error;
    }
    auto actualAddress = SocketAddress{};
    auto actualSize = static_cast<socklen_t>(actualAddress.size());
    if (::getsockname(descriptor, actualAddress.data(), &actualSize) < 0) {
        const auto error = NetworkError{createContext(
            errno, "TCP listener endpoint query failed"_el, "The bound endpoint could not be determined."_el)};
        closeLocked();
        throw error;
    }
    *actualAddress.sizePointer() = static_cast<int>(actualSize);
    const auto actualEndpoint = actualAddress.toEndpoint();
    if (!actualEndpoint.has_value()) {
        closeLocked();
        throw NetworkError{NetworkErrorContext{
            "TCP listener endpoint query failed"_el, "The platform returned an unsupported address family."_el}
                .setReason(NetworkErrorReason::SocketOperationFailed)};
    }
    _localEndpoint = *actualEndpoint;
    return *actualEndpoint;
}

void PosixTcpListenerDevice::setAccepting(const bool enabled) {
    auto lock = std::unique_lock{_operationMutex};
    if (_closed) {
        return;
    }
    _accepting = enabled;
    updateRegistration();
}

void PosixTcpListenerDevice::close() noexcept {
    const auto lock = std::scoped_lock{_operationMutex};
    closeLocked();
}

void PosixTcpListenerDevice::abort() noexcept {
    close();
}

void PosixTcpListenerDevice::closeLocked() noexcept {
    _closed = true;
    _registration.reset();
    const auto descriptor = std::exchange(_descriptor, -1);
    if (descriptor >= 0) {
        // Never retry close: the descriptor number may already have been reused.
        ::close(descriptor);
    }
}

auto PosixTcpListenerDevice::createRegisterFn(event::EventLoopDriverPtr driver) -> RegisterFn {
#ifdef ERBSLAND_OS_MACOS
    const auto nativeDriver = std::dynamic_pointer_cast<event::impl::KqueueEventLoopDriver>(std::move(driver));
#elif defined(ERBSLAND_OS_LINUX)
    const auto nativeDriver = std::dynamic_pointer_cast<event::impl::EpollEventLoopDriver>(std::move(driver));
#endif
    if (nativeDriver == nullptr) {
        throw err::ParameterFailure{
            "TCP listeners require the platform's default native event-loop driver."_el, "driver"_el};
    }
    return [nativeDriver](const int descriptor, const bool read, const bool write, auto callback) {
        return nativeDriver->registerDescriptor(descriptor, read, write, std::move(callback));
    };
}

void PosixTcpListenerDevice::configureDescriptor(const int descriptor) {
    if (::fcntl(descriptor, F_SETFL, ::fcntl(descriptor, F_GETFL, 0) | O_NONBLOCK) < 0 ||
        ::fcntl(descriptor, F_SETFD, ::fcntl(descriptor, F_GETFD, 0) | FD_CLOEXEC) < 0) {
        const auto context = system::impl::PosixErrorContext::fromErrno();
        throw NetworkError{NetworkErrorContext{
            "TCP socket configuration failed"_el, "The native TCP socket could not be configured as non-blocking."_el}
                .setReason(NetworkErrorReason::SocketOperationFailed)
                .setPlatformContext(context)};
    }
#ifdef SO_NOSIGPIPE
    constexpr auto enabled = int{1};
    if (::setsockopt(descriptor, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) < 0) {
        const auto context = system::impl::PosixErrorContext::fromErrno();
        throw NetworkError{NetworkErrorContext{
            "TCP socket configuration failed"_el, "SIGPIPE suppression could not be enabled for the TCP socket."_el}
                .setReason(NetworkErrorReason::SocketOperationFailed)
                .setPlatformContext(context)};
    }
#endif
}

void PosixTcpListenerDevice::updateRegistration() {
    _registration.reset();
    const auto descriptor = _descriptor;
    if (descriptor < 0 || !_accepting) {
        return;
    }
    _registration = _register(
        descriptor,
        true,
        false,
        [weak = weak_from_this()](const bool readable, [[maybe_unused]] const bool writable, const bool error) {
            if (const auto self = weak.lock()) {
                self->handleReady(readable, error);
            }
        });
}

void PosixTcpListenerDevice::handleReady(const bool readable, const bool error) {
    auto lock = std::unique_lock{_operationMutex};
    if (_closed) {
        return;
    }
    if (error) {
        auto errorCode = int{};
        auto size = static_cast<socklen_t>(sizeof(errorCode));
        if (::getsockopt(_descriptor, SOL_SOCKET, SO_ERROR, &errorCode, &size) == 0 && errorCode != 0) {
            if (_callbacks.error) {
                notify(
                    lock,
                    _callbacks.error,
                    createContext(
                        errorCode, "TCP listener failed"_el, "The native listener reported an operational error."_el));
            }
            return;
        }
    }
    if (readable && _accepting) {
        acceptReady(lock);
    }
}

void PosixTcpListenerDevice::acceptReady(std::unique_lock<std::mutex> &lock) {
    const auto descriptor = _descriptor;
    while (!_closed && _accepting && _descriptor >= 0) {
        auto remoteAddress = SocketAddress{};
        auto remoteSize = static_cast<socklen_t>(remoteAddress.size());
        const auto accepted = ::accept(descriptor, remoteAddress.data(), &remoteSize);
        if (accepted < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            if (_callbacks.error) {
                notify(
                    lock,
                    _callbacks.error,
                    createContext(
                        errno, "TCP accept failed"_el, "An incoming TCP connection could not be accepted."_el));
            }
            return;
        }
        try {
            configureDescriptor(accepted);
        } catch (...) {
            // Preserve the setup error; a close failure cannot safely be retried.
            ::close(accepted);
            throw;
        }
        *remoteAddress.sizePointer() = static_cast<int>(remoteSize);
        const auto remoteEndpoint = remoteAddress.toEndpoint();
        if (!remoteEndpoint.has_value()) {
            // The unsupported peer cannot be accepted; a close failure cannot safely be retried.
            ::close(accepted);
            continue;
        }
        if (_callbacks.accepted) {
            notify(
                lock,
                _callbacks.accepted,
                std::make_unique<PosixTcpSocket>(accepted, *_localEndpoint, *remoteEndpoint));
        } else {
            // No callback can take ownership; a close failure cannot safely be retried.
            ::close(accepted);
        }
    }
}

auto PosixTcpListenerDevice::createContext(const int errorCode, text::String title, text::String description) const
    -> NetworkErrorContext {
    auto context = NetworkErrorContext{std::move(title), std::move(description)};
    context.setReason(errorReason(errorCode))
        .setPlatformContext(system::impl::PosixErrorContext::fromErrorCode(errorCode));
    if (_localEndpoint.has_value()) {
        context.setLocalEndpoint(*_localEndpoint);
    }
    return context;
}

auto PosixTcpListenerDevice::errorReason(const int errorCode) noexcept -> NetworkErrorReason {
    switch (errorCode) {
    case EADDRINUSE:
        return NetworkErrorReason::AddressInUse;
    case EACCES:
    case EPERM:
        return NetworkErrorReason::PermissionDenied;
    default:
        return NetworkErrorReason::SocketOperationFailed;
    }
}

auto PosixTcpListenerDevice::create(event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks)
    -> PosixTcpListenerDevicePtr {
    return create(createRegisterFn(std::move(driver)), std::move(callbacks));
}

auto PosixTcpListenerDevice::create(RegisterFn registerFn, TcpListenerDeviceCallbacks callbacks)
    -> PosixTcpListenerDevicePtr {
    return std::make_shared<PosixTcpListenerDevice>(PrivateTag{}, std::move(registerFn), std::move(callbacks));
}

auto TcpListenerDevice::create(event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks)
    -> TcpListenerDevicePtr {
    return PosixTcpListenerDevice::create(std::move(driver), std::move(callbacks));
}

}
