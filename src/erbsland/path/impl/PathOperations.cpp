// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathOperations.hpp"

#include "BackendFactory.hpp"
#include "PathBackend.hpp"

#include "../PathContent.hpp"
#include "../PathError.hpp"
#include "../PathInfo.hpp"
#include "../PathWalker.hpp"

#include "../../stream/ByteOutputStream.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::path::impl {

using namespace text::literals;
using unit::ItemCount;

void PathOperations::removeOrThrow(const PathRemoveOptions options, const PathProgressFn &progressFn) {
    if (_path.isRoot()) {
        throw PathError{
            PathErrorContext{"Root path could not be removed"_el, "Removing a filesystem root is not permitted."_el}
                .setSourcePath(_path.toString())};
    }
    const auto info = _path.info();
    if (!info.exists()) {
        throw PathError{
            PathErrorContext{"Path could not be removed"_el, "The source path does not exist."_el}.setSourcePath(
                _path.toString())};
    }
    if (info.isDirectory() && !options.recursive()) {
        pathBackend().removeEntryOrThrow(_path);
        reportProgress(progressFn, PathProgressStatus::Success, ItemCount::one(), ItemCount::one(), {});
        return;
    }

    const auto total = options.prescan() ? countForProgress(SymlinkMode::Use) : ItemCount::infinite();
    auto processed = ItemCount{};
    auto errors = ItemCount{};
    auto walkOptions = PathWalkOptions{};
    walkOptions.setDirection(PathWalkDirection::LeafToRoot);
    walkOptions.setSymlinkMode(SymlinkMode::Use);
    walkOptions.setTypes(PathType::All);
    const auto result = _path.walker().walkOrThrow(
        [&](const Path &entry) -> PathWalkStatus {
            if (options.keepBase() && entry == _path) {
                return PathWalkStatus::Continue;
            }
            try {
                pathBackend().removeEntryOrThrow(entry);
                ++processed;
                reportProgress(progressFn, PathProgressStatus::Success, total, processed, errors);
            } catch (const PathError &) {
                ++processed;
                ++errors;
                reportProgress(progressFn, PathProgressStatus::Failed, total, processed, errors);
                if (!options.ignoreErrors()) {
                    throw;
                }
            }
            return PathWalkStatus::Continue;
        },
        walkOptions);
    if (result.isFailure() || !errors.isZero()) {
        throw PathError{
            PathErrorContext{"Path could not be removed completely"_el, "One or more paths could not be removed."_el}
                .setSourcePath(_path.toString())};
    }
}

