// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EpollEventLoopDriver.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../system/impl/PosixErrorContext.hpp"
#include "../../system/PlatformError.hpp"
#include "../../system/PlatformFailure.hpp"
#include "../../text/Literals.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

namespace erbsland::event::impl {

using namespace err;
using namespace text::literals;

EpollEventLoopDriver::EpollEventLoopDriver() : EpollEventLoopDriver{std::make_shared<EpollEventLoopNativeCalls>()} {
}

EpollEventLoopDriver::EpollEventLoopDriver(std::shared_ptr<EpollEventLoopNativeCalls> nativeCalls) :
    _nativeCalls{std::move(nativeCalls)} {
    if (_nativeCalls == nullptr) {
        throw err::ParameterFailure{"The native call adapter must not be null."_el, "nativeCalls"_el};
    }
    _epoll = ::epoll_create1(EPOLL_CLOEXEC);
    if (_epoll < 0) {
        throw system::PlatformError{
            "Failed to create the epoll event-loop driver."_el, system::impl::PosixErrorContext::fromErrno()};
    }
    _wake = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (_wake < 0) {
        const auto context = system::impl::PosixErrorContext::fromErrno();
        ::close(_epoll);
        throw system::PlatformError{"Failed to create the epoll wake descriptor."_el, context};
    }
    auto event = epoll_event{.events = EPOLLIN, .data = {.u64 = cWakeGeneration}};
    if (_nativeCalls->control(_epoll, EPOLL_CTL_ADD, _wake, &event) < 0) {
        const auto context = system::impl::PosixErrorContext::fromErrno();
        ::close(_wake);
        ::close(_epoll);
        _wake = -1;
        _epoll = -1;
        throw system::PlatformError{"Failed to register the epoll wake descriptor."_el, context};
    }
}

EpollEventLoopDriver::~EpollEventLoopDriver() {
    if (_wake >= 0) {
        ::close(_wake);
    }
    if (_epoll >= 0) {
        ::close(_epoll);
    }
}

void EpollEventLoopDriver::wait() {
    waitInternal(-1);
}

void EpollEventLoopDriver::wait(const time::TimeDelta maximumWait) {
    if (!maximumWait.isPositive()) {
        waitInternal(0);
        return;
    }
    const auto nanoseconds = maximumWait.toStdNanoseconds();
    const auto roundedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        nanoseconds + std::chrono::milliseconds{1} - std::chrono::nanoseconds{1});
    const auto maximum = static_cast<int64_t>(std::numeric_limits<int>::max());
    waitInternal(static_cast<int>(std::min<int64_t>(roundedMilliseconds.count(), maximum)));
}

void EpollEventLoopDriver::wake() {
    constexpr auto value = uint64_t{1U};
    while (true) {
        const auto result = _nativeCalls->writeWake(_wake, value);
        if (result == static_cast<ssize_t>(sizeof(value))) {
            return;
        }
        const auto error = errno;
        if (result < 0 && error == EINTR) {
            continue;
        }
        if (result < 0 && error == EAGAIN) {
            return; // A full eventfd counter already represents a pending wake.
        }
        auto errorCode = EIO;
        if (result < 0) {
            errorCode = error;
        }
        throw system::PlatformFailure{
            "Failed to signal the epoll wake descriptor."_el,
            system::impl::PosixErrorContext::fromErrorCode(errorCode)};
    }
}

auto EpollEventLoopDriver::registerDescriptor(
    const int descriptor, const bool read, const bool write, NativeEventCallback callback) -> RegistrationPtr {
    if (descriptor < 0) {
        throw ParameterFailure{"The native descriptor must not be negative."_el, "descriptor"_el};
    }
    if (!read && !write) {
        throw ParameterFailure{"At least one native event direction must be registered."_el, "read"_el};
    }
    if (!callback) {
        throw ParameterFailure{"The native event callback must not be empty."_el, "callback"_el};
    }
    const auto lock = std::scoped_lock{_registrationMutex};
    for (const auto &entry : _registrations) {
        const auto &registration = entry.second;
        if (registration.descriptor == descriptor) {
            throw ParameterFailure{"The native descriptor is already registered."_el, "descriptor"_el};
        }
    }
    const auto generation = _nextGeneration++;
    auto flags = uint32_t{};
    if (read) {
        flags |= EPOLLIN | EPOLLRDHUP;
    }
    if (write) {
        flags |= EPOLLOUT;
    }
    auto event = epoll_event{.events = flags, .data = {.u64 = generation}};
    if (_nativeCalls->control(_epoll, EPOLL_CTL_ADD, descriptor, &event) < 0) {
        throw system::PlatformError{
            "Failed to register a native descriptor with epoll."_el, system::impl::PosixErrorContext::fromErrno()};
    }
    _registrations.emplace(generation, Registration{descriptor, std::move(callback)});
    return std::make_unique<EventLoopDriverRegistration>(
        [this, generation]() -> void { unregisterDescriptor(generation); });
}

void EpollEventLoopDriver::unregisterDescriptor(const uint64_t generation) {
    const auto lock = std::scoped_lock{_registrationMutex};
    const auto iterator = _registrations.find(generation);
    if (iterator == _registrations.end()) {
        return;
    }
    while (_nativeCalls->control(_epoll, EPOLL_CTL_DEL, iterator->second.descriptor, nullptr) < 0) {
        const auto error = errno;
        if (error == EINTR) {
            continue;
        }
        if (error == ENOENT) {
            break;
        }
        throw system::PlatformFailure{
            "Failed to remove an epoll descriptor registration."_el,
            system::impl::PosixErrorContext::fromErrorCode(error)};
    }
    _registrations.erase(iterator);
}

void EpollEventLoopDriver::waitInternal(const int timeoutMilliseconds) {
    auto events = std::array<epoll_event, cMaximumEvents>{};
    while (true) {
        const auto result = ::epoll_wait(_epoll, events.data(), cMaximumEvents, timeoutMilliseconds);
        if (result >= 0) {
            for (auto index = 0; index < result; ++index) {
                const auto &event = events[static_cast<std::size_t>(index)];
                if (event.data.u64 == cWakeGeneration) {
                    drainWake();
                    continue;
                }
                auto callback = NativeEventCallback{};
                {
                    const auto lock = std::scoped_lock{_registrationMutex};
                    const auto iterator = _registrations.find(event.data.u64);
                    if (iterator == _registrations.end()) {
                        continue;
                    }
                    callback = iterator->second.callback;
                }
                callback(
                    (event.events & EPOLLIN) != 0,
                    (event.events & EPOLLOUT) != 0,
                    (event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) != 0);
            }
            return;
        }
        if (errno != EINTR) {
            throw system::PlatformError{
                "The epoll event-loop wait failed."_el, system::impl::PosixErrorContext::fromErrno()};
        }
    }
}

void EpollEventLoopDriver::drainWake() {
    auto value = uint64_t{};
    while (true) {
        const auto result = _nativeCalls->readWake(_wake, value);
        if (result == static_cast<ssize_t>(sizeof(value))) {
            continue;
        }
        const auto error = errno;
        if (result < 0 && error == EINTR) {
            continue;
        }
        if (result < 0 && error == EAGAIN) {
            return;
        }
        auto errorCode = EIO;
        if (result < 0) {
            errorCode = error;
        }
        throw system::PlatformError{
            "Failed to drain the epoll wake descriptor."_el, system::impl::PosixErrorContext::fromErrorCode(errorCode)};
    }
}

}
