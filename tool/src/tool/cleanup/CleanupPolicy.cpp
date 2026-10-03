// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CleanupPolicy.hpp"

#include "CleanupConfiguration.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/PathGlobPattern.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/RegEx.hpp>

#include <algorithm>

namespace tool::cleanup {

using namespace el::text::literals;

auto CleanupPolicy::directories(const el::conf::ValuePtr &patterns, const frame::ProjectFiles &files) -> el::PathList {
    if (patterns->toValueList().empty()) {
        throwConfigurationError(patterns, "Source Directories must not be empty."_el);
    }
    auto matched = el::PathList{};
    for (const auto &value : patterns->toValueList()) {
        const auto text = value->asTextOrThrow();
        const auto name = text.trimmed();
        const auto relative = el::Path{name};
        if (name.isEmpty() || !relative.isRelative() || relative.elements().contains(".."_el)) {
            throwConfigurationError(value, "Source directory patterns must be safe project-relative globs."_el);
        }
        auto pattern = el::PathGlobPatternPtr{};
        try {
            pattern = el::PathGlobPattern::compile(name);
        } catch (const el::ParseError &) {
            throwConfigurationError(value, "Invalid source directory glob."_el, std::current_exception());
        }
        auto found = false;
        files.projectRoot().walker().walkOrThrow(
            [&](const el::Path &directory, [[maybe_unused]] const el::PathInfo &info) -> el::PathWalkStatus {
                found = true;
                if (!matched.contains(directory)) {
                    matched.append(directory);
                }
                return el::PathWalkStatus::Continue;
            },
            el::PathWalkOptions{}
                .setGlobPattern(pattern)
                .setTypes(el::PathType::Directory)
                .setSymlinkMode(el::SymlinkMode::Skip));
        if (!found) {
            throwConfigurationError(value, "Source directory glob matches no directories."_el);
        }
    }
    matched.sort([](const el::Path &left, const el::Path &right) -> bool {
        if (left.elementCount() != right.elementCount()) {
            return left.elementCount() < right.elementCount();
        }
        return left < right;
    });
    auto result = el::PathList{};
    for (const auto &directory : matched) {
        if (!std::any_of(result.begin(), result.end(), [&](const el::Path &parent) -> bool {
                return directory.isRelativeTo(parent);
            })) {
            result.append(directory);
        }
    }
    result.sort();
    return result;
}

auto CleanupPolicy::path(const el::conf::ValuePtr &value, const frame::ProjectFiles &files, const bool directory)
    -> el::Path {
    const auto name = value->asTextOrThrow();
    const auto relative = el::Path{name};
    if (name.isEmpty() || !relative.isRelative() || relative.elements().contains(".."_el)) {
        throwConfigurationError(value, "Configuration paths must be nonempty project-relative paths without '..'."_el);
    }
    const auto result = files.checked(value, !directory);
    if (directory && !result.info().isDirectory()) {
        throwConfigurationError(value, "Configured input directory does not exist."_el);
    }
    return result;
}

void CleanupPolicy::read(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files) {
    trees.clear();
    cmake.clear();
    readOptions(configuration, files);
    readSourceTrees(configuration, files);
    readCmakeTrees(configuration, files);
}

void CleanupPolicy::readOptions(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files) {
    library = path(configuration->valueOrThrow(config::cLibraryDirectory), files, true);
    const auto prefix = configuration->getTextOrThrow(config::cPublicIncludePrefix);
    static const auto prefixPattern = el::RegEx::compile(R"/([A-Za-z0-9_.\-]+(?:/[A-Za-z0-9_.\-]+)*)/"_el);
    publicIncludePrefix = el::Path{prefix};
    if (!prefixPattern->fullMatch(prefix) || publicIncludePrefix.elements().contains("."_el) ||
        publicIncludePrefix.elements().contains(".."_el)) {
        throwConfigurationError(
            configuration->valueOrThrow(config::cPublicIncludePrefix),
            "Public Include Prefix must be a nonempty relative include directory without '.' or '..'."_el);
    }
    mapping = path(configuration->valueOrThrow(config::cPublicHeaderMap), files);
    style = path(configuration->valueOrThrow(config::cFormatStyleFile), files);
    if (!mapping.info().isRegularFile()) {
        throwConfigurationError(
            configuration->valueOrThrow(config::cPublicHeaderMap),
            "The public header map must be an existing regular file."_el);
    }
    if (!style.info().isRegularFile()) {
        throwConfigurationError(
            configuration->valueOrThrow(config::cFormatStyleFile),
            "The formatter style must be an existing regular file."_el);
    }
    cache = path(configuration->valueOrThrow(config::cFormatCacheFile), files);
    formatterValue = configuration->valueOrThrow(config::cFormatExecutable);
    cmakeVersion = configuration->getTextOrThrow(config::cCmakeMinimumVersion);
    suffixes = configuration->getStringList(config::cFileSuffixes);
    excludedNames = configuration->getStringList(config::cExcludedNames);
    static const auto versionPattern = el::RegEx::compile(R"/([0-9]+(?:\.[0-9]+){1,3})/"_el);
    static const auto suffixPattern = el::RegEx::compile(R"/(\.[A-Za-z0-9]+)/"_el);
    if (suffixes.isEmpty()) {
        throwConfigurationError(
            configuration->valueOrThrow(config::cFileSuffixes), "File Suffixes must not be empty."_el);
    }
    if (formatterValue->asTextOrThrow().isEmpty()) {
        throwConfigurationError(
            configuration->valueOrThrow(config::cFormatExecutable), "Formatter executable must not be empty."_el);
    }
    if (!versionPattern->fullMatch(cmakeVersion)) {
        throwConfigurationError(
            configuration->valueOrThrow(config::cCmakeMinimumVersion),
            "CMake minimum version must use dotted decimal components."_el);
    }
    for (const auto &suffix : suffixes) {
        if (!suffixPattern->fullMatch(suffix)) {
            throwConfigurationError(
                configuration->valueOrThrow(config::cFileSuffixes),
                "File Suffixes must contain dotted suffixes such as .hpp."_el);
        }
    }
    for (const auto &name : excludedNames) {
        if (el::Path{name}.name() != name || name == "."_el || name == ".."_el) {
            throwConfigurationError(
                configuration->valueOrThrow(config::cExcludedNames),
                "Excluded Names must contain filenames, not paths or patterns."_el);
        }
    }
}

void CleanupPolicy::readSourceTrees(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files) {
    for (const auto &entry : *configuration->valueOrThrow(config::cSources)) {
        const auto mode = entry->getTextOrThrow(config::cIncludeStyle);
        if (mode != config::cRelativeIncludeStyle && mode != config::cPublicIncludeStyle) {
            throwConfigurationError(
                entry->valueOrThrow(config::cIncludeStyle), "Include Style must be 'relative' or 'public'."_el);
        }
        auto includeRoots = el::PathList{};
        if (const auto roots = entry->value(config::cIncludeRoots)) {
            for (const auto &root : roots->toValueList()) {
                includeRoots.append(path(root, files, true));
            }
        }
        for (const auto &directory : directories(entry->valueOrThrow(config::cDirectories), files)) {
            auto tree = SourceTree{directory, mode == config::cPublicIncludeStyle, el::PathList{directory}};
            if (!tree.publicIncludes && tree.directory != library) {
                throwConfigurationError(
                    entry->valueOrThrow(config::cDirectories),
                    "The relative include tree must match Library Directory."_el);
            }
            for (const auto &root : includeRoots) {
                tree.includeRoots.append(root);
            }
            for (const auto &previous : trees) {
                if (tree.directory.isRelativeTo(previous.directory) ||
                    previous.directory.isRelativeTo(tree.directory)) {
                    throwConfigurationError(
                        entry->valueOrThrow(config::cDirectories),
                        "Source trees from different policies must not overlap."_el);
                }
            }
            trees.push_back(std::move(tree));
        }
    }
    if (trees.empty()) {
        throwConfigurationError(
            configuration->valueOrThrow(config::cSources), "At least one source tree must be configured."_el);
    }
    if (cache == mapping || cache == style || cache.isRelativeTo(library)) {
        throwConfigurationError(
            configuration->valueOrThrow(config::cFormatCacheFile),
            "Format Cache File must be distinct from configuration and library inputs."_el);
    }
    for (const auto &tree : trees) {
        if (cache.isRelativeTo(tree.directory)) {
            throwConfigurationError(
                configuration->valueOrThrow(config::cFormatCacheFile),
                "Format Cache File must be outside all source trees."_el);
        }
    }
}

void CleanupPolicy::readCmakeTrees(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files) {
    static const auto targetPattern = el::RegEx::compile(R"/([A-Za-z0-9_\-]+)/"_el);
    for (const auto &entry : *configuration->valueOrThrow(config::cCmakeTrees)) {
        const auto directory = path(entry->valueOrThrow(config::cDirectory), files, true);
        const auto target = entry->getTextOrThrow(config::cTarget);
        if (!targetPattern->fullMatch(target)) {
            throwConfigurationError(
                entry->valueOrThrow(config::cTarget), "CMake target must be a literal target name."_el);
        }
        for (const auto &[previous, previousTarget] : cmake) {
            if (previous == directory) {
                throwConfigurationError(
                    entry->valueOrThrow(config::cDirectory), "Duplicate CMake directory policy."_el);
            }
        }
        if (cache.isRelativeTo(directory)) {
            throwConfigurationError(
                configuration->valueOrThrow(config::cFormatCacheFile),
                "Format Cache File must be outside all CMake trees."_el);
        }
        cmake.emplace_back(directory, target);
    }
}

auto CleanupPolicy::cmakeTarget(const el::Path &directory) const -> el::String {
    auto result = el::String{};
    auto depth = el::ItemCount{};
    for (const auto &[root, target] : cmake) {
        if (directory.isRelativeTo(root) && root.elementCount() > depth) {
            result = target;
            depth = root.elementCount();
        }
    }
    return result;
}

auto CleanupPolicy::selects(const el::Path &path) const -> bool {
    return suffixes.contains(path.suffix()) && !excludedNames.contains(path.name());
}

void CleanupPolicy::throwConfigurationError(
    const el::conf::ValuePtr &value, const el::String &description, const std::exception_ptr &cause) {
    throw el::conf::ConfError{value, "Invalid Code Cleanup Configuration"_el, description, cause};
}

}
