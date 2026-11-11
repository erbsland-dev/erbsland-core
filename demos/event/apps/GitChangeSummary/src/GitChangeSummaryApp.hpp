// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GitStatusReport.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/ManagedEventThread.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PlatformErrorContext.hpp>
#include <erbsland/String.hpp>
#include <erbsland/Subprocess.hpp>

namespace demo {

/// Run Git from procedural main and receive subprocess notifications on a dedicated event loop.
class GitChangeSummaryApp final : public el::Application {
    static constexpr auto cDirectoryOption = el::StringLiteral{"directory"};

public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override;
    [[nodiscard]] auto main() -> el::ExitCode override;
    void cleanup() noexcept override;

private:
    /// Find the Git executable using the platform's PATH rules.
    static auto findGit() -> el::Path;
    /// Bind the prepared subprocess to the current worker loop and launch it.
    void onStart();
    /// Consume a block of machine-readable Git output on the owner loop.
    void onStandardOutput(const el::String &chunk);
    /// Remember Git's exit status and reject unsuccessful results.
    void onExit(el::ExitCode code);
    /// Forward native subprocess failures to the application error boundary.
    void onError(const el::PlatformErrorContext &error);
    /// Reject output loss because it would make the report incomplete.
    void onOutputTruncated();
    /// End worker dispatch after the subprocess finishes.
    void onFinal();
    /// Print the completed report after the worker has joined.
    void printReport() const;

private:
    el::Path _directory;               ///< Repository directory selected before starting the worker.
    el::SubprocessPtr _process;        ///< Created without a loop; retained through worker shutdown.
    el::ManagedEventThreadPtr _worker; ///< Runs the source's owner loop and is joined before reading results.
    GitStatusReport _report;           ///< Written on the worker; read by main only after joining.
    el::ExitCode _exitCode{el::ExitCode::failure()}; ///< Success is recorded only after complete output validation.
};

}
