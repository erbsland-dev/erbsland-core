// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef __linux__
#error "EpollEventLoopDriver.hpp is only available on Linux."
#endif

#include "EventLoopDriverRegistration.hpp"

#include "../EventLoopDriver.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

namespace erbsland::event::impl {

/// The Linux epoll event-loop driver.
/// @tested{EventLoopDriverTest}
class EpollEventLoopDriver final : public EventLoopDriver {
public:
    EpollEventLoopDriver();
    ~EpollEventLoopDriver() override;

    // defaults/deletions
    EpollEventLoopDriver(const EpollEventLoopDriver &) = delete;
    auto operator=(const EpollEventLoopDriver &) -> EpollEventLoopDriver & = delete;

public: // implement EventLoopDriver
    void wait() override;
    void wait(time::TimeDelta maximumWait) override;
    void wake() noexcept override;

public: // native source interface
    using NativeEventCallback = std::function<void(bool readable, bool writable, bool error)>;
    using RegistrationPtr = std::unique_ptr<EventLoopDriverRegistration>;

    /// Register descriptor readiness with this driver.
    [[nodiscard]] auto registerDescriptor(int descriptor, bool read, bool write, NativeEventCallback callback)
        -> RegistrationPtr;

private:
    void waitInternal(int timeoutMilliseconds);
    void drainWake() noexcept;
    void unregisterDescriptor(uint64_t generation) noexcept;

    struct Registration {
        int descriptor;
        NativeEventCallback callback;
    };

private:
    int _epoll{-1};                                            ///< Native epoll descriptor.
    int _wake{-1};                                             ///< Native eventfd wake descriptor.
    uint64_t _nextGeneration{2U};                              ///< Next unique registration generation.
    std::unordered_map<uint64_t, Registration> _registrations; ///< Active native registrations.
};

}
