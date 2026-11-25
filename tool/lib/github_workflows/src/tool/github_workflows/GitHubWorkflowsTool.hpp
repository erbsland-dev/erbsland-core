// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GitTagCache.hpp"
#include "WorkflowFile.hpp"

#include <erbsland/StringHashMap.hpp>
#include <tool/frame/Tool.hpp>

namespace tool::github_workflows {

using namespace el::text::literals;

/// Audit external workflow pins and apply explicitly requested repairs.
class GitHubWorkflowsTool final : public frame::Tool {
    /// Option repairing verified version comments without changing pins.
    static constexpr auto cFixOption = "fix"_el;
    /// Repeatable option selecting an action and requested stable version.
    static constexpr auto cUpdateOption = "update"_el;
    /// Configuration path containing workflow YAML files.
    static constexpr auto cWorkflowDirectoryValue = "workflow_directory"_el;
    /// Configuration roots allowed to contain executable targets.
    static constexpr auto cSafeBinaryDirectoriesValue = "safe_binary_directories"_el;
    /// Configuration setting for the required Git executable.
    static constexpr auto cGitExecutableValue = "git_executable"_el;
    /// Configuration setting for the optional offline auditor.
    static constexpr auto cZizmorExecutableValue = "zizmor_executable"_el;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    auto optionModule() const -> el::OptionModulePtr override;
    auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    /// Parse all update requests before any inventory or external process starts.
    void configureUpdates(const el::OptionValuesPtr &values);
    /// Run the optional offline auditor and preserve its advisory findings.
    void runZizmor() const;
    /// Validate and stage changes for all references in one workflow.
    void audit(WorkflowFile &workflow, GitTagCache &tags, el::StringList &violations) const;
    /// Describe one reference with its workflow and physical line.
    [[nodiscard]] static auto location(const WorkflowFile &workflow, const ActionReference &reference) -> el::String;

private:
    el::Path _directory;                    ///< Confined workflow directory.
    frame::ExternalCommand _git;            ///< Required trusted Git executable.
    frame::ExternalCommand _zizmor;         ///< Optional trusted offline auditor.
    bool _fix = false;                      ///< Whether every version comment may be repaired.
    el::StringHashMap<el::String> _updates; ///< Exact action paths and requested stable tags or latest.
    el::OptionValuePtr _updateOption;       ///< Update option context for deferred resolution errors.
};

}
