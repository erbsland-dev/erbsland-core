// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicHeaders.hpp"

#include "HeaderDeclarations.hpp"
#include "HeaderPolicyConfig.hpp"
#include "HeaderReadResult.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/frame/ToolApplication.hpp>

#include <utility>

namespace tool::headers {

using namespace el::text::literals;

PublicHeaders::PublicHeaders(
    HeaderPolicy policy,
    const frame::ProjectFiles &files,
    el::LogStreamPtr log,
    const unsigned int workers,
    frame::TaskProgress progress) :
    _progress{std::move(progress)},
    _policy{std::move(policy)},
    _files{files},
    _log{std::move(log)},
    _workerCount{workers},
    _sourceBase{files.checked(_policy.sourceBase())},
    _includeBase{checkedOutput(_policy.includeBase())} {
}

void PublicHeaders::prepare() {
    const auto workers = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(_workerCount));
    _log->info("Discovering public headers and verifying namespaces..."_el);
    _progress.beginPhase("Discovering public headers"_el);
    discover(*workers);
    validateHeaders(*workers);
    _log->info("Planning public wrappers and namespace exports..."_el);
    _progress.beginPhase("Planning public headers"_el);
    preserveAliases();
    if (_policy.generatesNamespaceHeaders()) {
        createNamespaces();
    }
    createUmbrellas();
    createWrappers();
    createMapping();
    renderOutputs(*workers);
    _log->info("Checking obsolete outputs and validating the output plan..."_el);
    _progress.beginPhase("Validating output plan"_el);
    collectObsolete();
    validatePlan();
}

void PublicHeaders::discover(el::WorkerPool &workers) {
    if (!_sourceBase.info().isDirectory()) {
        HeaderPolicy::throwError(
            "The source base is not a directory."_el,
            _policy.configuration()->valueOrThrow(config::cSourceBaseDirectory));
    }
    auto seenDirectories = el::StringSet{};
    auto uncovered = el::StringList{};
    auto inputs = std::vector<std::pair<el::Path, bool>>{};
    inventory(_sourceBase, true, seenDirectories, uncovered, inputs);
    for (const auto &[path, policy] : _policy.directories()) {
        if (!seenDirectories.contains(path)) {
            HeaderPolicy::throwError(
                el::StringFormat{"Configured directory '{}' does not exist or is excluded."_el}.build(path),
                _policy.configuration()->valueOrThrow(config::cDirectory));
        }
    }
    if (!uncovered.isEmpty()) {
        HeaderPolicy::throwError(
            el::StringFormat{"Unconfigured source directories:\n{}"_el}.build(uncovered.sorted().join("\n"_el)));
    }
    inventory(_includeBase, false, seenDirectories, uncovered, inputs);
    _progress.beginPhase("Reading headers"_el, el::ItemCount::fromSizeT(inputs.size()));
    auto results = workers
                       .map(
                           inputs,
                           [this](const auto &input) -> HeaderReadResult {
                               const auto &[path, selected] = input;
                               auto result = HeaderReadResult{_files.readWalkedTextFile(path), {}};
                               if (selected) {
                                   result.declarations = std::make_shared<const HeaderDeclarations>(result.file.text());
                               }
                               _progress.advance();
                               return result;
                           })
                       .get();
    for (auto &result : results) {
        const auto name = result.file.path().toString();
        if (result.declarations) {
            _scans.set(name, std::move(result.declarations));
        }
        _existingFiles.set(name, std::move(result.file));
    }
}

