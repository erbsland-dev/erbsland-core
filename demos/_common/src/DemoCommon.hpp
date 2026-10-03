// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringMap.hpp>

namespace demo {

using namespace el::text::literals;

/// A simple framework for demos to run multiple functions in a sequence or select one function.
class DemoApplication : public el::Application {
public:
    /// A demo function.
    using DemoFn = std::function<void()>;

    /// The mode of the demo application.
    enum class Mode : uint8_t {
        /// Run the registered function like the main function of an application.
        Procedural,
        /// The registered function initializes the application, the main loop runs after the function returns.
        EventLoop,
    };

    /// The demo entry.
    struct Demo {
        el::String name;
        Mode mode;
        DemoFn demoFn;
    };

public:
    /// Create a new demo application.
    DemoApplication(int argc, char **argv);

    // defaults
    ~DemoApplication() override = default;

public:
    /// Register a demo function.
    void registerDemo(const el::String &name, const DemoFn &demoFn, Mode mode = Mode::Procedural);

protected: // override Application
    [[nodiscard]] auto createAndInitializeTerminal(el::block::Size size, el::cterm::TerminalFlags flags)
        -> el::cterm::TerminalPtr override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    [[nodiscard]] auto main() -> el::ExitCode override;

private:
    void runDemoEventLoop();

private:
    el::StringMap<Demo> _demoFunctions; ///< A map with demo functions
};

}