void PathOperations::copyToOrThrow(
    const Path &destination, const PathCopyOptions options, const PathProgressFn &progressFn) const {
    if (destination.isEmpty()) {
        throw PathError{PathErrorContext{"Path could not be copied"_el, "The destination path is empty."_el}};
    }
    const auto sourceInfo = _path.info();
    if (!sourceInfo.exists()) {
        throw PathError{
            PathErrorContext{"Path could not be copied"_el, "The source path does not exist."_el}.setSourcePath(
                _path.toString())};
    }
    if (sourceInfo.isDirectory() && !options.recursive()) {
        throw PathError{
            PathErrorContext{"Directory could not be copied"_el, "Recursive copying is disabled for the directory."_el}
                .setSourcePath(_path.toString())};
    }
    const auto absoluteSource = _path.toAbsoluteOrThrow();
    const auto absoluteDestination = destination.toAbsoluteOrThrow();
    if (absoluteSource == absoluteDestination ||
        (sourceInfo.isDirectory() &&
            (absoluteDestination.isRelativeTo(absoluteSource) || absoluteSource.isRelativeTo(absoluteDestination)))) {
        throw PathError{
            PathErrorContext{"Path could not be copied"_el, "Source and destination trees must not overlap."_el}
                .setSourcePath(_path.toString())
                .setTargetPath(destination.toString())};
    }
    if (destination.info().exists()) {
        if (options.collisionMode() == PathCollisionMode::Skip) {
            return;
        }
        if (options.collisionMode() == PathCollisionMode::Stop) {
            throw PathError{PathErrorContext{"Path could not be copied"_el, "The destination path already exists."_el}
                    .setSourcePath(_path.toString())
                    .setTargetPath(destination.toString())};
        }
        removeExistingOrThrow(destination);
    }
    if (options.createParents()) {
        createParentsOrThrow(destination);
    }

    const auto total = options.prescan() ? countForProgress(options.symlinkMode()) : ItemCount::infinite();
    auto processed = ItemCount{};
    auto errors = ItemCount{};
    auto walkOptions = PathWalkOptions{};
    walkOptions.setSymlinkMode(options.symlinkMode());
    walkOptions.setTypes(PathType::All);
    const auto result = _path.walker().walkOrThrow(
        [&](const Path &entry, const PathInfo &info) -> PathWalkStatus {
            const auto absoluteEntry = entry.toAbsoluteOrThrow();
            const auto relative = absoluteEntry.toRelativeOrThrow(absoluteSource);
            const auto target = relative == Path::currentElement() ? destination : destination / relative;
            try {
                if (info.isDirectory()) {
                    pathBackend().createDirectoryEntryOrThrow(target, PathAccessProfile::Default);
                } else if (info.isSymlink() && options.symlinkMode() == SymlinkMode::Use) {
                    const auto linkTarget = pathBackend().readSymlinkOrThrow(entry);
                    const auto targetInfo = (linkTarget.isAbsolute() ? linkTarget : entry.parent() / linkTarget).info();
                    pathBackend().createSymlinkOrThrow(linkTarget, target, targetInfo.isDirectory());
                } else if (info.isRegularFile()) {
                    createParentsOrThrow(target);
                    pathBackend().copyFileEntryOrThrow(entry, target);
                } else {
                    throw PathError{PathErrorContext{
                        "Path could not be copied"_el, "The path type is not supported for copying."_el}
                            .setSourcePath(entry.toString())};
                }
                ++processed;
                reportProgress(progressFn, PathProgressStatus::Success, total, processed, errors);
            } catch (const PathError &) {
                ++processed;
                ++errors;
                reportProgress(progressFn, PathProgressStatus::Failed, total, processed, errors);
                if (!options.ignoreErrors()) {
                    throw;
                }
            }
            return PathWalkStatus::Continue;
        },
        walkOptions);
    if (result.isFailure() || !errors.isZero()) {
        throw PathError{
            PathErrorContext{"Path could not be copied completely"_el, "One or more paths could not be copied."_el}
                .setSourcePath(_path.toString())
                .setTargetPath(destination.toString())};
    }
}

void PathOperations::moveToOrThrow(const Path &destination, const PathMoveOptions options) const {
    if (destination.isEmpty()) {
        throw PathError{PathErrorContext{"Path could not be moved"_el, "The destination path is empty."_el}};
    }
    const auto sourceInfo = _path.info();
    if (!sourceInfo.exists()) {
        throw PathError{
            PathErrorContext{"Path could not be moved"_el, "The source path does not exist."_el}.setSourcePath(
                _path.toString())};
    }
    const auto absoluteSource = _path.toAbsoluteOrThrow();
    const auto absoluteDestination = destination.toAbsoluteOrThrow();
    if (absoluteSource == absoluteDestination) {
        return;
    }
    if (sourceInfo.isDirectory() &&
        (absoluteDestination.isRelativeTo(absoluteSource) || absoluteSource.isRelativeTo(absoluteDestination))) {
        throw PathError{
            PathErrorContext{"Path could not be moved"_el, "Source and destination trees must not overlap."_el}
                .setSourcePath(_path.toString())
                .setTargetPath(destination.toString())};
    }
    const auto destinationInfo = destination.info();
    if (destinationInfo.exists()) {
        if (options.collisionMode() == PathCollisionMode::Skip) {
            return;
        }
        if (options.collisionMode() == PathCollisionMode::Stop) {
            throw PathError{PathErrorContext{"Path could not be moved"_el, "The destination path already exists."_el}
                    .setSourcePath(_path.toString())
                    .setTargetPath(destination.toString())};
        }
        const auto overwriteRegularFile =
            options.collisionMode() == PathCollisionMode::Overwrite && sourceInfo.isRegularFile();
        if (overwriteRegularFile && !destinationInfo.isRegularFile()) {
            throw PathError{PathErrorContext{
                "File could not be moved"_el, "A regular file can only atomically replace another regular file."_el}
                    .setSourcePath(_path.toString())
                    .setTargetPath(destination.toString())};
        }
        const auto canReplaceRegularFile = overwriteRegularFile && destinationInfo.isRegularFile();
        if (!canReplaceRegularFile) {
            removeExistingOrThrow(destination);
        }
    }
    if (options.createParents()) {
        createParentsOrThrow(destination);
    }
    pathBackend().moveEntryOrThrow(_path, destination);
}

