// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeCleanupTool.hpp"

#include "CleanupConfiguration.hpp"
#include "CleanupText.hpp"
#include "CodeCleanup.hpp"

#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/system/SystemInfo.hpp>
#include <tool/frame/ToolApplication.hpp>

#include <algorithm>

namespace tool::cleanup {

using namespace el::text::literals;

auto CodeCleanupTool::identifier() const -> el::String {
    return cIdentifier;
}

auto CodeCleanupTool::title() const -> el::String {
    return "Clean up code"_el;
}

auto CodeCleanupTool::description() const -> el::String {
    return "Repair and sort includes, enforce project spacing, normalize simple CMake files, and format changed code in parallel."_el;
}

auto CodeCleanupTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({cCheckFlag, cCheckOption})
        .setHelpTitle("Report changes without writing sources or the format cache."_el);
    module->addOption({cFullFormatFlag, cFullFormatOption})
        .setHelpTitle("Run clang-format on every selected C++ file."_el);
    module->addOption({cJobsFlag, cJobsShortFlag, cJobsOption})
        .setType(el::OptionType::Integer)
        .setValueName("count"_el)
        .setHelpTitle("Parallel file workers (1–64; default: logical CPU count)."_el);
    return module;
}

auto CodeCleanupTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Code cleanup"_el},
        Description{"Source policies, explicit CMake targets and incremental formatting."_el});
    rules.addRule(
        config::cLibraryDirectory,
        RuleType::Text,
        Title{"Library directory"_el},
        Default{config::cDefaultLibraryDirectory},
        Description{"Implementation root used for include resolution."_el});
    rules.addRule(
        config::cSharedIncludePrefix,
        RuleType::Boolean,
        Default{false},
        Title{"Shared include prefix"_el},
        Description{
            "Keep external angle includes under the same public prefix when absent from this project's header map."_el});
    rules.addRule(
        config::cPublicIncludePrefix,
        RuleType::Text,
        Title{"Public include prefix"_el},
        Default{config::cDefaultPublicIncludePrefix},
        Description{
            "Compiler-visible directory prefix before mapped public headers, such as erbsland or vendor/core."_el});
    rules.addRule(
        config::cPublicHeaderMap,
        RuleType::Text,
        Title{"Public header map"_el},
        Default{config::cDefaultPublicHeaderMap},
        Description{"Canonical-first map produced by update-public-headers."_el});
    rules.addRule(
        config::cFileSuffixes,
        RuleType::ValueList,
        Title{"File suffixes"_el},
        EntryType{RuleType::Text},
        Description{"Dotted suffixes selected for C++ cleanup and local CMake inventory."_el});
    rules.addRule(
        config::cExcludedNames,
        RuleType::ValueList,
        Title{"Excluded filenames"_el},
        EntryType{RuleType::Text},
        IsOptional{},
        Description{"Exact filenames excluded from transformations, but still available for include lookup."_el});
    rules.addRule(
        config::cSources,
        RuleType::Section,
        Title{"C++ source trees"_el},
        Description{"Named, non-overlapping C++ source trees."_el});
    rules.addRule(
        config::cSourcePolicyRule,
        RuleType::Section,
        Title{"Source tree policy"_el},
        Description{"Directory selections with a uniform include policy. Names are descriptive labels."_el});
    rules.addRule(
        config::cSourceDirectoriesRule,
        RuleType::ValueList,
        Title{"Source directories"_el},
        EntryType{RuleType::Text},
        Description{"Project-relative directory globs; matched directories are scanned recursively."_el});
    rules.addRule(
        config::cSourceIncludeStyleRule,
        RuleType::Text,
        Title{"Include style"_el},
        Default{config::cPublicIncludeStyle},
        Description{"relative for the library, public for Core consumers."_el});
    rules.addRule(
        config::cSourceIncludeRootsRule,
        RuleType::ValueList,
        Title{"Local include roots"_el},
        EntryType{RuleType::Text},
        IsOptional{},
        Description{"Additional local header lookup roots; does not add cleanup targets."_el});
    rules.addRule(
        config::cCmake,
        RuleType::Section,
        Title{"CMake source lists"_el},
        Description{"Normalize supported directory-local PRIVATE source lists. Complex files are preserved."_el});
    rules.addRule(
        config::cCmakeMinimumVersion,
        RuleType::Text,
        Title{"Minimum CMake version"_el},
        Default{config::cDefaultCmakeVersion},
        Description{"Minimum version written to managed CMake files, such as 3.28."_el});
    rules.addRule(
        config::cCmakeTrees,
        RuleType::Section,
        Title{"CMake target mappings"_el},
        Description{"Explicit source-list target for each CMake root. Most specific root wins."_el});
    rules.addRule(
        config::cCmakePolicyRule,
        RuleType::Section,
        Title{"CMake tree policy"_el},
        Description{"One directory subtree mapped to an existing CMake target."_el});
    rules.addRule(
        config::cCmakeDirectoryRule,
        RuleType::Text,
        Title{"CMake tree directory"_el},
        Description{"Project-relative root containing managed CMake files."_el});
    rules.addRule(
        config::cCmakeTargetRule,
        RuleType::Text,
        Title{"CMake target"_el},
        Description{"Literal target name used by target_sources in this tree."_el});
    rules.addRule(
        config::cFormat,
        RuleType::Section,
        Title{"Incremental formatting"_el},
        Description{"Format files only when their code or the formatting configuration changes."_el});
    rules.addRule(
        config::cFormatExecutable,
        RuleType::Text,
        Title{"Formatter executable"_el},
        Default{config::cDefaultFormatter},
        Description{"Executable name on PATH or absolute path outside the project."_el});
    rules.addRule(
        config::cFormatStyleFile,
        RuleType::Text,
        Title{"Formatter style file"_el},
        Default{config::cDefaultStyleFile},
        Description{"Project-relative style file applied to every selected source."_el});
    rules.addRule(
        config::cFormatCacheFile,
        RuleType::Text,
        Title{"Formatter cache file"_el},
        Default{config::cDefaultCacheFile},
        Description{"Disposable cache; missing or invalidated cache requires a full baseline run."_el});
    return rules.takeRules();
}

void CodeCleanupTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    _policy.read(configuration, frame::app().files());
    _workers = std::clamp(el::system::info::logicalCpuCount(), 1U, 64U);
    if (values->value(cJobsOption)) {
        const auto workers = values->getInteger(cJobsOption);
        if (workers < 1 || workers > 64) {
            throw el::OptionError{
                "Invalid Worker Count"_el, "--jobs must be between 1 and 64."_el, values->value(cJobsOption)};
        }
        _workers = static_cast<unsigned int>(workers);
    }
    _checkOnly = values->getFlag(cCheckOption);
    _fullFormat = values->getFlag(cFullFormatOption);
}

void CodeCleanupTool::execute() {
    auto &application = frame::app();
    const auto environment = application.createRenderEnvironment(frame::TemplateLanguage::Cpp);
    const auto renderedHeader = environment->render("cleanup/cmake_header.cpp"_el, {});
    const auto header = renderedHeader.trimmed();
    auto cleanup = CodeCleanup{_policy, application.files(), log(), taskContext().progress};
    const auto changes = cleanup.run(_workers, _checkOnly, _fullFormat, header);
    if (_checkOnly && changes != 0) {
        text::fail("Files need cleanup. Run code-cleanup without --check."_el);
    }
}

}
