// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicHeadersTool.hpp"

#include "HeaderPolicyConfig.hpp"
#include "PublicHeaders.hpp"

#include <erbsland/all_conf_vr.hpp>
#include <erbsland/all_conf_vr_builder.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/PathInfo.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::headers {

using namespace el::text::literals;

auto PublicHeadersTool::identifier() const -> el::String {
    return "update_public_headers"_el;
}

auto PublicHeadersTool::title() const -> el::String {
    return "Update public headers"_el;
}

auto PublicHeadersTool::description() const -> el::String {
    return "Generate canonical includes, compatibility aliases, domain umbrellas, namespaces, and header mappings."_el;
}

auto PublicHeadersTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"--check"_el, cCheckOption})
        .setHelpTitle("Check generated headers"_el)
        .setHelpDescription("Report outdated or obsolete outputs without changing any files."_el);
    module->addOption({"--jobs"_el, "-j"_el, cJobsOption})
        .setType(el::OptionType::Integer)
        .setValueName("count"_el)
        .setHelpTitle("Parallel workers"_el)
        .setHelpDescription(
            "Use 1 to 64 read, parsing and rendering workers; default: the system's logical CPU count."_el);
    return module;
}

auto PublicHeadersTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Public headers"_el}, Description{"Publication, namespace and umbrella policies for the library."_el});
    rules.addRule(
        config::cSourceBaseDirectory,
        RuleType::Text,
        Title{"Source base directory"_el},
        Description{"Project-relative root of the library source tree."_el},
        Default{"src/erbsland"_el});
    rules.addRule(
        config::cIncludeBaseDirectory,
        RuleType::Text,
        Title{"Include base directory"_el},
        Description{"Project-relative destination for public wrappers."_el},
        Default{"include/erbsland"_el});
    rules.addRule(
        config::cBaseNamespace,
        RuleType::Text,
        Title{"Base namespace"_el},
        Description{"The outer C++ namespace shared by all configured domains."_el},
        Default{"erbsland"_el});
    rules.addRule(
        config::cGenerateNamespaceHeaders,
        RuleType::Boolean,
        Default{true},
        Title{"Generate namespace headers"_el},
        Description{"Publish namespace declarations and exports alongside domain wrappers."_el});
    rules.addRule(
        config::cNamespacesHeader,
        RuleType::Text,
        Title{"Namespace declarations"_el},
        Description{"Source-relative output declaring all configured namespaces."_el},
        Default{"core/Namespaces.hpp"_el});
    rules.addRule(
        config::cMakeOneNamespaceHeader,
        RuleType::Text,
        Title{"Namespace exports"_el},
        Description{"Source-relative output importing consumer symbols and aliases."_el},
        Default{"core/impl/MakeOneNamespace.hpp"_el});
    rules.addRule(
        config::cMappingFile,
        RuleType::Text,
        Title{"Header mapping"_el},
        Description{"Project-relative canonical-first source/include mapping output."_el},
        Default{"tool/data/public_headers/header_map.txt"_el});

    rules.addRule(
        config::cExcludedDirectories,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded directories"_el},
        Description{"Directory basenames excluded from publication and coverage checks."_el});
    rules.addRule(
        config::cExcludedHeaders,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded headers"_el},
        Description{"Global filename-only globs excluding headers from publication."_el});
    rules.addRule(
        config::cExcludedFromAllHeaders,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Global umbrella exclusions"_el},
        Description{"Filename-only globs excluded from every umbrella while retaining individual wrappers."_el});
    rules.addRule(
        config::cMergeableHeaderNames,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Mergeable header names"_el},
        Description{"Header filenames allowed to aggregate multiple source headers."_el});
    rules.addRule(
        config::cDeclaredNamespaces,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Declared namespaces"_el},
        Description{"Additional namespace declarations, relative to the base namespace."_el});
    rules.addRule(
        config::cGeneratedMarkers,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Generated markers"_el},
        Description{"Line-comment prefixes recognizing generated headers; the first prefix is emitted."_el});
    rules.addRule(
        config::cNamespaceAliases,
        RuleType::Section,
        IsOptional{},
        Title{"Namespace aliases"_el},
        Description{"Named aliases exported into the base namespace."_el});
    rules.addRule(
        "namespace_aliases.vr_any"_el,
        RuleType::Text,
        Title{"Alias target"_el},
        Description{"Qualified target namespace relative to the base namespace."_el});
    rules.addRule(
        config::cHeaderAliases,
        RuleType::Section,
        IsOptional{},
        Title{"Header aliases"_el},
        Description{"Explicit compatibility wrappers, grouped by an arbitrary entry name."_el});
    rules.addRule(
        "header_aliases.vr_any"_el,
        RuleType::Section,
        Title{"Header alias"_el},
        Description{"One source-to-include compatibility relationship."_el});
    auto aliases = rules.createBranch("header_aliases.vr_any"_el);
    aliases.addRule(
        "source_header"_el,
        RuleType::Text,
        Title{"Source header"_el},
        Description{"Header path relative to the source base."_el});
    aliases.addRule(
        "include_header"_el,
        RuleType::Text,
        Title{"Include header"_el},
        Description{"Compatibility path relative to the include base."_el});
    rules.addRule(
        config::cDirectory,
        RuleType::Section,
        Title{"Source directories"_el},
        Description{"Directory sections mirroring the source tree; every non-excluded directory must be covered."_el});
    // Cover the full ten-component name-path depth supported by ELCL documents.
    constexpr auto cDirectoryPolicyPath = "directory.vr_any"_el;
    rules.addRule(
        cDirectoryPolicyPath,
        RuleType::Section,
        Title{"Domain directory policy"_el},
        Description{"Publication and namespace settings for one top-level source domain."_el});
    auto directory = rules.createBranch(cDirectoryPolicyPath);
    addDirectoryRules(directory, 10);
    return rules.takeRules();
}

