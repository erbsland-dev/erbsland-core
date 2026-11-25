// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FixtureTool.hpp"

namespace tool::test_data {

/// Generate independent regular-expression category expectations from the verified UCD snapshot.
class CategoryTestDataTool final : public FixtureTool {
public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto usedDataSources() const -> el::StringList override;
    void execute() override;

protected: // implement FixtureTool
    [[nodiscard]] auto defaultDirectory() const -> el::String override;
};

}
