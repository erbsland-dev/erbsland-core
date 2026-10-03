// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/PreCommitTask.hpp"
#include "../frame/Tool.hpp"

#include <erbsland/StringLiteral.hpp>

namespace tool::doc {

/// Regenerate configured public reference interfaces and navigation.
class UpdateReferenceTool final : public frame::Tool, public frame::PreCommitTask {
    /// Configuration key for excluded header globs.
    static constexpr auto cExcludedHeaderGlobs = el::StringLiteral{"excluded_header_globs"};

public:
    // defaults
    UpdateReferenceTool() = default;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &, const el::OptionValuesPtr &) override;
    void execute() override;

private:
    el::conf::ValuePtr _configuration; ///< Validated reference policies.
};

}
