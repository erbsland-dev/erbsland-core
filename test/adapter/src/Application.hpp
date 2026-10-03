// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Options.hpp>

using namespace el::text::literals;

/// Test adapter for the official ELCL conformance suite.
/// @notest{Executed by the external ELCL conformance runner.}
class Application final : public el::Application {
private:
    static constexpr auto cConfigurationFileOption = "configuration-file"_el;

public:
    using el::Application::Application;

protected: // implement el::Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    [[nodiscard]] auto main() -> el::ExitCode override;
};
