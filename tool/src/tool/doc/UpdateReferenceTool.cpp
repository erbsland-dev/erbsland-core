// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UpdateReferenceTool.hpp"

#include "impl/ReferenceDocumentation.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/conf/vr/RulesBuilderBranch.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::doc {

using namespace el::text::literals;

auto UpdateReferenceTool::identifier() const -> el::String {
    return "doc_update_reference"_el;
}

auto UpdateReferenceTool::title() const -> el::String {
    return "Update reference documentation"_el;
}

auto UpdateReferenceTool::description() const -> el::String {
    return "Scan public headers assigned to reference groups, preserve page prose, remove orphan pages and update sorted navigation."_el;
}

auto UpdateReferenceTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using namespace el::conf::vr::builder;
    using el::conf::vr::RuleType;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Reference maintenance"_el},
        Description{"Header selections and managed reference documentation groups."_el});
    rules.addRule(
        "source_directory"_el,
        RuleType::Text,
        Default{"src/erbsland"_el},
        Title{"Source directory"_el},
        Description{"Project-relative library headers to scan."_el});
    rules.addRule(
        "reference_directory"_el,
        RuleType::Text,
        Default{"doc/reference"_el},
        Title{"Reference directory"_el},
        Description{"Project-relative root containing managed reference pages."_el});
    rules.addRule(
        "exclude_underscore_headers"_el,
        RuleType::Boolean,
        Default{true},
        Title{"Exclude underscore headers"_el},
        Description{"Skip headers whose filenames contain an underscore."_el});
    rules.addRule(
        "excluded_directory_names"_el,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded directories"_el},
        Description{"Directory basenames omitted from header discovery."_el});
    rules.addRule(
        "excluded_header_names"_el,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded headers"_el},
        Description{"Exact header filenames omitted from discovery."_el});
    rules.addRule(
        cExcludedHeaderGlobs,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded header patterns"_el},
        Description{"Globs matching source-relative header paths to omit."_el});
    rules.addRule(
        "manual_reference_pages"_el,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Manual reference pages"_el},
        Description{"Reference-relative RST pages preserved without generating their interfaces."_el});
    rules.addRule(
        "reference_groups"_el,
        RuleType::SectionList,
        IsOptional{},
        Title{"Reference groups"_el},
        Description{"Page assignments for groups of public headers."_el});
    rules.addRule(
        "reference_groups.vr_entry"_el,
        RuleType::Section,
        Title{"Reference group"_el},
        Description{"One reference page and the headers contributing its interface."_el});
    auto entry = rules.createBranch("reference_groups.vr_entry"_el);
    entry.addRule(
        "page"_el,
        RuleType::Text,
        Minimum{1},
        Title{"Reference page"_el},
        Description{"Unique RST path below the reference directory, excluding index.rst."_el});
    entry.addRule(
        "title"_el,
        RuleType::Text,
        Default{""_el},
        Title{"Page title"_el},
        Description{"Heading for a new page; an empty title is derived from its filename."_el});
    entry.addRule(
        "headers"_el,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Group headers"_el},
        Description{"Exact source-relative header paths assigned to this page."_el});
    entry.addRule(
        "header_globs"_el,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Group header patterns"_el},
        Description{"Path globs selecting additional headers for this page."_el});
    entry.addRule(
        cExcludedHeaderGlobs,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded group headers"_el},
        Description{"Globs removing headers from this group selection."_el});
    return rules.takeRules();
}

void UpdateReferenceTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &) {
    _configuration = configuration;
}

void UpdateReferenceTool::execute() {
    taskContext().progress.beginPhase("Discovering reference inputs"_el);
    auto documentation = impl::ReferenceDocumentation{frame::app().files(), _configuration};
    for (const auto &header : documentation.unassignedHeaders()) {
        log()->warn(el::join("Public header is not assigned to a reference group: "_el, header));
    }
    documentation.run(taskContext().progress);
}

}
