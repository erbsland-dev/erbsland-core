// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectRoot.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PlatformErrorContext.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringTransformations.hpp>
#include <erbsland/TextInputStream.hpp>

#include <utility>

namespace tool::frame {

using namespace el::text::literals;

auto ProjectRoot::hasMarker(const el::Path &directory) -> bool {
    const auto directoryInfo = directory.info();
    if (!directoryInfo.isDirectory() || !directoryInfo.isReadable()) {
        throwNotFound(
            "While scanning for the project root, we encountered a directory "
            "that is unreadable or a path that is not a directory."_el,
            {},
            directory.toString());
    }
    const auto candidate = directory / "CMakeLists.txt"_el;
    if (candidate.info().exists() && !candidate.info().isRegularFile()) {
        return false;
    }
    return isProjectFile(candidate);
}

auto ProjectRoot::isProjectFile(const el::Path &cmakeFile) -> bool {
    static const auto reProjectMarker =
        el::re::RegEx::compile(R"/((?im)^[ \t]*project\s*\(\s*([A-Za-z0-9_.\-]+)(?:\s|\)))/"_el);
    el::TextInputStreamPtr stream;
    try {
        const auto readOptions = el::PathReadTextOptions{}.setSymlinkMode(el::SymlinkMode::Skip);
        stream = cmakeFile.content().openTextInputStream(readOptions);
    } catch (const el::PathError &error) {
        if (error.platformContext() && error.platformContext()->category() == el::PlatformErrorCategory::NotFound) {
            return false;
        }
        throw;
    }
    // Read the first 1k text, if this is the right CMake file, we should find the marker.
    const auto result = stream->read(el::CpCount{1024});
    if (result.isFinished()) {
        return false; // empty file
    }
    if (!result.hasData()) {
        throwNotFound(
            "While we scanned a directory for a valid project root, "
            "we encountered an unexpected timeout reading this file."_el,
            {},
            cmakeFile.toString());
    }
    for (const auto &marker : reProjectMarker->findAll(result.data())) {
        if (el::toAsciiLowercase(marker->content(1)) == el::toAsciiLowercase(_project.cmakeName)) {
            return true;
        }
    }
    return false;
}

void ProjectRoot::throwNotFound(el::String description, const std::exception_ptr &cause, el::String sourcePath) {
    if (sourcePath.isEmpty()) {
        if (_resolvedProjectDirectory.isEmpty()) {
            sourcePath = _uncheckedProjectDirectory.toString();
        } else {
            sourcePath = _resolvedProjectDirectory.toString();
        }
    }
    throw el::ApplicationError{
        el::join("Could not find a valid "_el, _project.name, " project directory."_el), std::move(description), cause}
        .withSourcePath(std::move(sourcePath));
}

auto ProjectRoot::discover() -> el::Path {
    static constexpr auto errorHint =
        "Please call this tool from inside the repository, or supply a valid '--project' option."_el;
    try {
        _resolvedProjectDirectory = _uncheckedProjectDirectory.resolveOrThrow();
        auto directory = _resolvedProjectDirectory;
        while (!hasMarker(directory)) {
            const auto parent = directory.parent();
            if (parent == directory || parent.isEmpty()) {
                throwNotFound(
                    el::join(
                        "There was no matching CMake project in any of the scanned directories. "
                        "We scanned the current working directory and all accessible parent directories. "_el,
                        errorHint));
            }
            directory = parent;
        }
        return directory;
    } catch (const el::ApplicationError &) {
        throw;
    } catch (const el::RuntimeError &) {
        throwNotFound(
            el::join("There was an error while scanning for the project root. "_el, errorHint),
            std::current_exception());
    }
}

auto ProjectRoot::validate() -> el::Path {
    try {
        _resolvedProjectDirectory = _uncheckedProjectDirectory.resolveOrThrow();
        if (!hasMarker(_resolvedProjectDirectory)) {
            throwNotFound("There was no matching CMake project declaration at the specified location."_el);
        }
        return _resolvedProjectDirectory;
    } catch (const el::ApplicationError &) {
        throw;
    } catch (const el::RuntimeError &) {
        throwNotFound(
            "We could not validate the path supplied by the '--project' option because of a filesystem error."_el,
            std::current_exception());
    }
}

}
