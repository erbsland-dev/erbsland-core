// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../EventLoop_fwd.hpp"
#include "../Events_fwd.hpp"

#include <mutex>
#include <thread>

namespace erbsland::event::impl {

class EventThreadBase {
public:
    EventThreadBase();
    virtual ~EventThreadBase() noexcept;

    // defaults
    EventThreadBase(const EventThreadBase &) = delete;
    auto operator=(const EventThreadBase &) -> EventThreadBase & = delete;
    EventThreadBase(EventThreadBase &&) = delete;
    auto operator=(EventThreadBase &&) -> EventThreadBase & = delete;

public:
    void start();
    void quit() noexcept;
    void join();
    [[nodiscard]] auto isStarted() const noexcept -> bool;
    [[nodiscard]] auto isRunning() const noexcept -> bool;
    [[nodiscard]] auto eventLoop() -> event::EventLoop &;
    [[nodiscard]] auto events() -> event::EventsPtr;

protected:
    void shutdown() noexcept;
    [[nodiscard]] auto eventLoopPtr() const noexcept -> const event::EventLoopPtr &;

private:
    virtual void runEventLoop() = 0;

private:
    event::EventLoopPtr _eventLoop; ///< The event loop running in the thread.
    mutable std::mutex _mutex;      ///< Protects thread state.
    std::thread _thread;            ///< The native worker thread.
    bool _started{false};           ///< True after `start()` was called.
};

}
