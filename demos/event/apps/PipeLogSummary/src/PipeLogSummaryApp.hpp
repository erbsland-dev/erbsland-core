// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/Path.hpp>

#include <optional>

namespace demo {

/// Count log lines by severity while a pipeline is running.
class PipeLogSummaryApp final : public el::Application {
    static constexpr auto cFile = el::StringLiteral{"file"};

public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override;
    auto main() -> el::ExitCode override;

private:
    std::optional<el::Path> _file; ///< Optional file in place of piped standard input.
};

}
