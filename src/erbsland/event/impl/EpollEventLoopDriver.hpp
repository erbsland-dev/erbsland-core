// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef __linux__
#error "EpollEventLoopDriver.hpp is only available on Linux."
#endif

#include "EpollEventLoopNativeCalls.hpp"
#include "EventLoopDriverRegistration.hpp"

#include "../EventLoopDriver.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace erbsland::event::impl {

/// The Linux epoll event-loop driver.
/// @tested{EventLoopDriverTest EpollEventLoopDriverTest}
class EpollEventLoopDriver final : public EventLoopDriver {
public:
    /// Create an epoll-based event-loop driver.
    /// @throws system::PlatformError If native event-loop initialization fails.
    EpollEventLoopDriver();
    /// Create a driver using the given non-null native operations.
    /// @param nativeCalls The native operations retained by this driver.
    /// @throws system::PlatformError If native event-loop initialization fails.
    explicit EpollEventLoopDriver(std::shared_ptr<EpollEventLoopNativeCalls> nativeCalls);
    ~EpollEventLoopDriver() override;

    // defaults/deletions
    EpollEventLoopDriver(const EpollEventLoopDriver &) = delete;
    auto operator=(const EpollEventLoopDriver &) -> EpollEventLoopDriver & = delete;

public: // implement EventLoopDriver
    void wait() override;
    void wait(time::TimeDelta maximumWait) override;
    void wake() override;

public: // native source interface
    using NativeEventCallback = std::function<void(bool readable, bool writable, bool error)>;
    using RegistrationPtr = std::unique_ptr<EventLoopDriverRegistration>;

    /// Register descriptor readiness with this driver.
    /// The descriptor must remain open until the registration is reset. Reset may run on another thread.
    /// @param descriptor The native descriptor.
    /// @param read Whether to monitor reads.
    /// @param write Whether to monitor writes.
    /// @param callback The readiness callback.
    /// @return The owning registration token.
    /// @throws system::PlatformError If native event registration fails.
    [[nodiscard]] auto registerDescriptor(int descriptor, bool read, bool write, NativeEventCallback callback)
        -> RegistrationPtr;

private:
    static constexpr auto cWakeGeneration = uint64_t{1U}; ///< Generation reserved for wake events.
    static constexpr auto cMaximumEvents = 32;            ///< Maximum native events read in one wait.

    /// Wait for native events with a millisecond timeout.
    void waitInternal(int timeoutMilliseconds);
    /// Drain pending wake-event notifications.
    void drainWake();
    /// Remove a descriptor registration by generation.
    void unregisterDescriptor(uint64_t generation);

    /// Native descriptor callback registration.
    struct Registration {
        int descriptor;
        NativeEventCallback callback;
    };

private:
    std::shared_ptr<EpollEventLoopNativeCalls> _nativeCalls;   ///< Retained native call adapter.
    int _epoll{-1};                                            ///< Native epoll descriptor.
    int _wake{-1};                                             ///< Native eventfd wake descriptor.
    mutable std::mutex _registrationMutex;                     ///< Protects registration changes and callback lookup.
    uint64_t _nextGeneration{2U};                              ///< Next unique registration generation.
    std::unordered_map<uint64_t, Registration> _registrations; ///< Active native registrations.
};

}
