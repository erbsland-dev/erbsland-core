// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/ManagedEventThread.hpp>

namespace demo {

/// Run event work on a retained worker while procedural main waits for completion.
class WorkApplication final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    auto main() -> el::ExitCode override {
        auto options = el::EventLoopOptions{};
        options.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication);
        _worker = createEventThread(options);
        _worker->start();
        _worker->loop()->invoke([this]() -> void { beginWork(); });
        // completeWork() must quit the worker before this join can return.
        _worker->join();
        // Read worker-owned results only after joining. No main loop was needed.
        return el::ExitCode::success();
    }
    void cleanup() noexcept override {
        // The framework also stops and joins managed workers on failure.
        _worker.reset();
        // TODO: Release remaining resources, tolerating partial initialization.
    }

private:
    /// Start operations inside the worker loop, never on the waiting main thread.
    void beginWork() {
        // TODO: Create and retain your worker's business operations here.
        // Connect their final successful completion to completeWork().
        // Do not route required completion through the application's idle main loop.
    }
    /// Finish on the worker; the owner on the main thread performs the join.
    void completeWork() { _worker->quit(); }

private:
    el::ManagedEventThreadPtr _worker; ///< Owned by the application through shutdown and joining.
};

}
