// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PackageFiles.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/PathCopyOptions.hpp>
#include <erbsland/PathCreateDirectoryOptions.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/PathWalkOptions.hpp>
#include <erbsland/PathWalkStatus.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/U8StringConstIterator.hpp>

namespace erbsland::package {

using namespace path;
using namespace text;
using namespace text::literals;

PackageFiles::PackageFiles(Path baseDirectory, Path packageRoot) :
    _baseDirectory{std::move(baseDirectory)}, _packageRoot{std::move(packageRoot)} {
}

void PackageFiles::validateRelative(const Path &path, const bool allowEmpty) {
    if ((!allowEmpty && !path.isValid()) || (path.isValid() && !path.isRelative())) {
        throw core::ApplicationError{"Package paths must be relative."_el};
    }
    for (const auto &element : path.elements()) {
        if (element == ".."_el || element == "."_el) {
            throw core::ApplicationError{"Package paths cannot contain '.' or '..'."_el};
        }
    }
}

void PackageFiles::copyFile(const Path &source, const Path &relativeDestination) const {
    validateRelative(relativeDestination);
    if (!source.info().isRegularFile()) {
        throw core::ApplicationError{
            StringFormat{"Package source is not a regular file: {}"_el}.build(source.toString())};
    }
    const auto destination = _packageRoot / relativeDestination;
    if (destination.info().exists()) {
        throw core::ApplicationError{StringFormat{"Package file collision: {}"_el}.build(destination.toString())};
    }
    destination.parent().operations().createDirectoryOrThrow(
        PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(PathCreateMode::CreateOrOverwrite));
    source.operations().copyToOrThrow(destination);
}

void PackageFiles::copyDirectory(const Path &source, const Path &relativeDestination) const {
    validateRelative(relativeDestination);
    if (!source.info().isDirectory()) {
        throw core::ApplicationError{"Package directory is missing."_el};
    }
    const auto destination = _packageRoot / relativeDestination;
    if (destination.info().exists()) {
        throw core::ApplicationError{"Package directory collision."_el};
    }
    destination.parent().operations().createDirectoryOrThrow(
        PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(PathCreateMode::CreateOrOverwrite));
    source.operations().copyToOrThrow(
        destination, PathCopyOptions{}.setRecursive(true).setSymlinkMode(SymlinkMode::Use));
}

auto PackageFiles::matches(const StringList &patterns, const String &relative) -> bool {
    for (const auto &pattern : patterns) {
        auto regex = StringEditor{"^"_el};
        auto pendingStar = false;
        for (const auto character : pattern) {
            if (character == U'*') {
                if (pendingStar) {
                    regex.append(".*"_el);
                    pendingStar = false;
                } else {
                    pendingStar = true;
                }
                continue;
            }
            if (pendingStar) {
                regex.append("[^/]*"_el);
                pendingStar = false;
            }
            if (character == U'?') {
                regex.append("[^/]"_el);
            } else {
                if (character == U'.' || character == U'[' || character == U']' || character == U'(' ||
                    character == U')' || character == U'+' || character == U'{' || character == U'}' ||
                    character == U'^' || character == U'$' || character == U'|' || character == U'\\') {
                    regex.append("\\"_el);
                }
                regex.append(character);
            }
        }
        if (pendingStar) {
            regex.append("[^/]*"_el);
        }
        regex.append("$"_el);
        if (re::RegEx::compile(String{regex})->fullMatch(relative)) {
            return true;
        }
    }
    return false;
}

auto PackageFiles::matchesRegex(const StringList &patterns, const String &relative) -> bool {
    for (const auto &pattern : patterns) {
        if (re::RegEx::compile(pattern)->fullMatch(relative)) {
            return true;
        }
    }
    return false;
}

void PackageFiles::add(const FileEntry &entry) const {
    if (entry.path.isRelative()) {
        validateRelative(entry.path);
    }
    validateRelative(entry.target, true);
    const auto source = entry.path.isAbsolute() ? entry.path : _baseDirectory / entry.path;
    const auto info = source.info();
    if (info.isSymlink()) {
        throw core::ApplicationError{"Additional-file sources cannot be symlinks."_el};
    }
    if (info.isRegularFile()) {
        if (!entry.includes.isEmpty() || !entry.excludes.isEmpty() || !entry.includeRegex.isEmpty() ||
            !entry.excludeRegex.isEmpty()) {
            throw core::ApplicationError{"File patterns apply to directories only."_el};
        }
        copyFile(source, entry.target.isValid() ? entry.target / source.name() : Path{source.name()});
        return;
    }
    if (!info.isDirectory()) {
        throw core::ApplicationError{"Additional-file source is missing."_el};
    }
    const auto options = PathWalkOptions{}.setSymlinkMode(SymlinkMode::Skip);
    source.walker().walkOrThrow(
        [&](const Path &item, const PathInfo &itemInfo) -> PathWalkStatus {
            if (item == source) {
                return PathWalkStatus::Continue;
            }
            const auto relative = item.toRelativeOrThrow(source);
            if (itemInfo.isDirectory()) {
                return entry.recursive ? PathWalkStatus::Continue : PathWalkStatus::Skip;
            }
            if (!itemInfo.isRegularFile()) {
                return PathWalkStatus::Continue;
            }
            const auto text = relative.toPosix();
            if ((!entry.includes.isEmpty() || !entry.includeRegex.isEmpty()) && !matches(entry.includes, text) &&
                !matchesRegex(entry.includeRegex, text)) {
                return PathWalkStatus::Continue;
            }
            if (matches(entry.excludes, text) || matchesRegex(entry.excludeRegex, text)) {
                return PathWalkStatus::Continue;
            }
            copyFile(item, entry.target.isValid() ? entry.target / relative : relative);
            return PathWalkStatus::Continue;
        },
        options);
}

}
