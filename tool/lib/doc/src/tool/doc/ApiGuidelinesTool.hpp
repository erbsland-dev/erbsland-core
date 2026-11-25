// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tool/frame/PreCommitTask.hpp>
#include <tool/frame/Tool.hpp>

namespace tool::doc {

/// Validate API guideline pages.
class ApiGuidelinesTool final : public frame::Tool, public frame::PreCommitTask {
public:
    // defaults
    ApiGuidelinesTool() = default;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    void execute() override;
};

}
