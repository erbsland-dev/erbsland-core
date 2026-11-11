// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/UnmanagedEventThread.hpp>

#include <exception>

namespace demo {

/// Own a standalone event thread through termination, joining, and error inspection.
class WorkApplication final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    auto main() -> el::ExitCode override {
        const auto options = el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit);
        _worker = el::UnmanagedEventThread::create(options);
        _worker->start();
        _worker->loop()->invoke([this]() -> void { beginWork(); });
        // completeWork() or the Quit error policy must end dispatch before join returns.
        _worker->join();
        if (const auto error = _worker->workerEventLoop().takeError()) {
            std::rethrow_exception(error);
        }
        // TODO: Read worker-owned results here, after joining and checking failure.
        return el::ExitCode::success();
    }
    void cleanup() noexcept override {
        // The application does not manage this thread. Its final owner stops and joins it.
        _worker.reset();
    }

private:
    /// Set up operations inside the worker's current loop context.
    void beginWork() {
        // TODO: Create and retain your business operations here.
        // Connect successful completion to completeWork(), without a main-loop reply.
    }
    /// Close the worker's dispatch path only after its required operations finish.
    void completeWork() { _worker->quit(); }

private:
    el::UnmanagedEventThreadPtr _worker; ///< Main-thread owner retained through join and cleanup.
};

}
