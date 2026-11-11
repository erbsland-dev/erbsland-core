// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventThreadBase.hpp"

#include "../ManagedEventThread.hpp"

#include <utility>

namespace erbsland::event::impl {

/// Managed event-thread implementation backed by an event-thread base.
class ManagedEventThread final : public event::ManagedEventThread, private EventThreadBase {
public:
    /// Create an inactive managed event thread.
    /// @param options Copied capacity and error policies. Policies must be valid and supported by the failure handler.
    /// @param failureHandler Weak managed-application failure delivery.
    explicit ManagedEventThread(const EventLoopOptions &options = {}, EventLoop::FailureHandler failureHandler = {}) :
        EventThreadBase{options, std::move(failureHandler)} {}
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
