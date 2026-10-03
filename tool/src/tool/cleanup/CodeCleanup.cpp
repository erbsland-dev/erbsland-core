// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeCleanup.hpp"

#include "ClangFormat.hpp"
#include "CleanupResult.hpp"
#include "CleanupText.hpp"
#include "CMakeCleanup.hpp"
#include "CppCleanup.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/WorkerPool.hpp>

#include <algorithm>

namespace tool::cleanup {

using namespace el::text::literals;

CodeCleanup::CodeCleanup(
    const CleanupPolicy &policy, const frame::ProjectFiles &files, el::LogStreamPtr log, frame::TaskProgress progress) :
    _progress{std::move(progress)},
    _policy{policy},
    _files{files},
    _log{std::move(log)},
    _index{policy, files.projectRoot()} {
}

void CodeCleanup::inventory() {
    auto roots = el::PathList{_policy.library};
    for (const auto &tree : _policy.trees) {
        roots.append(tree.directory);
        for (const auto &root : tree.includeRoots) {
            roots.append(root);
        }
    }
    for (const auto &[root, target] : _policy.cmake) {
        roots.append(root);
    }
    roots.sort(
        [](const el::Path &left, const el::Path &right) -> bool { return left.elementCount() < right.elementCount(); });
    auto trees = el::List<SourceTreePtr>{};
    for (const auto &tree : _policy.trees) {
        trees.append(std::make_shared<const SourceTree>(tree));
    }
    auto walked = el::PathList{};
    for (const auto &root : roots) {
        if (std::any_of(walked.begin(), walked.end(), [&](const el::Path &parent) -> bool {
                return root.isRelativeTo(parent);
            })) {
            continue;
        }
        walked.append(root);
        root.walker().walkOrThrow(
            [&](const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus {
                if (!info.isRegularFile()) {
                    return el::PathWalkStatus::Continue;
                }
                _index.add(path);
                if (_policy.selects(path)) {
                    const auto directory = path.parent().toString();
                    _localSources.set(directory, _localSources.get(directory, {}).append(path.name()));
                    for (const auto &tree : trees) {
                        if (path.isRelativeTo(tree->directory)) {
                            _inputs.append({path, tree});
                            break;
                        }
                    }
                } else if (path.name() == "CMakeLists.txt"_el && !_policy.cmakeTarget(path.parent()).isEmpty()) {
                    _inputs.append({path, nullptr});
                }
                return el::PathWalkStatus::Continue;
            },
            el::PathWalkOptions{}.setSymlinkMode(el::SymlinkMode::Skip));
    }
    _inputs.sort([](const Input &left, const Input &right) -> bool { return left.path < right.path; });
    _index.readMapping(_files.readText(_policy.mapping));
}

auto CodeCleanup::run(
    const unsigned int workers, const bool checkOnly, const bool fullFormat, const el::String &cmakeHeader)
    -> std::size_t {
    _log->info("Inventorying C++ sources, include roots and CMake files..."_el);
    _progress.beginPhase("Discovering cleanup inputs"_el);
    inventory();
    const auto formatter = ClangFormat{_policy, _files, fullFormat};
    const auto cpp = CppCleanup{_index};
    const auto cmake = CMakeCleanup{_policy, _index};
    const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(workers));
    _log->info("Cleaning "_el, _inputs.count(), " files with "_el, workers, " workers..."_el);
    _progress.beginPhase("Cleaning files"_el, _inputs.count());
    const auto results = pool->map(
                                 _inputs,
                                 [&](const Input &input) -> CleanupResult {
                                     if (input.tree) {
                                         auto result = cleanCpp(input, cpp, formatter);
                                         _progress.advance();
                                         return result;
                                     }
                                     auto result = cleanCMake(input, cmake, cmakeHeader);
                                     _progress.advance();
                                     return result;
                                 })
                             .get();
    return applyResults(results, formatter, checkOnly);
}

auto CodeCleanup::cleanCpp(const Input &input, const CppCleanup &cpp, const ClangFormat &formatter) const
    -> CleanupResult {
    auto result = CleanupResult{_files.readWalkedTextFile(input.path)};
    result.text = cpp.apply(input.path, result.file.text(), *input.tree);
    result.fingerprint = CppCleanup::codeFingerprint(result.text);
    if (!formatter.needsFormat(input.path, result.fingerprint)) {
        return result;
    }
    const auto formatted = formatter.format(input.path);
    if (formatted.trimmed().isEmpty() && !result.file.text().trimmed().isEmpty()) {
        text::fail("clang-format returned empty output for a nonempty source."_el, input.path.toString());
    }
    result.text = cpp.apply(input.path, formatted, *input.tree);
    result.fingerprint = CppCleanup::codeFingerprint(result.text);
    result.formatted = true;
    return result;
}

auto CodeCleanup::cleanCMake(const Input &input, const CMakeCleanup &cmake, const el::String &header) const
    -> CleanupResult {
    auto result = CleanupResult{_files.readWalkedTextFile(input.path)};
    const auto sources = _localSources.get(input.path.parent().toString(), {});
    const auto output = cmake.apply(input.path, result.file.text(), sources, header);
    result.skipped = !output.has_value();
    result.text = output.value_or(result.file.text());
    return result;
}

void CodeCleanup::writeCache(const el::String &text) const {
    const auto cache = _files.checked(_policy.cache, true);
    if (!cache.parent().info().isDirectory()) {
        cache.parent().operations().createDirectoryOrThrow(el::PathCreateDirectoryOptions{}.setCreateParents(true));
    }
    if (_files.updateText(cache, text)) {
        _log->trace("Updated the formatting cache."_el);
    }
}

auto CodeCleanup::applyResults(
    const el::List<CleanupResult> &results, const ClangFormat &formatter, const bool checkOnly) -> std::size_t {
    std::size_t changes = 0;
    std::size_t formatted = 0;
    std::size_t skipped = 0;
    auto fingerprints = std::vector<std::pair<el::String, el::String>>{};
    _progress.beginPhase("Publishing cleaned files"_el, results.count());
    // No source or cache writes are allowed until every transformation has succeeded.
    for (const auto &result : results) {
        const auto relative = result.file.path().toRelativeOrThrow(_files.projectRoot()).toString();
        if (result.formatted) {
            ++formatted;
        }
        if (result.skipped) {
            ++skipped;
            _log->trace("Preserved complex CMake file: "_el, relative);
        }
        if (!result.fingerprint.isEmpty()) {
            fingerprints.emplace_back(relative, result.fingerprint);
        }
        if (!_files.isUnchanged(result.file, result.text) &&
            _files.updateText(result.file.path(), result.text, checkOnly)) {
            ++changes;
            _log->info("Changed: "_el, relative);
        }
        _progress.advance();
    }
    if (!checkOnly) {
        writeCache(formatter.cacheText(fingerprints));
    }
    _log->info(
        "Changed "_el,
        changes,
        " files; clang-format processed "_el,
        formatted,
        "; preserved "_el,
        skipped,
        " complex CMake files."_el);
    return changes;
}

}
