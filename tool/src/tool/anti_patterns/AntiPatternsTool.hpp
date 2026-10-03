// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Policy.hpp"
#include "RuleRegistry.hpp"

#include "../frame/PreCommitTask.hpp"
#include "../frame/Tool.hpp"

#include <erbsland/ItemCount.hpp>
#include <erbsland/Literals.hpp>

namespace tool::anti_patterns {

using namespace el::text::literals;

/// Scan project C++ sources for documented anti-patterns.
class AntiPatternsTool final : public frame::Tool, public frame::PreCommitTask {
    /// Option key for --jobs, limiting parallel source scanners.
    constexpr static auto cJobsOption = "jobs"_el;
    /// Option key for --limit, limiting the findings shown in the report.
    constexpr static auto cLimitOption = "limit"_el;
    /// Positional option key for the project paths to scan.
    constexpr static auto cPathsOption = "paths"_el;

public:
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    RuleRegistry _rules;                 ///< Independently registered detectors.
    Policy _policy;                      ///< Validated configuration.
    el::OptionValuePtr _requestedOption; ///< Positional argument context for scan-input errors.
    el::StringList _requested;           ///< Optional project-relative targets.
    unsigned int _workers = 1;           ///< Bounded file worker count.
    el::ItemCount _limit{20};            ///< Maximum report rows.
    bool _showSuppressed = false;        ///< Show accepted findings in the report.
};

}
