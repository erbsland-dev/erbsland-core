// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathWalker.hpp"

#include "BackendFactory.hpp"
#include "PathBackend.hpp"

#include "../PathError.hpp"
#include "../PathResolveMode.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringSet.hpp"

#include <algorithm>

namespace erbsland::path::impl {

using namespace text::literals;

auto PathWalker::walkOrThrow(const PathWalkFn &walkFn, const PathWalkOptions options) const -> PathWalkResult {
    if (!walkFn) {
        throw PathError{
            PathErrorContext{"Path walk could not be started"_el, "No callback was provided for the path walk."_el}};
    }
    return walkImpl([&walkFn](const Path &path, const PathInfo &) -> PathWalkStatus { return walkFn(path); }, options);
}

auto PathWalker::walkOrThrow(const PathInfoWalkFn &walkFn, const PathWalkOptions options) const -> PathWalkResult {
    if (!walkFn) {
        throw PathError{
            PathErrorContext{"Path walk could not be started"_el, "No callback was provided for the path walk."_el}};
    }
    return walkImpl(walkFn, options);
}

auto PathWalker::walkImpl(const WalkFn &walkFn, const PathWalkOptions &options) const -> PathWalkResult {
    if (_path.isEmpty()) {
        throw PathError{
            PathErrorContext{"Path walk could not be started"_el, "No base path was provided for the path walk."_el}};
    }
    auto visitedDirectories = text::StringSet{};
    auto hadErrors = false;
    const auto cacheTrust = std::make_shared<PathInfoCacheTrust>();
    const auto result = visit(_path, walkFn, options, visitedDirectories, hadErrors, false, cacheTrust);
    if (result == VisitResult::Stop) {
        return PathWalkResult::Stopped;
    }
    if (result == VisitResult::Failure || hadErrors) {
        return PathWalkResult::Failure;
    }
    return PathWalkResult::Success;
}

auto PathWalker::visit(
    const Path &path,
    const WalkFn &walkFn,
    const PathWalkOptions &options,
    text::StringSet &visitedDirectories,
    bool &hadErrors,
    const bool trustCache,
    const PathInfoCacheTrustPtr &cacheTrust) const -> VisitResult {
    try {
        auto info = trustCache ? PathInfo::fromDirectoryScan(path, options.infoParts() | PathInfoPart::Type, cacheTrust)
                               : path.info(options.infoParts() | PathInfoPart::Type);
        if (!info.exists()) {
            throw PathError{
                PathErrorContext{"Path could not be walked"_el, "Information for the path is unavailable."_el}
                    .setSourcePath(path.toString())};
        }

        auto type = info.type();
        auto isDirectory = type == PathType::Directory;
        if (type == PathType::Symlink) {
            if (options.symlinkMode() == SymlinkMode::Skip) {
                return VisitResult::Continue;
            }
            if (options.symlinkMode() == SymlinkMode::Follow) {
                const auto resolvedPath = path.resolveOrThrow(PathResolveMode::Physical);
                info = resolvedPath.info(options.infoParts() | PathInfoPart::Type);
                if (!info.exists()) {
                    throw PathError{PathErrorContext{
                        "Symbolic link could not be followed"_el, "The symbolic-link target is unavailable."_el}
                            .setSourcePath(path.toString())};
                }
                type = info.type();
                isDirectory = type == PathType::Directory;
            }
        }

        if (isDirectory && options.symlinkMode() == SymlinkMode::Follow) {
            if (!visitedDirectories.tryInsert(info.resolvedPath().toString())) {
                return VisitResult::Continue;
            }
        }

        const auto shouldReport = options.types().isSet(type);
        if (options.direction() == PathWalkDirection::RootToLeaf && shouldReport) {
            const auto callbackStatus = callbackResult(walkFn(path, info));
            if (callbackStatus != VisitResult::Continue) {
                return callbackStatus;
            }
        }

        if (isDirectory) {
            auto entries = pathBackend().directoryEntriesOrThrow(path, info.resolvedPath());
            std::ranges::sort(
                entries, [](const Path &left, const Path &right) -> bool { return left.name() < right.name(); });
            for (const auto &entry : entries) {
                const auto childResult = visit(entry, walkFn, options, visitedDirectories, hadErrors, true, cacheTrust);
                if (childResult == VisitResult::Stop || childResult == VisitResult::Failure) {
                    return childResult;
                }
            }
        }

        if (options.direction() == PathWalkDirection::LeafToRoot && shouldReport) {
            return callbackResult(walkFn(path, info));
        }
        return VisitResult::Continue;
    } catch (const PathError &) {
        if (!options.ignoreErrors()) {
            throw;
        }
        hadErrors = true;
        return VisitResult::Continue;
    }
}

auto PathWalker::callbackResult(const PathWalkStatus status) noexcept -> VisitResult {
    switch (status) {
    case PathWalkStatus::Continue:
        return VisitResult::Continue;
    case PathWalkStatus::Skip:
        return VisitResult::Skip;
    case PathWalkStatus::Stop:
        return VisitResult::Stop;
    case PathWalkStatus::Failure:
        return VisitResult::Failure;
    }
    return VisitResult::Failure;
}

}
