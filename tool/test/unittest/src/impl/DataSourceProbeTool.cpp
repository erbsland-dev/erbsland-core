// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DataSourceProbeTool.hpp"

#include "DataSourceTestState.hpp"

namespace tool::test::impl {

using namespace el::text::literals;

auto DataSourceProbeTool::configurationRules() const -> el::conf::vr::RulesPtr {
    if (DataSourceTestState::shared().failure == "tool.rules"_el) {
        DataSourceTestState::shared().phase(DataSourceTestState::shared().failure);
    }
    return nullptr;
}

auto DataSourceProbeTool::usedDataSources() const -> el::StringList {
    return DataSourceTestState::shared().dependencies;
}

void DataSourceProbeTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    DataSourceTestState::shared().phase("tool.configure"_el);
    NoConfigurationTool::configure(configuration, values);
}

void DataSourceProbeTool::initialize() {
    DataSourceTestState::shared().phase("tool.initialize"_el);
}

void DataSourceProbeTool::execute() {
    DataSourceTestState::shared().phase("tool.execute"_el);
    NoConfigurationTool::execute();
}

}
