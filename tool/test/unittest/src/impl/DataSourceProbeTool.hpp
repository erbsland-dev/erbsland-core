// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../NoConfigurationTool.hpp"

namespace tool::test::impl {

using namespace el::text::literals;

/// Tool fixture exposing each post-source callback.
class DataSourceProbeTool : public NoConfigurationTool {
public: // implement Tool
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    [[nodiscard]] auto usedDataSources() const -> el::StringList override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void initialize() override;
    void execute() override;
};

}
