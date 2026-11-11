// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

namespace demo {

/// Check ELCL files as an independent batch and report their failures together.
class ConfigCheckApp final : public el::Application {
    static constexpr auto cFilesOption = el::StringLiteral{"file"};
    static constexpr auto cMaximumErrorsOption = el::StringLiteral{"max-errors"};

public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override;
    auto main() -> el::ExitCode override;

private:
    auto reportFailures(el::EventLoop &loop) -> el::ItemCount;

private:
    el::List<el::Path> _paths;    ///< Valid path arguments; existence is checked by each parsing job.
    el::ItemCount _maximumErrors; ///< Failed-file limit, or zero for the whole batch.
};

}
