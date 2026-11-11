// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/DiagnosticHelper.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/TextDocument.hpp>

namespace demo {

/// An application starting point with explicit main-loop failure reporting.
class CustomErrorApplication : public el::Application {
public:
    using Application::Application;

protected: // customization
    /// Connect application operations and their completion callbacks before entering dispatch.
    /// Completion must call quit(); the unmodified template intentionally has no business work.
    virtual void startWork() = 0;
    /// Send a diagnostic, including its cause chain, to the application's reporting destination.
    /// @param document The diagnostic for one captured operational failure.
    virtual void reportFailure(const el::TextDocument &document) = 0;

protected: // implement Application
    auto main() -> el::ExitCode override {
        mainEventLoop().setErrorHandler(
            [](std::exception_ptr) -> el::EventLoopErrorAction { return el::EventLoopErrorAction::Stop; });
        loop()->invoke([this]() -> void { startWork(); });
        // Direct dispatch preserves our handler; runMainEventLoop() installs the default handler.
        mainEventLoop().run();
        auto result = el::ExitCode::success();
        while (const auto error = mainEventLoop().takeError()) {
            reportFailure(el::DiagnosticHelper::documentFromError(error));
            result = el::ExitCode::failure();
        }
        mainEventLoop().setErrorHandler({});
        return result;
    }
};

}
