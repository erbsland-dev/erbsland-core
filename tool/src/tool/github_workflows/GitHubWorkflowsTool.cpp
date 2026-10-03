// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GitHubWorkflowsTool.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/CodeLocation.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringHashSet.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::github_workflows {

using namespace el::text::literals;

auto GitHubWorkflowsTool::identifier() const -> el::String {
    return "github_workflows"_el;
}

auto GitHubWorkflowsTool::title() const -> el::String {
    return "Audit workflow pins and version comments."_el;
}

auto GitHubWorkflowsTool::description() const -> el::String {
    return "Verify external action SHA pins and stable version comments without writing. Use --fix for comments or --update ACTION@VERSION for explicit pin changes."_el;
}

auto GitHubWorkflowsTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"--fix"_el, cFixOption})
        .setHelpTitle("Repair version comments"_el)
        .setHelpDescription("Repair all verified action annotations while preserving their SHA pins."_el);
    module->addOption({"--update"_el, cUpdateOption})
        .setType(el::OptionType::Text)
        .setMaximum(el::ArgumentCount::infinite())
        .setValueName("ACTION@VERSION"_el)
        .setHelpTitle("Update an action pin"_el)
        .setHelpDescription(
            "Repeat for complete action paths; VERSION is latest or vMAJOR.MINOR.PATCH. Updates also repair the selected action's comments."_el);
    return module;
}

auto GitHubWorkflowsTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using namespace el::conf::vr;
    using namespace el::conf::vr::builder;
    auto rules = RulesBuilder{};
    rules.configureRoot(
        Title{"GitHub workflows"_el}, Description{"Workflow inputs and trusted external audit commands."_el});
    rules.addRule(
        cWorkflowDirectoryValue,
        RuleType::Text,
        Default{".github/workflows"_el},
        Minimum{1},
        Title{"Workflow directory"_el},
        Description{
            "Project-confined directory containing YAML workflow files; nested directories are not scanned."_el});
    rules.addRule(
        cSafeBinaryDirectoriesValue,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        Minimum{1},
        Title{"Safe binary directories"_el},
        Description{
            "Absolute trusted roots for resolved Git and zizmor executables; project executables are rejected."_el});
    rules.addRule(
        cGitExecutableValue,
        RuleType::Text,
        Default{"git"_el},
        Minimum{1},
        Title{"Git executable"_el},
        Description{"Required executable basename or absolute path used for remote stable-tag verification."_el});
    rules.addRule(
        cZizmorExecutableValue,
        RuleType::Text,
        Default{"zizmor"_el},
        Minimum{1},
        Title{"Zizmor executable"_el},
        Description{"Optional executable basename or absolute path for offline workflow auditing."_el});
    return rules.takeRules();
}

void GitHubWorkflowsTool::configureUpdates(const el::OptionValuesPtr &values) {
    _updateOption = values->value(cUpdateOption);
    for (const auto &request : values->getTextList(cUpdateOption)) {
        const auto parts = request.splitAtLast("@"_el);
        if (!parts || !WorkflowFile::isAction(parts->first) ||
            (parts->second != "latest"_el && !StableTag::fromString(parts->second))) {
            throw el::OptionError{
                "Invalid Action Update"_el,
                el::join("Expected owner/repository[/subpath]@latest or @vMAJOR.MINOR.PATCH: "_el, request),
                _updateOption};
        }
        if (_updates.contains(parts->first) && _updates.get(parts->first, {}) != parts->second) {
            throw el::OptionError{
                "Conflicting Action Updates"_el,
                el::join("Specify only one version for "_el, parts->first),
                _updateOption};
        }
        _updates.set(parts->first, parts->second);
    }
}

void GitHubWorkflowsTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    _fix = values->getFlag(cFixOption);
    configureUpdates(values);
    const auto &files = frame::app().files();
    _directory = files.checked(configuration->valueOrThrow(cWorkflowDirectoryValue), el::PathType::Directory);
    const auto directories =
        frame::ExternalCommand::directories(configuration->valueOrThrow(cSafeBinaryDirectoriesValue));
    _git.configure(configuration->valueOrThrow(cGitExecutableValue), directories, files.projectRoot());
    _zizmor.configure(configuration->valueOrThrow(cZizmorExecutableValue), directories, files.projectRoot(), false);
}

void GitHubWorkflowsTool::runZizmor() const {
    if (!_zizmor.isAvailable()) {
        log()->warn("Trusted zizmor is unavailable; skipping the optional offline audit."_el);
        return;
    }
    const auto result = _zizmor.run(
        el::StringList{"--offline"_el, "--collect=workflows"_el, _directory.toNativeString()},
        frame::app().files().projectRoot());
    if (!result.output.isEmpty()) {
        log()->info(result.output);
    }
    if (!result.error.isEmpty()) {
        log()->warn(result.error);
    }
    const auto code = result.status.exitCode();
    if (code != 0 && code != 11 && code != 12 && code != 13 && code != 14) {
        throw el::ApplicationError{"Zizmor Failed"_el, "The optional auditor did not complete normally."_el}
            .withSourcePath(_directory.toString());
    }
}

auto GitHubWorkflowsTool::location(const WorkflowFile &workflow, const ActionReference &reference) -> el::String {
    return el::StringFormat{"{}:{}: {}"_el}.build(
        workflow.path().toString(), reference.line.toRawValue() + 1, reference.action);
}