void PublicHeadersTool::addDirectoryRules(el::conf::vr::RulesBuilder &rules, const std::size_t remainingDepth) {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    rules.configureRoot(
        Title{"Directory policy"_el}, Description{"Publication policy inherited by organizational children."_el});
    rules.addRule(
        config::cFlatten,
        RuleType::Boolean,
        IsOptional{},
        Title{"Flatten"_el},
        Description{"Import this namespace or selected symbols into the base namespace."_el});
    rules.addRule(
        config::cNestedNamespace,
        RuleType::Boolean,
        IsOptional{},
        Title{"Nested namespace"_el},
        Description{"Append this directory name to its parent's C++ namespace."_el});
    rules.addRule(
        config::cTargetDirectory,
        RuleType::Text,
        IsOptional{},
        Title{"Target directory"_el},
        Description{
            "Per-entry include destination; defaults to the root when flattened, otherwise the source directory. Empty text selects the root."_el});
    rules.addRule(
        config::cTargetNamespace,
        RuleType::Text,
        IsOptional{},
        Title{"Target namespace"_el},
        Description{"Export a namespace alias instead of a namespace-wide import."_el});
    rules.addRule(
        config::cIncludedHeaders,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Included headers"_el},
        Description{"Filename-only globs selecting public headers in this directory."_el});
    rules.addRule(
        config::cExcludedHeaders,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded headers"_el},
        Description{"Filename-only globs excluding headers from publication."_el});
    rules.addRule(
        config::cExcludedFromFlattening,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Flattening exclusions"_el},
        Description{"Filename-only globs retaining source-shaped canonical wrappers."_el});
    rules.addRule(
        config::cExcludedFromAllHeaders,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Umbrella exclusions"_el},
        Description{"Filename-only globs excluding opt-in headers from umbrellas."_el});
    rules.addRule(
        config::cIncludedSymbols,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Included symbols"_el},
        Description{"Explicit type exports replacing a namespace-wide import."_el});
    if (remainingDepth > 1) {
        constexpr auto cChildRuleName = "vr_any"_el;
        rules.addRule(
            cChildRuleName,
            RuleType::Section,
            Title{"Child directory policy"_el},
            Description{"Publication settings inherited from the parent directory and locally overridden."_el});
        auto child = rules.createBranch(cChildRuleName);
        addDirectoryRules(child, remainingDepth - 1);
    }
}

void PublicHeadersTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    _policy.read(configuration);
    const auto &files = frame::app().files();
    [[maybe_unused]] const auto source =
        files.checked(configuration->valueOrThrow(config::cSourceBaseDirectory), el::PathType::Directory);
    for (const auto &key : el::StringList{config::cIncludeBaseDirectory, config::cMappingFile}) {
        const auto value = configuration->valueOrThrow(key);
        const auto path = files.checked(value, true);
        const auto info = path.info();
        if (!info.exists()) {
            continue;
        }
        if (key == config::cIncludeBaseDirectory && !info.isDirectory()) {
            throw el::conf::ConfError{
                value, "Invalid Include Directory"_el, "The include destination must be a directory."_el};
        }
        if (key == config::cMappingFile && !info.isRegularFile()) {
            throw el::conf::ConfError{
                value, "Invalid Mapping File"_el, "The header mapping destination must be a regular file."_el};
        }
    }
    _checkOnly = values->getFlag(cCheckOption);
    _workerCount = el::system::info::logicalCpuCount();
    if (values->value(cJobsOption)) {
        const auto workers = values->getInteger(cJobsOption);
        if (workers < 1 || workers > 64) {
            throw el::OptionError{
                "Invalid Worker Count"_el, "The --jobs count must be between 1 and 64."_el, values->value(cJobsOption)};
        }
        _workerCount = static_cast<unsigned int>(workers);
    }
}

void PublicHeadersTool::execute() {
    auto generator = PublicHeaders{_policy, frame::app().files(), log(), _workerCount, taskContext().progress};
    generator.prepare();
    const auto changes = generator.apply(_checkOnly);
    if (_checkOnly && changes != 0) {
        HeaderPolicy::throwError(
            "Generated public headers or mappings are out of date. Run update-public-headers without --check."_el);
    }
    log()->info("Public header outputs changed: "_el, changes);
}

}
