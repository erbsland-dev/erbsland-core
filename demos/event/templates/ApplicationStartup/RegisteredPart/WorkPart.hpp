// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/ApplicationPart.hpp>
#include <erbsland/ApplicationPartIdentifier.hpp>
#include <erbsland/Literals.hpp>

#include <memory>

namespace demo {

using namespace el::text::literals;

/// Let the application framework own a component and its dedicated event loop.
class WorkPart final : public el::ApplicationPart {
public: // registration
    /// Identify this component in the application's part registry.
    [[nodiscard]] static auto partIdentifier() -> el::ApplicationPartIdentifierPtr {
        static const auto identifier = el::ApplicationPartIdentifier::create("application.work"_el);
        return identifier;
    }
    /// Create the component retained by its manager.
    [[nodiscard]] static auto create() -> std::shared_ptr<WorkPart> { return std::make_shared<WorkPart>(); }

protected: // implement ApplicationPart
    void onInitialize() override {
        // TODO: Create and retain event sources on this part's loop and connect their handlers.
        // Connect the final successful completion of your work to completeWork().
    }
    void onRunning() override {
        // TODO: Start your business operations after this part enters Running.
        // Report asynchronous service readiness separately if other parts require it.
    }
    void cleanup() noexcept override {
        // TODO: Release part-owned resources after its loop exits.
        // Initialization may have failed partway.
    }

private:
    /// Finish the whole application after this component's required work completes.
    void completeWork() { el::Application::instance().quit(); }
};

}
