// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>

/// Test adapter for the official ELCL conformance suite.
/// @notest{Executed by the external ELCL conformance runner.}
class Application final : public el::Application {
public:
    using el::Application::Application;

protected: // implement el::Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    [[nodiscard]] auto main() -> el::ExitCode override;
};
