// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HeaderPolicy.hpp"

#include "../frame/PreCommitTask.hpp"
#include "../frame/Tool.hpp"

#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/system/SystemInfo.hpp>

namespace tool::headers {

using namespace el::text::literals;

/// Generate the public include tree and namespace exports from an explicit directory policy.
class PublicHeadersTool final : public frame::Tool, public frame::PreCommitTask {
    /// Option key for --check, reporting stale public headers without writing files.
    constexpr static auto cCheckOption = "check"_el;
    /// Option key for --jobs, limiting parallel public-header workers.
    constexpr static auto cJobsOption = "jobs"_el;

public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    /// Configure the current directory-policy section and add its relative child rules.
    /// @param rules The builder rooted at the existing directory-policy section.
    /// @param remainingDepth The maximum policy levels to declare, including the current section.
    static void addDirectoryRules(el::conf::vr::RulesBuilder &rules, std::size_t remainingDepth);

private:
    HeaderPolicy _policy;                                            ///< Validated command configuration.
    bool _checkOnly = false;                                         ///< Whether to report changes without writing.
    unsigned int _workerCount = el::system::info::logicalCpuCount(); ///< System hint or explicit worker override.
};

}
