// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PackageFiles.hpp"

#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/path/PathCopyOptions.hpp>
#include <erbsland/path/PathCreateDirectoryOptions.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/PathWalkOptions.hpp>
#include <erbsland/path/PathWalkStatus.hpp>
#include <erbsland/path/PathWalker.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/u8/U8StringConstIterator.hpp>

namespace erbsland::package {

using namespace text::literals;

PackageFiles::PackageFiles(path::Path baseDirectory, path::Path packageRoot) :
    _baseDirectory{std::move(baseDirectory)}, _packageRoot{std::move(packageRoot)} {}

void PackageFiles::validateRelative(const path::Path &path, const bool allowEmpty) {
    if ((!allowEmpty && !path.isValid()) || (path.isValid() && !path.isRelative())) {
        throw core::ApplicationError{"Package paths must be relative."_el};
    }
    for (const auto &element : path.elements()) {
        if (element == ".."_el || element == "."_el) {
            throw core::ApplicationError{"Package paths cannot contain '.' or '..'."_el};
        }
    }
}

void PackageFiles::copyFile(const path::Path &source, const path::Path &relativeDestination) const {
    validateRelative(relativeDestination);
    if (!source.info().isRegularFile()) {
        throw core::ApplicationError{text::StringFormat{"Package source is not a regular file: {}"_el}
                                         .build(source.toString())};
    }
    const auto destination = _packageRoot / relativeDestination;
    if (destination.info().exists()) {
        throw core::ApplicationError{text::StringFormat{"Package file collision: {}"_el}.build(destination.toString())};
    }
    destination.parent().operations().createDirectoryOrThrow(
        path::PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(path::PathCreateMode::CreateOrOverwrite));
    source.operations().copyToOrThrow(destination);
}

void PackageFiles::copyDirectory(const path::Path &source, const path::Path &relativeDestination) const {
    validateRelative(relativeDestination);
    if (!source.info().isDirectory()) { throw core::ApplicationError{"Package directory is missing."_el}; }
    const auto destination = _packageRoot / relativeDestination;
    if (destination.info().exists()) { throw core::ApplicationError{"Package directory collision."_el}; }
    destination.parent().operations().createDirectoryOrThrow(
        path::PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(path::PathCreateMode::CreateOrOverwrite));
    source.operations().copyToOrThrow(
        destination, path::PathCopyOptions{}.setRecursive(true).setSymlinkMode(path::SymlinkMode::Use));
}

auto PackageFiles::matches(const text::StringList &patterns, const text::String &relative) -> bool {
    for (const auto &pattern : patterns) {
        auto regex = text::StringEditor{"^"_el};
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
            if (character == U'?') { regex.append("[^/]"_el); }
            else {
                if (character == U'.' || character == U'[' || character == U']' || character == U'(' ||
                    character == U')' || character == U'+' || character == U'{' || character == U'}' ||
                    character == U'^' || character == U'$' || character == U'|' || character == U'\\') {
                    regex.append("\\"_el);
                }
                regex.append(character);
            }
        }
        if (pendingStar) { regex.append("[^/]*"_el); }
        regex.append("$"_el);
        if (re::RegEx::compile(text::String{regex})->fullMatch(relative)) { return true; }
    }
    return false;
}

auto PackageFiles::matchesRegex(const text::StringList &patterns, const text::String &relative) -> bool {
    for (const auto &pattern : patterns) {
        if (re::RegEx::compile(pattern)->fullMatch(relative)) { return true; }
    }
    return false;
}

void PackageFiles::add(const FileEntry &entry) const {
    if (entry.path.isRelative()) { validateRelative(entry.path); }
    validateRelative(entry.target, true);
    const auto source = entry.path.isAbsolute() ? entry.path : _baseDirectory / entry.path;
    const auto info = source.info();
    if (info.isSymlink()) { throw core::ApplicationError{"Additional-file sources cannot be symlinks."_el}; }
    if (info.isRegularFile()) {
        if (!entry.includes.isEmpty() || !entry.excludes.isEmpty() ||
            !entry.includeRegex.isEmpty() || !entry.excludeRegex.isEmpty()) {
            throw core::ApplicationError{"File patterns apply to directories only."_el};
        }
        copyFile(source, entry.target.isValid() ? entry.target / source.name() : path::Path{source.name()});
        return;
    }
    if (!info.isDirectory()) { throw core::ApplicationError{"Additional-file source is missing."_el}; }
    const auto options = path::PathWalkOptions{}.setSymlinkMode(path::SymlinkMode::Skip);
    source.walker().walkOrThrow([&](const path::Path &item, const path::PathInfo &itemInfo) -> path::PathWalkStatus {
        if (item == source) { return path::PathWalkStatus::Continue; }
        const auto relative = item.toRelativeOrThrow(source);
        if (itemInfo.isDirectory()) {
            return entry.recursive ? path::PathWalkStatus::Continue : path::PathWalkStatus::Skip;
        }
        if (!itemInfo.isRegularFile()) { return path::PathWalkStatus::Continue; }
        const auto text = relative.toPosix();
        if ((!entry.includes.isEmpty() || !entry.includeRegex.isEmpty()) &&
            !matches(entry.includes, text) && !matchesRegex(entry.includeRegex, text)) {
            return path::PathWalkStatus::Continue;
        }
        if (matches(entry.excludes, text) || matchesRegex(entry.excludeRegex, text)) {
            return path::PathWalkStatus::Continue;
        }
        copyFile(item, entry.target.isValid() ? entry.target / relative : relative);
        return path::PathWalkStatus::Continue;
    }, options);
}

}
