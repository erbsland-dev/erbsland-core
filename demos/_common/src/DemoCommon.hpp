// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/stream/StandardStreams.hpp>

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
    DemoApplication(const int argc, char **argv) : Application(argc, argv) {}

    // defaults
    ~DemoApplication() override = default;

public:
    void registerDemo(const el::String &name, const DemoFn &demoFn, Mode mode = Mode::Procedural);

protected: // override Application
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    [[nodiscard]] auto main() -> el::ExitCode override;

protected:                                                                   // override Application
    void initializeRandom(el::RandomPtr &randomPtr) noexcept override;       // only possible in developer builds
    void initializeSecureRandom(el::RandomPtr &randomPtr) noexcept override; // only possible in developer builds

private:
    void runDemoEventLoop();

private:
    el::StringMap<Demo> _demoFunctions; ///< A map with demo functions
};

}
