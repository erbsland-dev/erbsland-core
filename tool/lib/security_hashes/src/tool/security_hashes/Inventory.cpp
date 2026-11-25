// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Inventory.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathType.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/PathWalkOptions.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/SymlinkMode.hpp>

namespace tool::security_hashes {

using namespace el::text::literals;

Inventory::Inventory(const Settings &settings, const frame::ProjectFiles &files) : _settings{settings}, _files{files} {
}

auto Inventory::collect() const -> el::List<Input> {
    auto selected = el::StringMap<Input>{};
    const auto add = [&](const el::Path &path, const bool directoryRecord, const bool fileRecord) -> void {
        const auto relative = path.toRelativeOrThrow(_files.projectRoot()).toPosix();
        if (!Settings::isManifestPath(relative)) {
            throw el::ApplicationError{
                "Invalid Source Filename"_el, "Source paths must not contain backslashes, NULs or line breaks."_el}
                .withSourcePath(path.toString());
        }
        auto input = selected.get(relative, Input{path, relative});
        input.directoryRecord = input.directoryRecord || directoryRecord;
        input.fileRecord = input.fileRecord || fileRecord;
        selected.set(relative, input);
    };
    auto walked = el::PathList{};
    auto roots = _settings.directories;
    roots.sort(
        [](const el::Path &left, const el::Path &right) -> bool { return left.elementCount() < right.elementCount(); });
    for (const auto &root : roots) {
        if (_settings.excludesDirectory(root)) {
            continue;
        }
        auto covered = false;
        for (const auto &previous : walked) {
            if (root.isRelativeTo(previous)) {
                covered = true;
                break;
            }
        }
        if (covered) {
            continue;
        }
        walked.append(root);
        try {
            _files.checked(root).walker().walkOrThrow(
                [&](const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus {
                    if (info.isDirectory()) {
                        if (_settings.excludesDirectory(path)) {
                            return el::PathWalkStatus::Skip;
                        }
                    } else if (info.isSymlink() || info.isReparsePoint()) {
                        throw el::ApplicationError{
                            "Unsafe Security Hash Input"_el,
                            "Symbolic links and reparse points are not allowed in source trees."_el}
                            .withSourcePath(path.toString());
                    } else if (_settings.selects(path)) {
                        if (!info.isRegularFile()) {
                            throw el::ApplicationError{
                                "Invalid Security Hash Input"_el, "Selected sources must be regular files."_el}
                                .withSourcePath(path.toString());
                        }
                        add(path, true, false);
                    }
                    return el::PathWalkStatus::Continue;
                },
                el::PathWalkOptions{}.setTypes(el::PathType::All).setSymlinkMode(el::SymlinkMode::Use));
        } catch (const el::PathError &) {
            throw el::ApplicationError{
                "Source Inventory Failed"_el,
                "The security hash source tree could not be scanned."_el,
                std::current_exception()}
                .withSourcePath(root.toString());
        }
    }
    for (const auto &path : _settings.individualFiles) {
        if (!_settings.excludesFile(path)) {
            add(path, false, true);
        }
    }
    auto result = el::List<Input>{};
    result.reserve(selected.count());
    for (const auto &[name, input] : selected) {
        result.append(input);
    }
    result.sort(
        [](const Input &left, const Input &right) -> bool { return left.relative.compareBinary(right.relative) < 0; });
    return result;
}

}
