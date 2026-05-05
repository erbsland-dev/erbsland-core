// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/StandardStreams.hpp>

using namespace el::text::literals;

/// A simple framework for demos to run multiple functions in a sequence or select one function.
class DemoApplication : public el::Application {
public:
    using DemoFn = std::function<void()>;

public:
    DemoApplication(const int argc, char **argv) : Application(argc, argv) {}
    ~DemoApplication() override = default;

public:
    void registerDemo(const el::StringView &name, const DemoFn &demoFn);

public: // override Application
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    [[nodiscard]] auto main() -> el::ExitCode override;

protected:                                                                   // override Application
    void initializeRandom(el::RandomPtr &randomPtr) noexcept override;       // only possible in developer builds
    void initializeSecureRandom(el::RandomPtr &randomPtr) noexcept override; // only possible in developer builds

private:
    el::StringMap<DemoFn> _demoFunctions; ///< A map with demo functions
};
