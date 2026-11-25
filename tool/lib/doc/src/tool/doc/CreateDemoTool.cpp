// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CreateDemoTool.hpp"

#include "impl/DemoCreation.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/cterm/ReadLine.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/FileLock.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PlatformErrorContext.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringTransformations.hpp>
#include <tool/cleanup/CleanupText.hpp>
#include <tool/frame/ToolApplication.hpp>

#include <chrono>
#include <optional>
#include <thread>

namespace tool::doc {

using namespace el::text::literals;

auto CreateDemoTool::identifier() const -> el::String {
    return "create_demo"_el;
}

auto CreateDemoTool::title() const -> el::String {
    return "Create or extend a documentation snippet collection."_el;
}

auto CreateDemoTool::description() const -> el::String {
    return "Create snippets from domain/DemoName/DemoPart; extend legacy collections in place, confirming before writing files."_el;
}

auto CreateDemoTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({cDemoPath})
        .setType(el::OptionType::Text)
        .setFlag(el::OptionFlag::Required)
        .setValueName("domain/DemoName/DemoPart"_el)
        .setHelpTitle("Demo collection and new part."_el);
    module->addOption({"--yes"_el, "-y"_el, cYes})
        .setHelpTitle("Create the reviewed files without interactive confirmation."_el);
    return module;
}

void CreateDemoTool::configure(const el::conf::ValuePtr &, const el::OptionValuesPtr &values) {
    _pathOption = values->value(cDemoPath);
    _yes = values->getFlag(cYes);
}

auto CreateDemoTool::confirm() const -> bool {
    const auto terminal = frame::app().terminal();
    if (!terminal || !terminal->isInteractive()) {
        throwError("Confirmation requires an interactive terminal; use --yes."_el);
    }
    const auto result =
        el::cterm::ReadLine::create(terminal, el::cterm::ReadLineOptions{}.setPrompt("Ok? [Y/n]: "_el))->waitForInput();
    if (!result.isCommitted()) {
        return false;
    }
    const auto answer = el::toLowercase(result.data().trimmed());
    if (answer.isEmpty() || answer == "y"_el || answer == "yes"_el) {
        return true;
    }
    if (answer == "n"_el || answer == "no"_el) {
        return false;
    }
    throwError("Please answer 'y' or 'n'."_el);
}

void CreateDemoTool::execute() {
    auto &files = frame::app().files();
    const auto lockPath = el::Path::systemTempDirectoryOrThrow() /
        el::join("erbsland-core-create-demo-"_el, cleanup::text::digest(files.projectRoot().toString()));
    auto lock = std::optional<el::path::FileLock>{};
    for (auto attempt = 0; !lock; ++attempt) {
        try {
            lock.emplace(lockPath.createLock());
        } catch (const el::PathError &error) {
            if (!error.platformContext() ||
                error.platformContext()->category() != el::PlatformErrorCategory::ResourceBusy || attempt >= 1000) {
                throw;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    }
    auto plan = [&]() -> impl::DemoCreation {
        try {
            return impl::DemoCreation{
                files, frame::app().createRenderEnvironment(frame::TemplateLanguage::Cpp), _pathOption->getText()};
        } catch (const el::ParseError &) {
            throw el::OptionError{
                "Invalid Demo Path"_el,
                "Expected <domain>/<DemoName>/<DemoPart>."_el,
                _pathOption,
                std::current_exception()};
        }
    }();
    for (const auto &[path, text] : plan.updates()) {
        (void)text;
        log()->info(el::join("Planned file: "_el, el::Path{path}.toRelative(files.projectRoot()).toString()));
    }
    for (const auto &warning : plan.warnings()) {
        log()->warn(warning);
    }
    if (!_yes && !confirm()) {
        log()->info("Aborted."_el);
        return;
    }
    plan.apply();
    log()->info(plan.documentation());
}

void CreateDemoTool::throwError(const el::String &description, const el::String &path) {
    throw el::ApplicationError{"Demo Confirmation Failed"_el, description}.withSourcePath(path);
}

}
