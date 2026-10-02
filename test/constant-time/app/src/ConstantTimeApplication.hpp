// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Registry.hpp"
#include "Runner.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ExitCode.hpp>

#include <atomic>

namespace app::constant_time {

/// On-demand constant-time leakage assessment application.
/// @notest{Command-line integration validated by explicit smoke runs.}
class ConstantTimeApplication final : public el::Application {
public:
    using el::Application::Application;

protected:
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void parseCommandLine() override;
    [[nodiscard]] auto main() -> el::ExitCode override;

private:
    /// Emit and flush one record synchronously between measurements.
    void printRecord(const el::String &line) const;
    /// Report the build, platform and seed before measured execution.
    void printBuild(uint64_t seed) const;
    /// Execute validated command-line options with cooperative signal handling.
    [[nodiscard]] auto execute() -> el::ExitCode;
    std::atomic<bool> _interrupted{}; ///< Cooperative interrupt flag.
};
}
