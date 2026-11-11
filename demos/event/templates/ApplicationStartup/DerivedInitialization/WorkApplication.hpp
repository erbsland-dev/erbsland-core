// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/EventTarget.hpp>

namespace demo {

/// Own event-driven work while retaining Application's default main loop.
class WorkApplication final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override {
        // Prepare application-owned state that does not require loop context.
        loop()->invoke([this]() -> void { beginWork(); });
    }
    void cleanup() noexcept override {
        // TODO: Release application resources; initialization may have failed partway.
        // Main-loop callbacks cannot run here.
    }

private:
    /// Create event sources in the context of the running main loop.
    void beginWork() {
        // TODO: Start and retain your business operations here.
        // Connect their final successful completion to completeWork().
    }
    /// Request shutdown after all required work has finished on this loop.
    void completeWork() { quit(); }
};

}
