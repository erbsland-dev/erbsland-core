// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DocumentationOutputFilter.hpp"

#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/Subprocess.hpp>
#include <tool/frame/Tool.hpp>

namespace tool::doc {

using namespace el::text::literals;

/// Build the repository's Sphinx documentation with incremental caches and concise diagnostics.
/// @tested{BuildDocToolTest}
class BuildDocTool final : public frame::Tool {
    /// Option key for --full-rebuild, discarding generated documentation caches before building.
    constexpr static auto cFullRebuildOption = "full-rebuild"_el;
    /// Option key for --no-filter, displaying the complete Sphinx output.
    constexpr static auto cNoFilterOption = "no-filter"_el;
    /// Option key for --show-suppressed, displaying accepted Doxygen warnings.
    constexpr static auto cShowSuppressedOption = "show-suppressed"_el;
    /// Configuration key listing the Doxygen warning-suppression rules.
    constexpr static auto cIgnoredWarningsConfig = "ignored_doxygen_warnings"_el;
    /// Suppression-rule key matching the warning source path against a glob.
    static constexpr auto cSourceGlobConfig = "source_glob"_el;
    /// Suppression-rule key requiring a literal substring in the warning message.
    static constexpr auto cMessageContainsConfig = "message_contains"_el;
    /// Suppression-rule key matching the warning message against a regular expression.
    static constexpr auto cMessageRegexConfig = "message_regex"_el;
    /// Suppression-rule key explaining why a matching Doxygen warning is accepted.
    static constexpr auto cReasonConfig = "reason"_el;

public:
    // defaults
    BuildDocTool() = default;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    auto optionModule() const -> el::OptionModulePtr override;
    auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    /// Remove one fixed cache directory, retrying transient removal failures.
    /// @param name The generated directory name.
    void removeGeneratedDirectory(const el::String &name);
    /// Validate the selected project virtual environment launcher.
    /// @return Its confined executable path.
    /// @throws el::ApplicationError If launcher validation fails.
    auto sphinxExecutable() const -> el::Path;
    /// Prepare the restricted environment and project working directory.
    /// @return Child launch settings.
    auto sphinxOptions() const -> el::SubprocessOptions;
    /// Run Sphinx using owner-loop output delivery and replay failures.
    /// @param executable Validated Sphinx launcher.
    /// @param arguments Serial documentation-build arguments.
    /// @return The ordinary Sphinx exit code.
    /// @throws el::ApplicationError If the native workflow fails.
    auto runFiltered(const el::Path &executable, const el::StringList &arguments) -> el::ExitCode;
    /// Retain and filter one complete output line.
    /// @param line The line without its terminator.
    void outputLine(const el::String &line);
    /// Assemble raw callback blocks into complete output lines.
    /// @param block Original subprocess output bytes.
    void outputBlock(const el::String &block);

private:
    el::Path _root;                                     ///< Selected physical project root.
    bool _fullRebuild{};                                ///< Discard generated caches before launch.
    bool _filterOutput{true};                           ///< Concise diagnostics mode.
    std::unique_ptr<DocumentationOutputFilter> _filter; ///< Configured diagnostic filter.
    el::StringEditor _pendingLine;                      ///< Line spanning output callbacks.
    el::StringList _captured;                           ///< Delivered lines retained for failure replay.
};

}
