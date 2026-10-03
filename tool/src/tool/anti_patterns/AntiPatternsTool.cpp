// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AntiPatternsTool.hpp"

#include "Report.hpp"
#include "Scanner.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/all_conf_vr.hpp>
#include <erbsland/all_conf_vr_builder.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionFlag.hpp>
#include <erbsland/system/SystemInfo.hpp>

#include <algorithm>

namespace tool::anti_patterns {

using namespace el::text::literals;

auto AntiPatternsTool::identifier() const -> el::String {
    return "anti_patterns"_el;
}

auto AntiPatternsTool::title() const -> el::String {
    return "Check anti-patterns"_el;
}

auto AntiPatternsTool::description() const -> el::String {
    return "Scan project C++ sources for documented anti-patterns and report accepted exceptions."_el;
}

auto AntiPatternsTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({cPathsOption})
        .setType(el::OptionType::Text)
        .setMaximum(el::ArgumentCount{65535U})
        .setFlag(el::OptionFlag::Greedy)
        .setValueName("path"_el)
        .setHelpTitle("Project-relative source files or directories; omit to scan configured roots."_el);
    module->addOption({"--jobs"_el, "-j"_el, cJobsOption})
        .setType(el::OptionType::Integer)
        .setValueName("count"_el)
        .setHelpTitle("Parallel file workers (1–64; default: logical CPU count)."_el);
    module->addOption({"--limit"_el, cLimitOption})
        .setType(el::OptionType::Integer)
        .setValueName("rows"_el)
        .setHelpTitle("Maximum report rows (1–200; default: 20)."_el);
    module->addOption({"--show-suppressed"_el, "show_suppressed"_el})
        .setType(el::OptionType::Flag)
        .setHelpTitle("Include accepted findings in the report."_el);
    return module;
}

auto AntiPatternsTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Anti-pattern scan"_el}, Description{"Source selection and rule policies for the C++ quality scan."_el});
    rules.addRule(
        "source_directories"_el,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        Title{"Source directories"_el},
        Description{"Project-relative directory globs selecting C++ source trees to scan."_el});
    rules.addRule(
        "file_suffixes"_el,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        Title{"File suffixes"_el},
        Description{"Dotted suffixes identifying source files to scan."_el});
    rules.addRule(
        "exception"_el,
        RuleType::SectionList,
        IsOptional{},
        Title{"Accepted findings"_el},
        Description{"Documented exceptions for specific paths and anti-pattern rules."_el});
    auto exceptions = rules.createBranch("exception"_el);
    exceptions.addRule(
        "vr_entry"_el,
        RuleType::Section,
        Title{"Accepted finding"_el},
        Description{"Paths, rule identifiers and the reason for accepting their findings."_el});
    auto entry = exceptions.createBranch("vr_entry"_el);
    entry.addRule(
        "path"_el,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        Title{"Exception paths"_el},
        Description{"Project-relative globs matching files with accepted findings."_el});
    entry.addRule(
        "rules"_el,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        Title{"Exception rules"_el},
        Description{"Registered anti-pattern rule identifiers covered by this exception."_el});
    entry.addRule(
        "reason"_el,
        RuleType::Text,
        Minimum{1},
        Title{"Exception reason"_el},
        Description{"Explanation of why the selected findings are accepted."_el});
    _rules.addConfigurationRules(rules);
    return rules.takeRules();
}

void AntiPatternsTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    _rules.configure(configuration);
    _policy.read(configuration, frame::app().files(), _rules);
    _requestedOption = values->value(cPathsOption);
    _requested = values->getTextList(cPathsOption);
    _workers = std::clamp(el::system::info::logicalCpuCount(), 1U, 64U);
    if (values->value(cJobsOption)) {
        const auto count = values->getInteger(cJobsOption);
        if (count < 1 || count > 64) {
            throw el::OptionError{
                "Invalid Worker Count"_el, "--jobs must be between 1 and 64."_el, values->value(cJobsOption)};
        }
        _workers = static_cast<unsigned int>(count);
    }
    if (values->value(cLimitOption)) {
        const auto count = values->getInteger(cLimitOption);
        if (count < 1 || count > 200) {
            throw el::OptionError{
                "Invalid Finding Limit"_el, "--limit must be between 1 and 200."_el, values->value(cLimitOption)};
        }
        _limit = el::ItemCount::fromInteger(static_cast<el::ItemCount::Value>(count));
    }
    _showSuppressed = values->getFlag("show_suppressed"_el);
}

void AntiPatternsTool::execute() {
    auto scanner = Scanner{_policy, _rules, frame::app().files(), taskContext().progress};
    const auto findings = scanner.scan(_requested, _workers, _requestedOption);
    const auto report = Report{};
    if (!report.activeCount(findings).isZero()) {
        log()->error(report.render(findings, _limit, _showSuppressed));
    } else {
        log()->info(report.render(findings, _limit, _showSuppressed));
    }
    log()->trace(
        "Scanned "_el,
        scanner.fileCount(),
        " files ("_el,
        scanner.cacheHits(),
        " cache hits, "_el,
        scanner.cacheMisses(),
        " misses)."_el);
    if (!report.activeCount(findings).isZero()) {
        throw el::ApplicationError{"Anti-Patterns Found"_el, "Resolve or document the reported findings."_el};
    }
}

}