void GitHubWorkflowsTool::audit(WorkflowFile &workflow, GitTagCache &tags, el::StringList &violations) const {
    for (const auto &reference : workflow.references()) {
        const auto request = _updates.get(reference.action, {});
        if (request.isEmpty() && !WorkflowFile::isSha(reference.ref)) {
            violations.append(el::join(location(workflow, reference), " is not pinned to a full SHA."_el));
            continue;
        }
        auto catalog = std::shared_ptr<const TagCatalog>{};
        try {
            catalog = tags.get(reference.repository);
        } catch (const el::ApplicationError &error) {
            throw el::ApplicationError{
                el::ApplicationErrorContext{}
                    .setTitle(error.title())
                    .setDescription(error.description())
                    .setSourcePath(workflow.path().toString())
                    .setCodeLocation(
                        el::CodeLocation{el::LineIndex::fromInteger(reference.line.toRawValue()), el::ColumnIndex{}}),
                std::current_exception()};
        }
        auto tag = std::optional<ActionTag>{};
        if (request == "latest"_el) {
            tag = catalog->latest();
        } else if (!request.isEmpty()) {
            tag = catalog->named(request);
        } else {
            tag = catalog->forSha(reference.ref);
        }
        if (!tag) {
            if (!request.isEmpty()) {
                throw el::OptionError{
                    "Requested Action Version Unavailable"_el,
                    el::join(reference.action, "@"_el, request, " has no stable remote tag."_el),
                    _updateOption};
            }
            violations.append(
                el::join(
                    location(workflow, reference),
                    " has no stable tag for its SHA; its comment cannot be verified."_el));
            continue;
        }
        if (!request.isEmpty()) {
            // A requested tag selects the commit; annotations always identify its highest stable tag.
            tag = catalog->forSha(tag->sha);
            workflow.repair(reference, tag->sha, tag->version.toString());
        } else if (_fix) {
            workflow.repair(reference, {}, tag->version.toString());
        } else if (!workflow.hasAnnotation(reference, tag->version.toString())) {
            violations.append(
                el::join(
                    location(workflow, reference),
                    " requires comment # "_el,
                    reference.action,
                    "@"_el,
                    tag->version.toString(),
                    ". Use --fix."_el));
        }
        const auto latest = catalog->latest();
        if (latest && latest->version > tag->version) {
            log()->info(
                reference.action, "@"_el, tag->version.toString(), " (latest "_el, latest->version.toString(), ")"_el);
        } else {
            log()->info(reference.action, "@"_el, tag->version.toString(), " (up-to-date)"_el);
        }
    }
}

void GitHubWorkflowsTool::execute() {
    const auto &files = frame::app().files();
    auto inputs = el::List<frame::ProjectTextFile>{};
    _directory.walker().walkOrThrow(
        [&](const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus {
            if (info.isDirectory() && path != _directory) {
                return el::PathWalkStatus::Skip;
            }
            if (info.isRegularFile() &&
                (path.suffix().compare(".yml"_el, el::Char::compareAsciiFolded) == 0 ||
                    path.suffix().compare(".yaml"_el, el::Char::compareAsciiFolded) == 0)) {
                try {
                    inputs.append(files.readWalkedTextFile(path));
                } catch (const el::ApplicationError &error) {
                    throw error.withSourcePath(path.toString());
                }
            }
            return el::PathWalkStatus::Continue;
        },
        el::PathWalkOptions{}.setSymlinkMode(el::SymlinkMode::Skip));
    auto workflows = el::List<WorkflowFile>{};
    auto bomModes = el::List<el::StringBomMode>{};
    auto actions = el::StringHashSet{};
    for (const auto &input : inputs) {
        auto bomMode = el::StringBomMode::Automatic;
        if ((_fix || !_updates.count().isZero()) && files.readData(input.path()).startsWith({0xef, 0xbb, 0xbf})) {
            bomMode = el::StringBomMode::Require;
        }
        bomModes.append(bomMode);
        auto workflow = WorkflowFile{input.path(), input.text()};
        for (const auto &reference : workflow.references()) {
            actions.insert(reference.action);
        }
        workflows.append(std::move(workflow));
    }
    for (const auto &[action, version] : _updates) {
        if (!actions.contains(action)) {
            throw el::OptionError{"Action Not Used"_el, el::join("No workflow uses "_el, action), _updateOption};
        }
    }
    runZizmor();
    auto tags = GitTagCache{_git, files.projectRoot()};
    auto violations = el::StringList{};
    auto outputs = el::StringList{};
    for (auto workflow : workflows) {
        audit(workflow, tags, violations);
        outputs.append(workflow.text());
    }
    if (!violations.isEmpty()) {
        throw el::ApplicationError{"Workflow Audit Failed"_el, violations.join("\n"_el)};
    }
    // Verify every original snapshot before replacing any file.
    if (_fix || !_updates.count().isZero()) {
        for (const auto &input : inputs) {
            if (!files.isUnchanged(input, input.text())) {
                throw el::ApplicationError{
                    "Workflow Changed During Audit"_el, "Run the audit again against the current workflow contents."_el}
                    .withSourcePath(input.path().toString());
            }
        }
        for (auto index = el::ItemIndex{}; index.isWithin(inputs.count()); ++index) {
            files.updateText(inputs.get(index).path(), outputs.get(index), false, bomModes.get(index));
        }
    }
    log()->info("Verified "_el, actions.count(), " external actions in "_el, workflows.count(), " workflow files."_el);
}

}
