// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PlatformErrorContext.hpp>
#include <erbsland/Subprocess.hpp>
#include <erbsland/System.hpp>

namespace demo {

using namespace el::text::literals;

/// Prepare a subprocess source and its handlers inside the owning main loop.
class ProcessApplication final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->addOption("executable"_el).setRequired().setHelp("Absolute path of the child executable."_el);
    }
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        _executable = el::Path::fromNativeOrThrow(values->getText("executable"_el));
        loop()->invoke([this]() -> void {
            // Source preparation and launch both happen inside the owner loop.
            prepareSource();
            _process->start();
        });
    }
    void cleanup() noexcept override { _process.reset(); }

private:
    /// Create the inactive source and connect reactions before starting the child.
    void prepareSource() {
        _process = loop()->get<el::System>().createSubprocess(_executable);
        _process->events()
            .onExit([this](const el::ExitCode code) -> void { _exitCode = code; })
            .onError([](const el::PlatformErrorContext &error) -> void {
                throw el::ApplicationError{"Child process failed."_el, error.toString()};
            })
            .onFinal([this]() -> void { quit(_exitCode); });
        // TODO: Connect output handlers and other business reactions before launch.
    }

private:
    el::Path _executable;                            ///< Configuration retained until source preparation.
    el::SubprocessPtr _process;                      ///< Retains the source and its source-owned handlers.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Completion status, accessed only on the main loop.
};

}
