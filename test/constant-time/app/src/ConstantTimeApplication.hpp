// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Registry.hpp"
#include "Runner.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/Definitions.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/String.hpp>

#include <atomic>

namespace app::constant_time {

using namespace el::text::literals;

/// On-demand constant-time leakage assessment application.
/// @notest{Command-line integration validated by explicit smoke runs.}
class ConstantTimeApplication final : public el::Application {
    constexpr static auto cAllOption = "all"_el;
    constexpr static auto cMaxSamplesOption = "max-samples"_el;
    constexpr static auto cSeedOption = "seed"_el;
    constexpr static auto cTestOption = "test"_el;

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
