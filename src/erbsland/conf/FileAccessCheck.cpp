// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileAccessCheck.hpp"

#include "impl/constants/Defaults.hpp"
#include "impl/constants/Limits.hpp"

#include "../err/ParameterError.hpp"
#include "../path/PathError.hpp"
#include "../path/PathInfo.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/String.hpp"

namespace erbsland::conf {

using namespace text::literals;

void FileAccessCheck::enable(const Feature feature) {
    if (feature >= _featureCount) {
        throw err::ParameterError{"Invalid file access feature.", "feature"};
    }
    _features.set(feature);
}

void FileAccessCheck::disable(const Feature feature) {
    if (feature >= _featureCount) {
        throw err::ParameterError{"Invalid file access feature.", "feature"};
    }
    _features.reset(feature);
}

auto FileAccessCheck::isEnabled(const Feature feature) const -> bool {
    if (feature >= _featureCount) {
        throw err::ParameterError{"Invalid file access feature.", "feature"};
    }
    return _features.test(feature);
}

auto FileAccessCheck::check(const AccessSources &sources) -> AccessCheckResult {
    if (sources.source == nullptr || sources.root == nullptr) {
        throwAccessError("No document or root source given."_el);
    }
    if (sources.source->name() != text::String{impl::defaults::fileSourceIdentifier}) {
        if (isEnabled(OnlyFileSources)) {
            throwAccessError("Only file sources are permitted."_el);
        }
        return AccessCheckResult::Granted; // Grant access to all other sources.
    }
    fileAccessCheck(sources);
    return AccessCheckResult::Granted;
}

void FileAccessCheck::fileAccessCheck(const AccessSources &sources) const {
    // Sanity checks.
    if (sources.source->name() != text::String{impl::defaults::fileSourceIdentifier}) {
        throw err::LogicError("This function only checks file sources.");
    }
    if (!(isEnabled(AnyDirectory) || isEnabled(SameDirectory) || isEnabled(Subdirectories))) {
        throwAccessError("No directory access policies are configured. All file access is currently blocked."_el);
    }

    // The next part checks the relationship between the included and including document.
    if (sources.parent == nullptr) {
        return; // This is the root document, grant access to it.
    }
    auto sourcePath = extractSourcePath(sources);
    auto parentDirectory = extractParentDirectory(sources);
    canonicalizePaths(sourcePath, parentDirectory);
    if (isEnabled(LimitSize)) {
        try {
            auto sourceInfo = sourcePath.info();
            sourceInfo.reload(path::PathInfoParts{path::PathInfoPart::Type, path::PathInfoPart::Size});
            if (sourceInfo.fileSize().toSizeT() > limits::maxDocumentSize) {
                throwAccessError("The included file exceeds the maximum allowed size of 100MB."_el, sourcePath);
            }
        } catch (const path::PathError &) {
            throwAccessError(
                "The size of the included file could not be read."_el, sourcePath, std::current_exception());
        }
    }
    if (!isEnabled(AnyDirectory)) {
        const bool isInSame = requireSourceInParentDirectory(sourcePath, parentDirectory);
        if (!isEnabled(SameDirectory) && isInSame) {
            throwAccessError(
                "Including files from the same directory as the parent file is not permitted by policy."_el,
                sourcePath);
        }
        if (!isEnabled(Subdirectories) && !isInSame) {
            throwAccessError("Including files from subdirectories is not permitted by policy."_el, sourcePath);
        }
    }
}

void FileAccessCheck::throwAccessError(text::String message, path::Path path, std::exception_ptr cause) {
    auto context = ConfErrorContext{ConfErrorCategory::Access, std::move(message)};
    if (!path.isEmpty()) {
        context.setFilePath(std::move(path));
    }
    throw ConfError{std::move(context), std::move(cause)};
}

auto FileAccessCheck::extractSourcePath(const AccessSources &sources) const -> path::Path {
    auto sourcePath = path::Path{sources.source->path()};
    if (isEnabled(RequireSuffix)) {
        if (sourcePath.suffix().compare(
                text::String{impl::defaults::fileSuffix}, text::cCaseInsensitive.asciiComparisonFn()) !=
            std::strong_ordering::equal) {
            throwAccessError("The included file does not have the suffix \".elcl\"."_el, sourcePath);
        }
    }
    return sourcePath;
}

auto FileAccessCheck::extractParentDirectory(const AccessSources &sources) const -> path::Path {
    path::Path parentDirectory;
    if (sources.parent == nullptr || sources.parent->name() != text::String{impl::defaults::fileSourceIdentifier}) {
        if (!isEnabled(AnyDirectory)) {
            throwAccessError("Cannot verify the parent path because the including document it is not a local file."_el);
        }
    } else {
        parentDirectory = path::Path{sources.parent->path()};
        if (parentDirectory.parent().isEmpty()) {
            throwAccessError("Could not determine the parent directory of the including file."_el);
        }
        parentDirectory = parentDirectory.parent();
    }
    return parentDirectory;
}

void FileAccessCheck::canonicalizePaths(path::Path &sourcePath, path::Path &parentDirectory) {
    try {
        sourcePath = sourcePath.resolveOrThrow(path::PathResolveMode::Physical);
    } catch (const path::PathError &) {
        throwAccessError(
            "Failed to resolve the canonical path of the included file."_el, sourcePath, std::current_exception());
    }
    try {
        parentDirectory = parentDirectory.resolveOrThrow(path::PathResolveMode::Physical);
    } catch (const path::PathError &) {
        throwAccessError(
            "Failed to resolve the canonical path of the parent file's directory."_el,
            parentDirectory,
            std::current_exception());
    }
}

auto FileAccessCheck::requireSourceInParentDirectory(const path::Path &sourcePath, const path::Path &parentDirectory)
    -> bool {

    if (sourcePath.parent().isEmpty()) {
        throwAccessError("Could not determine the parent directory of the including file."_el, sourcePath);
    }
    const auto sourceElements = sourcePath.parent().elements();
    const auto parentElements = parentDirectory.elements();
    auto parentIt = parentElements.begin();
    auto sourceIt = sourceElements.begin();
    while (parentIt != parentElements.end()) {
        if (sourceIt == sourceElements.end() || *parentIt != *sourceIt) {
            throwAccessError(
                "The included file is outside the allowed directory range of the parent file."_el, sourcePath);
        }
        ++parentIt;
        ++sourceIt;
    }
    return sourceIt == sourceElements.end();
}

}