void PublicHeaders::inventory(
    const el::Path &base,
    const bool sourceTree,
    el::StringSet &seenDirectories,
    el::StringList &uncovered,
    std::vector<std::pair<el::Path, bool>> &inputs) {
    if (!base.info().isDirectory()) {
        return;
    }
    const auto flattenPath = _sourceBase / _policy.flattenHeader();
    const auto namespacesPath = _sourceBase / _policy.namespacesHeader();
    base.walker().walkOrThrow(
        [&](const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus {
            if (path == base) {
                return el::PathWalkStatus::Continue;
            }
            const auto relative = path.toRelativeOrThrow(base);
            auto excluded = false;
            if (sourceTree) {
                const auto directory = info.isDirectory() ? relative : relative.parent();
                for (const auto &element : directory.elements()) {
                    excluded = excluded || _policy.excludedDirectories().contains(element);
                }
            }
            if (info.isDirectory()) {
                if (sourceTree && !excluded) {
                    const auto relativeName = relative.toString();
                    HeaderPolicy::validatePath(relativeName);
                    _log->trace("Scanning directory: "_el, relative);
                    seenDirectories.insert(relativeName);
                    if (!_policy.directories().contains(relativeName)) {
                        uncovered.append(relativeName);
                    }
                }
            } else if (info.isRegularFile() && path.name().endsWith(".hpp"_el)) {
                const auto selected = sourceTree && !excluded && selectInput(path, relative);
                // Excluded implementation headers belong to other generators. Only their old umbrellas are ours.
                if (!excluded || path.name() == "all.hpp"_el || path == flattenPath || path == namespacesPath) {
                    inputs.emplace_back(path, selected);
                }
            }
            return el::PathWalkStatus::Continue;
        },
        el::PathWalkOptions{}
            .setSymlinkMode(el::SymlinkMode::Skip)
            .setFilter([](const el::Path &path, const el::PathType type) -> bool {
                return type == el::PathType::Directory ||
                    (type == el::PathType::RegularFile && path.name().endsWith(".hpp"_el));
            }));
}

auto PublicHeaders::selectInput(const el::Path &path, const el::Path &relative) -> bool {
    const auto name = path.name();
    if (name == "all.hpp"_el || name == "fwd.hpp"_el || _policy.matches(name, _policy.excludedHeaders())) {
        return false;
    }
    const auto directoryName = relative.parent().toString();
    const auto directory = _policy.directories().get(directoryName);
    if (!directory || !_policy.matches(name, directory->includedHeaders) ||
        _policy.matches(name, directory->excludedHeaders)) {
        return false;
    }
    const auto relativeName = relative.toString();
    HeaderPolicy::validatePath(relativeName);
    _inputs.set(relativeName, directoryName);
    return true;
}

void PublicHeaders::scanForwardHeaders(el::WorkerPool &workers) {
    auto inputs = std::vector<std::pair<el::String, el::String>>{};
    for (const auto &[source, directory] : _inputs) {
        const auto path = _sourceBase / source;
        const auto name = path.withName(el::join(path.stem(), "_fwd.hpp"_el)).toString();
        if (_scans.contains(name)) {
            continue;
        }
        if (const auto file = _existingFiles.get(name)) {
            inputs.emplace_back(name, file->text());
        }
    }
    _progress.beginPhase("Scanning forward declarations"_el, el::ItemCount::fromSizeT(inputs.size()));
    auto results = workers
                       .map(
                           inputs,
                           [this](const auto &input) -> std::shared_ptr<const HeaderDeclarations> {
                               auto result = std::make_shared<const HeaderDeclarations>(input.second);
                               _progress.advance();
                               return result;
                           })
                       .get();
    auto result = results.begin();
    for (const auto &[name, text] : inputs) {
        _scans.set(name, *result);
        ++result;
    }
}

void PublicHeaders::validateHeaders(el::WorkerPool &workers) {
    scanForwardHeaders(workers);
    _progress.beginPhase("Validating declarations"_el, _inputs.count());
    for (const auto &[source, directory] : _inputs) {
        inspectHeader(_sourceBase / source, *_policy.directories().get(directory));
        _progress.advance();
    }
    for (const auto &[path, policy] : _policy.directories()) {
        for (const auto &symbol : policy.includedSymbols) {
            if (!_selectedExports.get(policy.sourceNamespace, {}).contains(symbol)) {
                HeaderPolicy::throwError(
                    el::StringFormat{"Selected symbol '{}::{}' was not found in published headers."_el}.build(
                        policy.sourceNamespace, symbol));
            }
        }
    }
}

void PublicHeaders::validateNamespaces(
    const HeaderDeclarations &scan, const DirectoryPolicy &directory, const el::String &relative) {
    for (const auto &name : scan.namespaces()) {
        if (name != "std"_el && !name.startsWith("std::"_el) && name != directory.sourceNamespace &&
            !isChildNamespace(name, directory.sourceNamespace) && !isChildNamespace(directory.sourceNamespace, name)) {
            HeaderPolicy::throwError(
                el::StringFormat{"Header '{}' declares '{}', expected '{}' or an auxiliary child namespace."_el}.build(
                    relative, name, directory.sourceNamespace));
        }
    }
    for (const auto &declaration : scan.declarations()) {
        const auto &name = declaration.sourceNamespace;
        if (name != "std"_el && !name.startsWith("std::"_el) && name != directory.sourceNamespace &&
            !isChildNamespace(name, directory.sourceNamespace)) {
            HeaderPolicy::throwError(
                el::StringFormat{"Header '{}' declares type '{}' in '{}', expected '{}'."_el}.build(
                    relative, declaration.name, name, directory.sourceNamespace));
        }
    }
}

auto PublicHeaders::selectExports(
    const HeaderDeclarations &declarations,
    const HeaderDeclarations &forward,
    const DirectoryPolicy &directory,
    const el::String &relative,
    const el::Path &forwardPath) -> bool {
    auto selected = false;
    for (const auto &symbol : directory.includedSymbols) {
        const HeaderDeclaration *declaration = nullptr;
        auto fromForward = false;
        for (const auto &candidate : declarations.declarations()) {
            if (candidate.sourceNamespace == directory.sourceNamespace && candidate.name == symbol) {
                declaration = &candidate;
            }
        }
        for (const auto &candidate : forward.declarations()) {
            if (candidate.sourceNamespace == directory.sourceNamespace && candidate.name == symbol) {
                declaration = &candidate;
                fromForward = true;
            }
        }
        if (declaration == nullptr) {
            continue;
        }
        selected = true;
        _selectedExports.set(
            directory.sourceNamespace, _selectedExports.get(directory.sourceNamespace, {}).insert(symbol));
        if (fromForward) {
            _forwardIncludes.insert(forwardPath.toRelativeOrThrow(_sourceBase).toString());
        } else if (!declaration->forwardText.isEmpty()) {
            _forwardDeclarations.set(
                directory.sourceNamespace,
                _forwardDeclarations.get(directory.sourceNamespace, {}).insert(declaration->forwardText));
        } else {
            HeaderPolicy::throwError(
                el::StringFormat{
                    "Cannot safely forward-declare '{}::{}' from '{}'; provide its canonical _fwd.hpp declaration."_el}
                    .build(directory.sourceNamespace, symbol, relative));
        }
    }
    return selected;
}

void PublicHeaders::inspectHeader(const el::Path &path, const DirectoryPolicy &directory) {
    const auto relative = path.toRelativeOrThrow(_sourceBase).toString();
    const auto declarations = _scans.get(path.toString()).value();
    const auto forwardPath = path.withName(el::join(path.stem(), "_fwd.hpp"_el));
    static const auto empty = std::make_shared<const HeaderDeclarations>(el::String{});
    const auto forward = _scans.get(forwardPath.toString(), empty);
    if (!_policy.generatesNamespaceHeaders() || relative != _policy.namespacesHeader()) {
        validateNamespaces(*declarations, directory, relative);
        validateNamespaces(*forward, directory, relative);
    }
    const auto selected = selectExports(*declarations, *forward, directory, relative, forwardPath);
    auto target = directory.targetDirectory;
    if ((!directory.includedSymbols.isEmpty() && !selected) ||
        _policy.matches(path.name(), directory.excludedFromFlattening)) {
        target = directory.directory;
    }
    auto header = PublicHeader{
        relative,
        (_includeBase / target).joinedNative(path.name()).toRelativeOrThrow(_includeBase).toString(),
        el::StringSet{relative},
        false,
        _policy.matches(path.name(), _policy.excludedFromAll()) ||
            _policy.matches(path.name(), directory.excludedFromAll)};
    _headers.set(relative, std::move(header));
}

}
