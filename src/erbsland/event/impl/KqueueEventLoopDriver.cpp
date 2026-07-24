// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KqueueEventLoopDriver.hpp"

#include "../../err/ParameterError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../text/Literals.hpp"

#include <sys/event.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdint>

namespace erbsland::event::impl {

using namespace text::literals;

namespace {
constexpr auto cWakeIdentifier = uintptr_t{1U};
constexpr auto cMaximumEvents = 32;
}

KqueueEventLoopDriver::KqueueEventLoopDriver() : _queue{::kqueue()} {
    if (_queue < 0) {
        throw err::RuntimeError{"Failed to create the kqueue event-loop driver."};
    }
    struct kevent change{};
    EV_SET(&change, cWakeIdentifier, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    if (::kevent(_queue, &change, 1, nullptr, 0, nullptr) < 0) {
        ::close(_queue);
        _queue = -1;
        throw err::RuntimeError{"Failed to register the kqueue wake event."};
    }
}

KqueueEventLoopDriver::~KqueueEventLoopDriver() {
    if (_queue >= 0) {
        ::close(_queue);
    }
}

void KqueueEventLoopDriver::wait() {
    waitInternal(nullptr);
}

void KqueueEventLoopDriver::wait(const time::TimeDelta maximumWait) {
    const auto duration = maximumWait.isPositive() ? maximumWait.toStdNanoseconds() : std::chrono::nanoseconds::zero();
    auto timeout = timespec{
        .tv_sec = static_cast<time_t>(std::chrono::duration_cast<std::chrono::seconds>(duration).count()),
        .tv_nsec = static_cast<long>((duration % std::chrono::seconds{1}).count()),
    };
    waitInternal(&timeout);
}

void KqueueEventLoopDriver::wake() noexcept {
    struct kevent change{};
    EV_SET(&change, cWakeIdentifier, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
    static_cast<void>(::kevent(_queue, &change, 1, nullptr, 0, nullptr));
}

auto KqueueEventLoopDriver::registerDescriptor(
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
    for (const auto &[generation, registration] : _registrations) {
        static_cast<void>(generation);
        if (registration.descriptor == descriptor) {
            throw err::ParameterError{"The native descriptor is already registered."_el, "descriptor"_el};
        }
    }
    const auto generation = _nextGeneration++;
    auto changes = std::array<struct kevent, 2>{};
    auto changeCount = 0;
    if (read) {
        EV_SET(
            &changes[static_cast<std::size_t>(changeCount++)],
            static_cast<uintptr_t>(descriptor),
            EVFILT_READ,
            EV_ADD | EV_ENABLE,
            0,
            0,
            reinterpret_cast<void *>(static_cast<uintptr_t>(generation)));
    }
    if (write) {
        EV_SET(
            &changes[static_cast<std::size_t>(changeCount++)],
            static_cast<uintptr_t>(descriptor),
            EVFILT_WRITE,
            EV_ADD | EV_ENABLE,
            0,
            0,
            reinterpret_cast<void *>(static_cast<uintptr_t>(generation)));
    }
    if (::kevent(_queue, changes.data(), changeCount, nullptr, 0, nullptr) < 0) {
        throw err::RuntimeError{"Failed to register a native descriptor with kqueue."_el};
    }
    _registrations.emplace(generation, Registration{descriptor, read, write, std::move(callback)});
    return std::make_unique<EventLoopDriverRegistration>(
        [this, generation]() noexcept -> void { unregisterDescriptor(generation); });
}

void KqueueEventLoopDriver::unregisterDescriptor(const uint64_t generation) noexcept {
    const auto iterator = _registrations.find(generation);
    if (iterator == _registrations.end()) {
        return;
    }
    auto changes = std::array<struct kevent, 2>{};
    auto changeCount = 0;
    if (iterator->second.read) {
        EV_SET(
            &changes[static_cast<std::size_t>(changeCount++)],
            static_cast<uintptr_t>(iterator->second.descriptor),
            EVFILT_READ,
            EV_DELETE,
            0,
            0,
            nullptr);
    }
    if (iterator->second.write) {
        EV_SET(
            &changes[static_cast<std::size_t>(changeCount++)],
            static_cast<uintptr_t>(iterator->second.descriptor),
            EVFILT_WRITE,
            EV_DELETE,
            0,
            0,
            nullptr);
    }
    static_cast<void>(::kevent(_queue, changes.data(), changeCount, nullptr, 0, nullptr));
    _registrations.erase(iterator);
}

void KqueueEventLoopDriver::waitInternal(const timespec *timeout) {
    auto events = std::array<struct kevent, cMaximumEvents>{};
    while (true) {
        const auto result = ::kevent(_queue, nullptr, 0, events.data(), cMaximumEvents, timeout);
        if (result >= 0) {
            for (auto index = 0; index < result; ++index) {
                const auto &event = events[static_cast<std::size_t>(index)];
                if (event.filter == EVFILT_USER && event.ident == cWakeIdentifier) {
                    continue;
                }
                const auto generation = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(event.udata));
                const auto iterator = _registrations.find(generation);
                if (iterator == _registrations.end()) {
                    continue;
                }
                const auto callback = iterator->second.callback;
                callback(event.filter == EVFILT_READ, event.filter == EVFILT_WRITE, (event.flags & EV_ERROR) != 0);
            }
            return;
        }
        if (errno != EINTR) {
            throw err::RuntimeError{"The kqueue event-loop wait failed."};
        }
    }
}

}
