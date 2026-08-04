// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EpollEventLoopDriver.hpp"

#include "../../err/ParameterError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../text/Literals.hpp"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <limits>

namespace erbsland::event::impl {

using namespace text::literals;

EpollEventLoopDriver::EpollEventLoopDriver() :
    _epoll{::epoll_create1(EPOLL_CLOEXEC)}, _wake{::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)} {
    if (_epoll < 0 || _wake < 0) {
        if (_wake >= 0) {
            ::close(_wake);
        }
        if (_epoll >= 0) {
            ::close(_epoll);
        }
        throw err::RuntimeError{"Failed to create the epoll event-loop driver."};
    }
    auto event = epoll_event{.events = EPOLLIN, .data = {.u64 = cWakeGeneration}};
    if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, _wake, &event) < 0) {
        ::close(_wake);
        ::close(_epoll);
        _wake = -1;
        _epoll = -1;
        throw err::RuntimeError{"Failed to register the epoll wake descriptor."};
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

void EpollEventLoopDriver::wake() noexcept {
    constexpr auto value = uint64_t{1U};
    // anti-pattern: allow static_cast_void -- A failed wake is harmless because the queue remains signalled.
    static_cast<void>(::write(_wake, &value, sizeof(value)));
}

auto EpollEventLoopDriver::registerDescriptor(
    const int descriptor, const bool read, const bool write, NativeEventCallback callback) -> RegistrationPtr {
    if (descriptor < 0) {
        throw err::ParameterError{"The native descriptor must not be negative."_el, "descriptor"_el};
    }
    if (!read && !write) {
        throw err::ParameterError{"At least one native event direction must be registered."_el, "read"_el};
    }
    if (!callback) {
        throw err::ParameterError{"The native event callback must not be empty."_el, "callback"_el};
    }
    for (const auto &entry : _registrations) {
        const auto &registration = entry.second;
        if (registration.descriptor == descriptor) {
            throw err::ParameterError{"The native descriptor is already registered."_el, "descriptor"_el};
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
    if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, descriptor, &event) < 0) {
        throw err::RuntimeError{"Failed to register a native descriptor with epoll."_el};
    }
    _registrations.emplace(generation, Registration{descriptor, std::move(callback)});
    return std::make_unique<EventLoopDriverRegistration>(
        [this, generation]() noexcept -> void { unregisterDescriptor(generation); });
}

void EpollEventLoopDriver::unregisterDescriptor(const uint64_t generation) noexcept {
    const auto iterator = _registrations.find(generation);
    if (iterator == _registrations.end()) {
        return;
    }
    // anti-pattern: allow static_cast_void -- The descriptor is being released and failed deregistration is
    // unrecoverable.
    static_cast<void>(::epoll_ctl(_epoll, EPOLL_CTL_DEL, iterator->second.descriptor, nullptr));
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
                const auto iterator = _registrations.find(event.data.u64);
                if (iterator == _registrations.end()) {
                    continue;
                }
                const auto callback = iterator->second.callback;
                callback(
                    (event.events & EPOLLIN) != 0,
                    (event.events & EPOLLOUT) != 0,
                    (event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) != 0);
            }
            return;
        }
        if (errno != EINTR) {
            throw err::RuntimeError{"The epoll event-loop wait failed."};
        }
    }
}

void EpollEventLoopDriver::drainWake() noexcept {
    auto value = uint64_t{};
    while (::read(_wake, &value, sizeof(value)) == sizeof(value)) {}
}

}
