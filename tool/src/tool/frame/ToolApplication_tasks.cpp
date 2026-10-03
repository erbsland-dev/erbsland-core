// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ToolApplication.hpp"

#include <erbsland/OptionError.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/Options.hpp>

namespace tool::frame {

void ToolApplication::registerTask(PreCommitTaskPtr task) {
    requireOpenRegistration();
    validateIdentifier(task->identifier());
    requireAvailableIdentifier(task->identifier());
    if (task->title().isEmpty() || task->title().characterLength() > el::CpCount{60}) {
        throwRegistrationFailure("A task requires a nonempty title of at most 60 characters."_el);
    }
    if (auto tool = std::dynamic_pointer_cast<Tool>(task)) {
        validateToolMeta(tool);
        if (!tool->requiresProject()) {
            throwRegistrationFailure("Pre-commit commands must use the shared project."_el);
        }
        _tools.append(std::move(tool));
    }
    _tasks.append(std::move(task));
}

auto ToolApplication::defaultToolOptions(const Tool &tool) const -> el::OptionValuesPtr {
    auto options = el::Options::create();
    auto module = tool.optionModule();
    options->addModule(module);
    auto arguments = el::StringList{"erbsland-core-tool"_el, module->name()};
    const auto result = el::OptionManager{options}.parse(arguments);
    if (result.errorContext()) {
        throw el::OptionError{*result.errorContext()};
    }
    if (!result.values()) {
        throw el::LogicFailure{"Task defaults did not produce command option values."_el};
    }
    return result.values();
}

void ToolApplication::preparePreCommitTask(PreCommitTask &task, PreCommitContext context) {
    task._taskContext = std::move(context);
    if (auto *tool = dynamic_cast<Tool *>(&task)) {
        prepareTool(*tool, defaultToolOptions(*tool), task.taskContext().log);
    }
}

void ToolApplication::prepareTool(Tool &tool, const el::OptionValuesPtr &values, const el::LogStreamPtr &log) {
    const auto section = validateSection(_configuration, tool.identifier(), tool.configurationRules());
    const auto sources = validateSources(_configuration, tool.usedDataSources());
    tool.setLog(log);
    tool.configure(section, values);
    initializeSources(sources, log);
    tool.initialize();
}

}
