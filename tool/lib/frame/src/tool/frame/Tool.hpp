// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ToolApplication_fwd.hpp"

#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/LogStream.hpp>
#include <erbsland/OptionModule.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>

namespace tool::frame {

/// A built-in command owning its options, configuration rules, and execution.
/// @tested{ToolApplicationTest IdnaToolTest}
class Tool {
    friend class ToolApplication;

public:
    // defaults/deletions
    Tool() = default;
    virtual ~Tool() = default;
    Tool(const Tool &) = delete;
    void operator=(const Tool &) = delete;

public: // configuration and execution
    /// Get the identifier for this tool.
    /// The identifier selects its configuration section; its normalized hyphen spelling selects the command.
    /// @return The tool's identifier, must be a short lower-case name, with just letters and underscores.
    [[nodiscard]] virtual auto identifier() const -> el::String = 0;
    /// Get a short title for this tool.
    /// The title should be less than 60 characters and describe this tools function for the overview list.
    [[nodiscard]] virtual auto title() const -> el::String = 0;
    /// Get a detailed description of this tool.
    /// The tools description is used for the command help.
    [[nodiscard]] virtual auto description() const -> el::String = 0;
    /// Get the option module for this tool.
    /// The option module is required and its canonical name must match the normalized `identifier()`.
    /// **Extend** this method if your tool has additional options.
    /// Use the returned pointer directly, and add your own options to the returned object.
    /// @return Option module for this tool, must not be null.
    [[nodiscard]] virtual auto optionModule() const -> el::OptionModulePtr;
    /// Test whether this command requires project discovery and repository configuration.
    /// Independent commands receive no configuration section and load their own configuration.
    [[nodiscard]] virtual auto requiresProject() const noexcept -> bool { return true; }
    /// Get optional validation rules for this tool's configuration section.
    /// Tools with rules require a configuration section matching their identifier.
    /// Tools without rules must not have a configuration section.
    /// @return Section rules, or null if this tool has no individual configuration.
    [[nodiscard]] virtual auto configurationRules() const -> el::conf::vr::RulesPtr;
    /// Get the identifiers of data sources required by this tool.
    /// @return Source identifiers in initialization order; duplicates are initialized once.
    [[nodiscard]] virtual auto usedDataSources() const -> el::StringList;
    /// Interpret validated settings and options before required sources initialize.
    /// Keep source-dependent preparation in initialize(); configure() must not access source data.
    /// @param configuration Validated section with defaults applied, or null without rules.
    /// @param values Parsed command-line options.
    /// @throws el::conf::ConfError If configuration cannot be applied.
    /// @throws el::OptionError If an option cannot be applied.
    /// @throws el::ApplicationError If another runtime failure prevents configuration.
    virtual void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values);
    /// Prepare the configured tool after all required sources are available.
    /// The default implementation does nothing.
    /// @throws el::ApplicationError If preparation fails.
    virtual void initialize();
    /// Execute the selected module against validated configuration.
    /// Access application global data and features using `frame::app()`.
    /// Returning normally allows the application to query exitCode().
    /// Use `el::ApplicationError` for generic errors to control the exit code.
    virtual void execute() = 0;
    /// Get the exit status after normal completion, allowing commands to report aggregated failures.
    [[nodiscard]] virtual auto exitCode() const noexcept -> el::ExitCode { return el::ExitCode::success(); }

protected:
    /// The log stream to use for all console output from this tool.
    /// Trace output is only displayed if the command line option `-v` or `--verbose` is provided.
    /// @return The log stream, is never null inside `configure`, `initialize`, and `execute`.
    [[nodiscard]] auto log() const -> const el::LogStreamPtr &;

private:
    /// Report access to logging before application initialization.
    [[noreturn]] void throwLogNotInitialized() const;
    /// Set the log stream for this tool.
    /// @param log The application stream assigned before `configure()`.
    void setLog(el::LogStreamPtr log);

private:
    el::LogStreamPtr _log; ///< The log stream for all console output from this tool.
};

using ToolPtr = std::shared_ptr<Tool>;

}
