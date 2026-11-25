// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProcessRstTool.hpp"

#include "impl/DemoSynchronizer.hpp"
#include "impl/RstCleaner.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/conf/vr/RulesBuilderBranch.hpp>
#include <erbsland/OptionChoices.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::doc {

using namespace el::text::literals;

auto ProcessRstTool::identifier() const -> el::String {
    return "doc_process_rst"_el;
}

auto ProcessRstTool::title() const -> el::String {
    return "Clean RST and optionally process demo blocks."_el;
}

auto ProcessRstTool::description() const -> el::String {
    return "Normalize headings, paragraphs and API links; explicitly select demo validation, synchronization, refresh or metadata migration."_el;
}

auto ProcessRstTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({cTarget})
        .setType(el::OptionType::Text)
        .setFlag(el::OptionFlag::Required)
        .setValueName("path"_el)
        .setHelpTitle("RST file or directory to process."_el);
    module->addOption({"--recursive"_el, "-r"_el, cRecursive}).setHelpTitle("Process nested directories."_el);
    module->addOption({"--force"_el, cForce}).setHelpTitle("Regenerate demo blocks even when hashes match."_el);
    module->addOption({"--check"_el, cCheck})
        .setHelpTitle("Fail if changes are required, without writing documents."_el);
    module->addOption({"--dry-run"_el, "-d"_el, cDryRun})
        .setHelpTitle("Report proposed changes without writing documents."_el);
    module->addOption({"--no-cleanup"_el, cNoCleanup})
        .setHelpTitle("Only perform explicitly selected demo processing."_el);
    auto choices = el::OptionChoices::create();
    choices->addChoice("validate"_el).setHelpTitle("Check whether demo blocks are synchronized."_el);
    choices->addChoice("sync"_el).setHelpTitle("Regenerate stale demo blocks."_el);
    choices->addChoice("refresh"_el).setHelpTitle("Recursively regenerate every demo block."_el);
    choices->addChoice("migrate"_el).setHelpTitle("Convert legacy demo metadata without running demos."_el);
    module->addOption({"--demos"_el, cDemos})
        .setChoices(choices)
        .setValueName("mode"_el)
        .setHelpTitle("Enable demo processing; omitted by default."_el);
    return module;
}

auto ProcessRstTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"RST processing"_el}, Description{"Reflow, link resolution and demo synchronization settings."_el});
    rules.addRule(
        cLineWidth,
        el::conf::vr::RuleType::Integer,
        Default{120},
        Minimum{20},
        Title{"Paragraph width"_el},
        Description{"Maximum character width when reflowing RST paragraphs."_el});
    rules.addRule(
        cDoxygenIndex,
        el::conf::vr::RuleType::Text,
        IsOptional{},
        Title{"Doxygen index"_el},
        Description{"Project-relative XML index used to resolve API links."_el});
    rules.addRule(
        cDemoExecutableRoot,
        el::conf::vr::RuleType::Text,
        Default{"cmake-build-debug/demo-apps"_el},
        Title{"Demo executable directory"_el},
        Description{"Project-relative directory containing compiled demo applications."_el});
    rules.addRule(
        cDemoTextReplacement,
        el::conf::vr::RuleType::SectionList,
        IsOptional{},
        Title{"Demo text replacements"_el},
        Description{"Ordered replacements applied to selected demo source excerpts."_el});
    rules.addRule(
        "demo_text_replacement.vr_entry"_el,
        el::conf::vr::RuleType::Section,
        Title{"Demo text replacement"_el},
        Description{"One regular expression and replacement applied to demo source."_el});
    auto replacement = rules.createBranch("demo_text_replacement.vr_entry"_el);
    replacement.addRule(
        cReplacementPattern,
        el::conf::vr::RuleType::RegEx,
        Title{"Source pattern"_el},
        Description{"Regular expression matching source text to replace."_el});
    replacement.addRule(
        cReplacementText,
        el::conf::vr::RuleType::Text,
        Title{"Replacement text"_el},
        Description{"Replacement expression whose group references must exist in the pattern."_el});
    return rules.takeRules();
}

void ProcessRstTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    const auto &files = frame::app().files();
    _target = files.checked(values->value(cTarget));
    if (!_target.info().isDirectory() && (!_target.info().isRegularFile() || _target.suffix() != ".rst"_el)) {
        throw el::OptionError{
            "Invalid RST Input"_el, "Expected an .rst file or a directory."_el, values->value(cTarget)};
    }
    _recursive = values->getFlag(cRecursive);
    _force = values->getFlag(cForce);
    _check = values->getFlag(cCheck);
    _dryRun = values->getFlag(cDryRun);
    _cleanup = !values->getFlag(cNoCleanup);
    _demos = values->getText(cDemos);
    if (_demos == "refresh"_el) {
        _force = true;
        _recursive = true;
    }
    _lineWidth = configuration->getOrThrow<int>(cLineWidth);
    if (const auto index = configuration->value(cDoxygenIndex)) {
        _doxygenIndex = files.checked(index, true);
    } else {
        const auto project = frame::app().project().cmakeName;
        _doxygenIndex = files.checked(el::Path{"_build/breathe/doxygen"_el} / project / "xml/index.xml"_el, true);
    }
    _demoExecutableRoot = files.checked(configuration->valueOrThrow(cDemoExecutableRoot), true);
    _demoTextReplacements.clear();
    if (const auto entries = configuration->getSectionList(cDemoTextReplacement)) {
        for (const auto &entry : *entries) {
            auto pattern = entry->getRegExOrThrow(cReplacementPattern);
            auto replacement = entry->getTextOrThrow(cReplacementText);
            try {
                pattern->validateReplacement(replacement);
            } catch (const el::re::RegExError &) {
                throw el::conf::ConfError{
                    entry->valueOrThrow(cReplacementText),
                    "Invalid Demo Text Replacement"_el,
                    "The replacement must use valid group references for its pattern."_el,
                    std::current_exception()};
            }
            _demoTextReplacements.append({std::move(pattern), std::move(replacement)});
        }
    }
    if (!_cleanup && _demos.isEmpty()) {
        throw el::OptionError{
            "Invalid RST Processing Options"_el,
            "Select --demos when using --no-cleanup."_el,
            values->value(cNoCleanup)};
    }
}

void ProcessRstTool::execute() {
    auto &files = frame::app().files();
    auto index = impl::DoxygenIndex{};
    if (_cleanup) {
        index.read(files, _doxygenIndex);
    }
    auto cleaner = impl::RstCleaner{index, _lineWidth};
    auto demos = impl::DemoSynchronizer{files, _demoExecutableRoot, _demoTextReplacements};
    auto changed = false;
    auto validationChanged = false;
    auto issues = el::StringList{};
    const auto root = files.checked(_target);
    root.walker().walkOrThrow(
        [&](const el::Path &path) -> el::PathWalkStatus {
            const auto original = files.readWalkedTextFile(path).text();
            auto updated = original;
            if (_demos == "migrate"_el) {
                updated = impl::DemoSynchronizer::migrate(updated);
            } else if (!_demos.isEmpty()) {
                updated = demos.process(path, updated, _force, issues);
                if (_demos == "validate"_el && updated != original) {
                    validationChanged = true;
                    updated = original;
                }
            }
            if (_cleanup) {
                auto warnings = el::StringList{};
                updated = cleaner.process(updated, warnings);
                for (const auto &warning : warnings) {
                    log()->warn(el::join(path.toString(), ": "_el, warning));
                }
            }
            const auto differs = files.updateText(path, updated, _check || _dryRun);
            if (differs) {
                changed = true;
                log()->info(el::join("changed: "_el, path.toRelative(files.projectRoot()).toString()));
            }
            return el::PathWalkStatus::Continue;
        },
        el::PathWalkOptions{}
            .setTypes(el::PathType::RegularFile)
            .setSymlinkMode(el::SymlinkMode::Skip)
            .setFilter([](const el::Path &path, el::PathType) -> bool { return path.suffix() == ".rst"_el; })
            .setDescendFilter([&](const el::Path &path, el::PathType) -> bool { return _recursive || path == root; }));
    if (!issues.isEmpty()) {
        throwError(issues.join("\n"_el));
    }
    if (validationChanged) {
        throwError("Demo documentation is not synchronized."_el);
    }
    if (_check && changed) {
        throwError("RST documents require processing."_el);
    }
}

void ProcessRstTool::throwError(const el::String &description, const el::String &path) {
    throw el::ApplicationError{"RST Processing Failed"_el, description}.withSourcePath(path);
}

}
