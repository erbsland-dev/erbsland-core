// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PackageConfig.hpp"
#include "PackageTarget.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

#include <vector>

namespace erbsland::package {

using namespace text::literals;

/// Creates one release ZIP from CMake-built targets.
/// @notest{Covered by package integration tests.}
class PackageApplication final : public core::Application {
    static constexpr auto cProjectNameOption = "project-name"_el;
    static constexpr auto cProjectVersionOption = "project-version"_el;

public:
    /// Create the package application from a narrow command line.
    PackageApplication(int argc, char *argv[]) : Application{argc, argv} {}
    /// Create the package application from a Windows wide command line.
    PackageApplication(int argc, wchar_t *argv[]) : Application{argc, argv} {}

protected: // implement core::Application
    /// Set the command-line application metadata.
    void initialize() override;
    /// Register the package invocation options.
    void registerCommandLineOptions(const options::OptionsPtr &options) override;
    /// Stage and publish the requested ZIP.
    [[nodiscard]] auto main() -> unit::ExitCode override;

private:
    /// Escape one dynamic Info.plist value.
    [[nodiscard]] static auto xmlEscape(const text::String &value) -> text::String;
    /// Validate and resolve the CMake executable target paths.
    [[nodiscard]] auto readTargets() const -> std::vector<PackageTarget>;
};

}
