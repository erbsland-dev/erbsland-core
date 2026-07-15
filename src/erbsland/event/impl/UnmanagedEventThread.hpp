// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventThreadBase.hpp"

#include "../UnmanagedEventThread.hpp"

namespace erbsland::event::impl {

class UnmanagedEventThread final : public event::UnmanagedEventThread, private EventThreadBase {
public:
    UnmanagedEventThread() = default;
    ~UnmanagedEventThread() noexcept override;

    // defaults
    UnmanagedEventThread(const UnmanagedEventThread &) = delete;
    auto operator=(const UnmanagedEventThread &) -> UnmanagedEventThread & = delete;
    UnmanagedEventThread(UnmanagedEventThread &&) = delete;
    auto operator=(UnmanagedEventThread &&) -> UnmanagedEventThread & = delete;

public: // implement event::EventThread
    void start() override;
    void quit() noexcept override;
    void join() override;
    [[nodiscard]] auto isStarted() const noexcept -> bool override;
    [[nodiscard]] auto isRunning() const noexcept -> bool override;
    [[nodiscard]] auto eventLoop() -> event::EventLoop & override;
    [[nodiscard]] auto events() -> event::EventsPtr override;

private: // implement EventThreadBase
    void runEventLoop() override;
};

}
