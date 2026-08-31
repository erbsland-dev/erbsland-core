// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

namespace demo {

/// Load and apply an ELCL logging configuration during application startup.
///
/// The configuration file may contain the log settings at its root or below a selected branch. Configuration parser
/// and validation errors are intentionally allowed to reach `Application::run()`, which renders a consistent
/// diagnostic.
/// @notest{Compiled and exercised as part of the configured logging demo.}
class ConfiguredLoggingApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void parseCommandLine() override;
    [[nodiscard]] auto main() -> el::ExitCode override;
};

}
