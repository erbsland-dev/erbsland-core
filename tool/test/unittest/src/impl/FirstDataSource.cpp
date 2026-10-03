// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FirstDataSource.hpp"

#include "DataSourceTestState.hpp"

#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::test::impl {

using namespace el::text::literals;

auto FirstDataSource::configurationRules() const -> el::conf::vr::RulesPtr {
    if (DataSourceTestState::shared().failure == el::join(label(), ".rules"_el)) {
        DataSourceTestState::shared().phase(DataSourceTestState::shared().failure);
    }
    if (!DataSourceTestState::shared().withRules) {
        return nullptr;
    }
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.addRule(
        cConfiguredValue,
        el::conf::vr::RuleType::Integer,
        Title{"Test Value"_el},
        Description{"An integer supplied by validation defaults."_el},
        Default{42});
    return rules.takeRules();
}

void FirstDataSource::configure(const el::conf::ValuePtr &configuration) {
    DataSourceTestState::shared().phase(el::join(label(), ".configure"_el));
    hasConfiguration = static_cast<bool>(configuration);
    if (configuration) {
        value = configuration->getIntegerOrThrow(cConfiguredValue);
    }
    log()->trace("Source configuration logging works."_el);
}

void FirstDataSource::initialize() {
    DataSourceTestState::shared().phase(el::join(label(), ".initialize"_el));
    log()->info(el::StringFormat{"Initialized source: {}"_el}.build(label()));
}

}
