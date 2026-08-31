// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventThreadBase.hpp"

#include "../ManagedEventThread.hpp"

namespace erbsland::event::impl {

/// Managed event-thread implementation backed by an event-thread base.
class ManagedEventThread final : public event::ManagedEventThread, private EventThreadBase {
public:
    /// Create an inactive managed event thread.
    ManagedEventThread() = default;
    /// Stop and release the managed event thread.
    ~ManagedEventThread() noexcept override;

    // defaults/deletions
    ManagedEventThread(const ManagedEventThread &) = delete;
    auto operator=(const ManagedEventThread &) -> ManagedEventThread & = delete;
    ManagedEventThread(ManagedEventThread &&) = delete;
    auto operator=(ManagedEventThread &&) -> ManagedEventThread & = delete;

public: // implements event::EventThread
    void start() override;
    void quit() noexcept override;
    void join() override;
    [[nodiscard]] auto isStarted() const noexcept -> bool override;
    [[nodiscard]] auto isRunning() const noexcept -> bool override;
    [[nodiscard]] auto eventLoop() noexcept -> event::EventLoop & override;
    [[nodiscard]] auto events() noexcept -> event::EventsPtr override;

private: // implements EventThreadBase
    void runEventLoop() override;
};

}
