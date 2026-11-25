// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DependencyRule.hpp"
#include "IncludeGraph.hpp"

#include <tool/frame/ExternalCommand.hpp>
#include <tool/frame/Tool.hpp>

namespace tool::include_dependencies {

using namespace el::text::literals;

/// Inspect and validate explicit source-level library include dependencies.
class IncludeDependenciesTool final : public frame::Tool {
    /// Positional option selecting the dependency query.
    static constexpr auto cModeOption = "mode"_el;
    /// Positional option containing source paths or a rule name.
    static constexpr auto cArgumentsOption = "arguments"_el;
    /// Option selecting incoming include edges.
    static constexpr auto cReverseOption = "reverse"_el;
    /// Option bounding tree traversal depth.
    static constexpr auto cMaxDepthOption = "max-depth"_el;
    /// Option selecting JSON or DOT serialization.
    static constexpr auto cFormatOption = "format"_el;
    /// Option naming an ignored, untracked report file.
    static constexpr auto cOutputOption = "output"_el;
    /// Option listing every missing dependency.
    static constexpr auto cAllOption = "all"_el;
    /// Configuration path to the canonical publication map.
    static constexpr auto cPublicHeaderMapValue = "public_header_map"_el;
    /// Configuration list of named public-header requirements.
    static constexpr auto cRequiredDependenciesValue = "required_dependencies"_el;
    /// Configuration setting for the trusted Git executable.
    static constexpr auto cGitExecutableValue = "git_executable"_el;
    /// Configuration roots allowed to contain executable targets.
    static constexpr auto cSafeBinaryDirectoriesValue = "safe_binary_directories"_el;

    /// Unique configuration name selecting one dependency rule.
    static constexpr auto cRuleNameValue = "name"_el;
    /// Source-relative configuration path that public headers must reach.
    static constexpr auto cRuleTargetValue = "target"_el;
    /// Configuration paths exempt from one dependency rule.
    static constexpr auto cExcludedHeadersValue = "excluded_headers"_el;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    auto optionModule() const -> el::OptionModulePtr override;
    auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    /// Validate mode-specific arity and options before accessing source files.
    void configureOptions(const el::OptionValuesPtr &values);
    /// Validate the export destination and its Git ownership before scanning.
    void configureOutput(const el::OptionValuesPtr &values, const frame::ExternalCommand &git);
    /// Resolve all configured rule paths against the inventory.
    void resolveRules(const IncludeGraph &graph);
    /// Resolve one user argument, preserving the command-line diagnostic context.
    [[nodiscard]] auto resolveArgument(const IncludeGraph &graph, el::ItemIndex index) const -> el::String;
    /// Render and enforce selected dependency rules.
    [[nodiscard]] auto check(const IncludeGraph &graph) const -> el::String;
    /// Require a canonical source-relative configured path.
    static void validateRelative(const el::conf::ValuePtr &value);

private:
    el::String _mode;                    ///< Selected query mode.
    el::StringList _arguments;           ///< Mode-specific source or rule arguments.
    el::OptionValuesPtr _values;         ///< Retained option context for inventory-dependent checks.
    el::Path _sourceRoot;                ///< Confined library source directory.
    el::Path _output;                    ///< Optional prevalidated report destination.
    el::String _format;                  ///< JSON or DOT export selection.
    bool _reverse = false;               ///< Follow incoming edges for tree output.
    bool _all = false;                   ///< Include all missing headers in check diagnostics.
    std::optional<el::ItemCount> _depth; ///< Optional tree depth limit.
    headers::PublicHeaderMap _mapping;   ///< Validated publication membership.
    el::List<DependencyRule> _rules;     ///< Rules with original configuration context.
};

}
