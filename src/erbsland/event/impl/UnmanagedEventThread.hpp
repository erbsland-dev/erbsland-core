// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventThreadBase.hpp"

#include "../UnmanagedEventThread.hpp"

namespace erbsland::event::impl {

/// Unmanaged event-thread implementation backed by an event-thread base.
class UnmanagedEventThread final : public event::UnmanagedEventThread, private EventThreadBase {
public:
    /// Create an inactive unmanaged event thread.
    /// @param options Copied capacity and error policies. Must be valid and must not require managed-application
    /// delivery.
    explicit UnmanagedEventThread(const EventLoopOptions &options = {}) : EventThreadBase{options} {}
    /// Stop and release the unmanaged event thread.
    ~UnmanagedEventThread() noexcept override;

    // defaults/deletions
    UnmanagedEventThread(const UnmanagedEventThread &) = delete;
    auto operator=(const UnmanagedEventThread &) -> UnmanagedEventThread & = delete;
    UnmanagedEventThread(UnmanagedEventThread &&) = delete;
    auto operator=(UnmanagedEventThread &&) -> UnmanagedEventThread & = delete;

public: // implements event::EventThread
    void start() override;
    void quit() noexcept override;
    [[nodiscard]] auto stopToken() const noexcept -> util::StopToken override;
    void join() override;
    [[nodiscard]] auto isStarted() const noexcept -> bool override;
    [[nodiscard]] auto isRunning() const noexcept -> bool override;
    [[nodiscard]] auto workerEventLoop() noexcept -> event::EventLoop & override;
    [[nodiscard]] auto loop() noexcept -> event::EventTargetPtr override;

private: // implements EventThreadBase
    void runWorkerEventLoop() override;
};

}
