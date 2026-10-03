// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/PreCommitTask.hpp"
#include "../frame/Tool.hpp"

namespace tool::doc {

/// Validate API test-status markers.
class ApiTestStatusTool final : public frame::Tool, public frame::PreCommitTask {
public:
    // defaults
    ApiTestStatusTool() = default;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    auto optionModule() const -> el::OptionModulePtr override;
    void configure(const el::conf::ValuePtr &, const el::OptionValuesPtr &) override;
    void execute() override;

private:
    bool _normalize{false}; ///< Normalize coverage markers before validation.
};

}