void PathOperations::createFileOrThrow(const PathCreateFileOptions options) const {
    auto writeOptions = PathWriteDataOptions{};
    writeOptions.setCreateParents(options.createParents());
    writeOptions.setCreationMode(options.creationMode());
    writeOptions.setAccessProfile(options.accessProfile());
    auto stream = _path.content().openByteOutputStream(writeOptions);
    stream->close();
}

void PathOperations::createDirectoryOrThrow(const PathCreateDirectoryOptions options) const {
    const auto existingInfo = _path.info();
    if (existingInfo.exists()) {
        if (!existingInfo.isDirectory() || options.creationMode() == PathCreateMode::CreateNew) {
            throw PathError{PathErrorContext{
                "Directory could not be created"_el, "A path already exists at the requested location."_el}
                    .setSourcePath(_path.toString())};
        }
        return;
    }
    if (options.createParents()) {
        createParentsOrThrow(_path);
    }
    pathBackend().createDirectoryEntryOrThrow(_path, options.accessProfile());
}

auto PathOperations::setAccessProfile(const PathAccessProfile profile, const PathChangeOptions options) const -> bool {
    return applyChange(
        options, [profile](const Path &path) -> void { pathBackend().setAccessProfileOrThrow(path, profile, {}); });
}

auto PathOperations::setLastModified(const time::DateTime &value, const PathChangeOptions options) const -> bool {
    return applyChange(options, [&value, &options](const Path &path) -> void {
        pathBackend().setLastModifiedOrThrow(path, value, options);
    });
}

auto PathOperations::addAttributes(const PathAttributes attributes, const PathChangeOptions options) const -> bool {
    return applyChange(
        options, [attributes](const Path &path) -> void { pathBackend().addAttributesOrThrow(path, attributes, {}); });
}

auto PathOperations::clearAttributes(const PathAttributes attributes, const PathChangeOptions options) const -> bool {
    return applyChange(options, [attributes](const Path &path) -> void {
        pathBackend().clearAttributesOrThrow(path, attributes, {});
    });
}

auto PathOperations::applyChange(
    const PathChangeOptions &options, const std::function<void(const Path &)> &changeFn) const -> bool {
    if (!options.recursive()) {
        try {
            changeFn(_path);
            return true;
        } catch (const PathError &) {
            if (!options.ignoreErrors()) {
                throw;
            }
            return false;
        }
    }

    auto hadErrors = false;
    auto walkOptions = PathWalkOptions{};
    walkOptions.setSymlinkMode(options.symlinkMode());
    walkOptions.setTypes(PathType::All);
    const auto result = _path.walker().walkOrThrow(
        [&](const Path &entry, const PathInfo &info) -> PathWalkStatus {
            try {
                const auto &changePath = options.symlinkMode() == SymlinkMode::Follow ? info.resolvedPath() : entry;
                changeFn(changePath);
            } catch (const PathError &) {
                if (!options.ignoreErrors()) {
                    throw;
                }
                hadErrors = true;
            }
            return PathWalkStatus::Continue;
        },
        walkOptions);
    return result.isSuccessful() && !hadErrors;
}

auto PathOperations::countForProgress(const SymlinkMode symlinkMode) const -> ItemCount {
    auto result = ItemCount{};
    auto options = PathWalkOptions{};
    options.setSymlinkMode(symlinkMode);
    options.setTypes(PathType::All);
    _path.walker().walkOrThrow(
        [&result](const Path &) -> PathWalkStatus {
            ++result;
            return PathWalkStatus::Continue;
        },
        options);
    return result;
}

void PathOperations::createParentsOrThrow(const Path &path) {
    const auto parent = path.parent();
    if (parent.isEmpty() || parent.info().exists()) {
        return;
    }
    createParentsOrThrow(parent);
    pathBackend().createDirectoryEntryOrThrow(parent, PathAccessProfile::Default);
}

void PathOperations::removeExistingOrThrow(const Path &path) {
    auto options = PathRemoveOptions{};
    options.setRecursive(true);
    PathOperations{path}.removeOrThrow(options, {});
}

void PathOperations::reportProgress(
    const PathProgressFn &progressFn,
    const PathProgressStatus status,
    const ItemCount total,
    const ItemCount processed,
    const ItemCount errors) {
    if (progressFn) {
        progressFn(PathProgress{status, total, processed, errors});
    }
}

}
