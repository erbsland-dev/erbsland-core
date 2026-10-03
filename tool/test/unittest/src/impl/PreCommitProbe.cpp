// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PreCommitProbe.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringJoins.hpp>

namespace test::impl {

using namespace el::text::literals;

auto PreCommitProbe::description() const -> el::String {
    return "Pre-commit test probe."_el;
}

auto PreCommitProbe::usedDataSources() const -> el::StringList {
    if (sourceUsers.contains(identifier())) {
        return el::StringList{"first"_el};
    }
    return {};
}

auto PreCommitProbe::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"--sample"_el, "sample"_el})
        .setType(el::OptionType::Text)
        .setDefaultValue(el::String{"default-value"_el});
    return module;
}

void PreCommitProbe::phase(const el::String &name) {
    events.append(el::join(identifier(), ":"_el, name));
    if (failures.get(identifier(), {}) == name) {
        throw el::ApplicationError{"Probe Failure"_el, el::join(identifier(), " failed during "_el, name)};
    }
}

void PreCommitProbe::configure(const el::conf::ValuePtr &, const el::OptionValuesPtr &values) {
    configured = this;
    defaultsApplied = values->getText("sample"_el) == "default-value"_el;
    phase("configure"_el);
}

void PreCommitProbe::initialize() {
    phase("initialize"_el);
}

void PreCommitProbe::execute() {
    executed = this;
    phase("execute"_el);
    const auto level = messages.get(identifier(), {});
    if (level == "warning"_el) {
        log()->warn(identifier(), " warning-detail"_el);
    } else if (level == "error"_el) {
        log()->error(identifier(), " error-detail"_el);
    }
    log()->info(identifier(), " information-detail"_el);
    log()->trace(identifier(), " trace-detail"_el);
    if (burstLogging) {
        for (auto index = 0; index < 100; ++index) {
            log()->info(identifier(), " drained-entry "_el, index);
        }
    }
    if (action) {
        action();
    }
    taskContext().progress.beginPhase("Done"_el, el::ItemCount{1});
    taskContext().progress.advance();
    progressActive = !taskContext().progress.snapshot().phase.isEmpty();
}

void PreCommitProbe::reset() {
    created = 0;
    sourceUsers = {};
    progressActive = false;
    burstLogging = false;
    events = {};
    failures = {};
    messages = {};
    action = {};
    defaultsApplied = false;
    configured = nullptr;
    executed = nullptr;
}

}
