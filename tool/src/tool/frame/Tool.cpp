// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Tool.hpp"

#include <erbsland/LogicFailure.hpp>
#include <erbsland/StringFormat.hpp>

#include <utility>

namespace tool::frame {

using namespace el::text::literals;

auto Tool::optionModule() const -> el::OptionModulePtr {
    auto module = el::OptionModule::create(identifier());
    module->setHelpTitle(title());
    module->setHelpDescription(description());
    return module;
}

auto Tool::configurationRules() const -> el::conf::vr::RulesPtr {
    return nullptr;
}

auto Tool::usedDataSources() const -> el::StringList {
    return {};
}

void Tool::initialize() {
}

void Tool::configure(
    [[maybe_unused]] const el::conf::ValuePtr &configuration, [[maybe_unused]] const el::OptionValuesPtr &values) {
    // do nothing
}

auto Tool::log() const -> const el::LogStreamPtr & {
    if (_log == nullptr) {
        throwLogNotInitialized();
    }
    return _log;
}

void Tool::setLog(el::LogStreamPtr log) {
    _log = std::move(log);
}

void Tool::throwLogNotInitialized() const {
    throw el::LogicFailure{el::StringFormat{
        "Tool logging is not initialized. Tool '{}' requested its log stream before the application assigned one."_el}
            .build(identifier())};
}

}
