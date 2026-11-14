// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef __APPLE__
#error "KqueueEventLoopDriver.hpp is only available on macOS."
#endif

#include "EventLoopDriverRegistration.hpp"
#include "KqueueEventLoopNativeCalls.hpp"

#include "../EventLoopDriver.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace erbsland::event::impl {

/// The macOS kqueue event-loop driver.
/// @tested{EventLoopDriverTest KqueueEventLoopDriverTest}
class KqueueEventLoopDriver final : public EventLoopDriver {
public:
    /// Create a kqueue-based event-loop driver.
    /// @throws system::PlatformError If native event-loop initialization fails.
    KqueueEventLoopDriver();
    /// Create a driver using the given non-null native operations.
    /// @param nativeCalls The native operations retained by this driver.
    /// @throws system::PlatformError If native event-loop initialization fails.
    explicit KqueueEventLoopDriver(std::shared_ptr<KqueueEventLoopNativeCalls> nativeCalls);

    /// Release the native kqueue descriptor and its registrations.
    ~KqueueEventLoopDriver() override;

    // defaults/deletions
    KqueueEventLoopDriver(const KqueueEventLoopDriver &) = delete;
    auto operator=(const KqueueEventLoopDriver &) -> KqueueEventLoopDriver & = delete;

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
    static constexpr auto cWakeIdentifier = uintptr_t{1U}; ///< Identifier reserved for wake events.
    static constexpr auto cMaximumEvents = 32;             ///< Maximum native events read in one wait.

    /// Wait for native events until an optional absolute timeout.
    void waitInternal(const timespec *timeout);
    /// Remove one readiness filter while holding the registration lock.
    void removeFilter(int descriptor, short filter);
    /// Remove a descriptor registration by generation.
    void unregisterDescriptor(uint64_t generation);

    /// A descriptor readiness registration.
    struct Registration {
        int descriptor;
        bool read;
        bool write;
        NativeEventCallback callback;
    };

private:
    std::shared_ptr<KqueueEventLoopNativeCalls> _nativeCalls;  ///< Retained native call adapter.
    int _queue{-1};                                            ///< Native kqueue descriptor.
    mutable std::mutex _registrationMutex;                     ///< Protects registration changes and callback lookup.
    uint64_t _nextGeneration{2U};                              ///< Next unique registration generation.
    std::unordered_map<uint64_t, Registration> _registrations; ///< Active native registrations.
};

}
