// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KqueueEventLoopDriver.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../system/impl/PosixErrorContext.hpp"
#include "../../system/PlatformError.hpp"
#include "../../system/PlatformFailure.hpp"
#include "../../text/Literals.hpp"

#include <sys/event.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>

namespace erbsland::event::impl {

using namespace err;
using namespace text::literals;

KqueueEventLoopDriver::KqueueEventLoopDriver() : KqueueEventLoopDriver{std::make_shared<KqueueEventLoopNativeCalls>()} {
}

KqueueEventLoopDriver::KqueueEventLoopDriver(std::shared_ptr<KqueueEventLoopNativeCalls> nativeCalls) :
    _nativeCalls{std::move(nativeCalls)} {
    if (_nativeCalls == nullptr) {
        throw err::ParameterFailure{"The native call adapter must not be null."_el, "nativeCalls"_el};
    }
    _queue = ::kqueue();
    if (_queue < 0) {
        throw system::PlatformError{
            "Failed to create the kqueue event-loop driver."_el, system::impl::PosixErrorContext::fromErrno()};
    }
    struct kevent change{};
    EV_SET(&change, cWakeIdentifier, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    if (_nativeCalls->change(_queue, &change, 1) < 0) {
        const auto context = system::impl::PosixErrorContext::fromErrno();
        ::close(_queue);
        _queue = -1;
        throw system::PlatformError{"Failed to register the kqueue wake event."_el, context};
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

void KqueueEventLoopDriver::wake() {
    struct kevent change{};
    EV_SET(&change, cWakeIdentifier, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
    while (_nativeCalls->change(_queue, &change, 1) < 0) {
        const auto error = errno;
        if (error == EINTR) {
            continue;
        }
        throw system::PlatformFailure{
            "Failed to trigger the kqueue wake event."_el, system::impl::PosixErrorContext::fromErrorCode(error)};
    }
}

auto KqueueEventLoopDriver::registerDescriptor(
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
        if (entry.second.descriptor == descriptor) {
            throw ParameterFailure{"The native descriptor is already registered."_el, "descriptor"_el};
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
    if (_nativeCalls->change(_queue, changes.data(), changeCount) < 0) {
        throw system::PlatformError{
            "Failed to register a native descriptor with kqueue."_el, system::impl::PosixErrorContext::fromErrno()};
    }
    _registrations.emplace(generation, Registration{descriptor, read, write, std::move(callback)});
    return std::make_unique<EventLoopDriverRegistration>(
        [this, generation]() -> void { unregisterDescriptor(generation); });
}

void KqueueEventLoopDriver::removeFilter(const int descriptor, const short filter) {
    struct kevent change{};
    EV_SET(&change, static_cast<uintptr_t>(descriptor), filter, EV_DELETE, 0, 0, nullptr);
    while (_nativeCalls->change(_queue, &change, 1) < 0) {
        const auto error = errno;
        if (error == EINTR) {
            continue;
        }
        if (error == ENOENT) {
            return;
        }
        throw system::PlatformFailure{
            "Failed to remove a kqueue descriptor filter."_el, system::impl::PosixErrorContext::fromErrorCode(error)};
    }
}

void KqueueEventLoopDriver::unregisterDescriptor(const uint64_t generation) {
    const auto lock = std::scoped_lock{_registrationMutex};
    const auto iterator = _registrations.find(generation);
    if (iterator == _registrations.end()) {
        return;
    }
    if (iterator->second.read) {
        removeFilter(iterator->second.descriptor, EVFILT_READ);
    }
    if (iterator->second.write) {
        removeFilter(iterator->second.descriptor, EVFILT_WRITE);
    }
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
                auto callback = NativeEventCallback{};
                {
                    const auto lock = std::scoped_lock{_registrationMutex};
                    const auto iterator = _registrations.find(generation);
                    if (iterator == _registrations.end()) {
                        continue;
                    }
                    callback = iterator->second.callback;
                }
                callback(event.filter == EVFILT_READ, event.filter == EVFILT_WRITE, (event.flags & EV_ERROR) != 0);
            }
            return;
        }
        if (errno != EINTR) {
            throw system::PlatformError{
                "The kqueue event-loop wait failed."_el, system::impl::PosixErrorContext::fromErrno()};
        }
    }
}

}
