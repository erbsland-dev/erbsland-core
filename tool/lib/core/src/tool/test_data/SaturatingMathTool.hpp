// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FixtureTool.hpp"

namespace tool::test_data {

/// Generate exact independent expectations for saturating integer operations.
class SaturatingMathTool final : public FixtureTool {
public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    void execute() override;

protected: // implement FixtureTool
    [[nodiscard]] auto defaultDirectory() const -> el::String override;
};

}
