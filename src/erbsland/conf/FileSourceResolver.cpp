// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileSourceResolver.hpp"

#include "impl/char/CharClass.hpp"
#include "impl/constants/Defaults.hpp"
#include "impl/constants/Limits.hpp"

#include "../err/ParameterFailure.hpp"
#include "../err/ParseError.hpp"
#include "../path/impl/PathGlobCompiler.hpp"
#include "../path/PathError.hpp"
#include "../path/PathInfo.hpp"
#include "../path/PathWalker.hpp"
#include "../text/String.hpp"
#include "../text/StringCharReader.hpp"
#include "../text/StringFormat.hpp"
#include "../unit/CpCount.hpp"

#include <cstddef>

namespace erbsland::conf {

using namespace err;
using namespace path;
using namespace text;
using namespace unit;
using namespace text::literals;

void FileSourceResolver::enable(const Feature feature) {
    if (feature >= _featureCount) {
        throw ParameterFailure{"Invalid file source resolver feature."_el, "feature"_el};
    }
    _features.set(feature);
}

void FileSourceResolver::disable(const Feature feature) {
    if (feature >= _featureCount) {
        throw ParameterFailure{"Invalid file source resolver feature."_el, "feature"_el};
    }
    _features.reset(feature);
}

auto FileSourceResolver::isEnabled(const Feature feature) const -> bool {
    if (feature >= _featureCount) {
        throw ParameterFailure{"Invalid file source resolver feature."_el, "feature"_el};
    }
    return _features.test(feature);
}

auto FileSourceResolver::resolve(const SourceResolverContext &context) -> SourceListPtr {
    // An empty include text is not valid.
    if (context.includeText.isEmpty()) {
        throwError("The include path is empty."_el);
    }
    // It makes no sense for having more than 500 characters.
    if (context.includeText.characterLength() > CpCount{500}) {
        throwError("The include path is too long."_el);
    }
    auto pathString = context.includeText;
    removeFileProtocol(pathString);
    auto flags = PathGlobPatternFlags{PathGlobPatternFlag::TrimWhitespace};
    if (isEnabled(RecursiveWildcard)) {
        flags |= PathGlobPatternFlag::RecursiveWildcard;
    }
    auto query = path::impl::PathGlobCompiler::WalkPattern{};
    try {
        query = path::impl::PathGlobCompiler{pathString, flags}.compileForWalk(isEnabled(Wildcards));
    } catch (const ParseError &) {
        throwError("The include path contains an invalid glob pattern."_el, {}, std::current_exception());
    }
    if (query.prefix.isAbsolute() && !isEnabled(AbsolutePaths)) {
        throwError("Absolute include paths are not allowed."_el);
    }
    if (query.prefix.root().startsWith("//"_el)) {
        if (!isEnabled(WindowsUNCPath)) {
            throwError("Windows UNC include paths are not allowed."_el);
        }
        verifyUncPath(query.prefix.toString());
    }
    if (query.pattern == nullptr) {
        const auto directory = buildDirectory(context.sourceIdentifier, query.prefix.parent());
        return createSourcesFromPaths(PathList{directory.joinedNative(query.prefix.name())});
    }
    const auto directory = buildDirectory(context.sourceIdentifier, query.prefix);
    return createSourcesFromPaths(scanForPaths(directory, query.pattern));
}

void FileSourceResolver::removeFileProtocol(String &path) const {
    if (path.startsWith("file:"_el)) {
        if (!isEnabled(FileProtocol)) {
            throwError("File protocol prefix 'file:' is not supported."_el);
        }
        path = path.slice(StringSide::Back, ByteIndex{5U});
    }
}

void FileSourceResolver::verifyUncPath(const String &path) {
    static const auto slashCharacters = CharSet{"/"_el};
    // On Windows, accept a UNC path, starting with //server/...
    const auto slashPosAfterServerName = path.findFirstOf(slashCharacters, ByteIndex{2U});
    if (slashPosAfterServerName.isNoIndex()) {
        throwError("A slash is required after the Windows UNC path server name."_el);
    }
    if (slashPosAfterServerName == ByteIndex{2U}) {
        throwError("The UNC path has no server name. Found three consecutive slashes."_el);
    }
    const auto serverName = path.slice(ByteRange{ByteIndex{2U}, slashPosAfterServerName});
    auto reader = StringCharReader{serverName};
    for (auto c = reader.read(); c != Char::endOfData(); c = reader.read()) {
        if (c == impl::CharClass::InvalidWindowsServerName) {
            throwError("The server name in the Windows UNC path contains invalid characters."_el);
        }
    }
    if (ByteIndex::end(path.length()) < slashPosAfterServerName.advanced(ByteCount{2U})) {
        throwError("There is no filename after the server in the UNC path."_el);
    }
}

auto FileSourceResolver::getBaseDirectory(const SourceIdentifierPtr &sourceIdentifier) -> Path {
    if (sourceIdentifier == nullptr) {
        throw LogicFailure{"sourceIdentifier must not be null"};
    }
    const auto withErrorPrefix = [](const String &message) -> String {
        StringEditor result{"Cannot determine the base directory the including document. "_el};
        result.append(message);
        return result;
    };
    if (sourceIdentifier->name() != String{impl::defaults::fileSourceIdentifier}) {
        throwError(withErrorPrefix("The document is not a file source."_el));
    }
    auto result = Path{sourceIdentifier->path()};
    if (!result.isAbsolute()) {
        throwError(withErrorPrefix("The path of the document is not absolute."_el), result);
    }
    try {
        result = result.resolveOrThrow(PathResolveMode::Physical);
    } catch (const PathError &) {
        throwError(
            withErrorPrefix("The path of the document cannot be canonicalized."_el), result, std::current_exception());
    }
    auto baseDirectory = result.parent();
    if (baseDirectory.isEmpty()) {
        throwError(withErrorPrefix("Could not determine the directory of the document."_el), result);
    }
    try {
        auto directoryInfo = baseDirectory.info();
        directoryInfo.reload(PathInfoPart::Type);
        if (!directoryInfo.isDirectory()) {
            throwError(withErrorPrefix("The parent path of the document is not a directory."_el), baseDirectory);
        }
    } catch (const PathError &) {
        throwError(
            withErrorPrefix("The parent path of the document cannot be inspected."_el),
            baseDirectory,
            std::current_exception());
    }
    return baseDirectory;
}

auto FileSourceResolver::buildDirectory(const SourceIdentifierPtr &sourceIdentifier, const Path &directory) const
    -> Path {

    Path result;
    if (directory.isEmpty()) {
        result = getBaseDirectory(sourceIdentifier);
    } else {
        result = directory;
        if (!result.isAbsolute()) {
            result = getBaseDirectory(sourceIdentifier) / result;
        } else if (!isEnabled(AbsolutePaths)) {
            throwError("Absolute include paths are not allowed."_el);
        }
    }
    try {
        result = result.resolveOrThrow(PathResolveMode::Physical);
        auto directoryInfo = result.info();
        directoryInfo.reload(PathInfoPart::Type);
        if (!directoryInfo.isDirectory()) {
            throwError("The base of an include path is not a directory."_el, result);
        }
    } catch (const PathError &) {
        throwError(
            "Could not canonicalize the base directory of an include path."_el, result, std::current_exception());
    }
    return result;
}

auto FileSourceResolver::scanForPaths(const Path &directory, const PathGlobPatternPtr &pattern) -> PathList {
    auto paths = PathList{};
    try {
        const auto options =
            PathWalkOptions{}.setGlobPattern(pattern).setTypes(PathType::RegularFile).setInfoParts(PathInfoPart::Type);
        directory.walker().walkOrThrow(
            [&](const Path &candidate) -> PathWalkStatus {
                if (paths.count().toSizeT() >= impl::limits::maxIncludeSources) {
                    throw ConfError{
                        ConfErrorCategory::LimitExceeded,
                        StringFormat{"This include directive includes more than {} documents."_el}.build(
                            impl::limits::maxIncludeSources)};
                }
                paths.append(candidate);
                return PathWalkStatus::Continue;
            },
            options);
    } catch (const PathError &) {
        throwError("Scanning the include directory failed."_el, directory, std::current_exception());
    }
    return paths;
}

auto FileSourceResolver::createSourcesFromPaths(const PathList &paths) -> SourceListPtr {
    auto resolvedPaths = PathList{};
    for (auto sourcePath : paths) {
        try {
            sourcePath = sourcePath.resolveOrThrow(PathResolveMode::Physical);
            auto sourceInfo = sourcePath.info();
            sourceInfo.reload(PathInfoPart::Type);
            if (!sourceInfo.isRegularFile()) {
                throwError("The path of an included file is not a regular file."_el, sourcePath);
            }
        } catch (const PathError &) {
            throwError("Could not find the path of an included file."_el, sourcePath, std::current_exception());
        }
        resolvedPaths.append(sourcePath);
    }
    resolvedPaths.sort(sortLess);
    auto result = std::make_shared<SourceList>();
    result->reserve(resolvedPaths.count().toSizeT());
    for (const auto &sourcePath : resolvedPaths) {
        result->push_back(Source::fromFile(sourcePath));
    }
    return result;
}

auto FileSourceResolver::sortLess(const Path &a, const Path &b) noexcept -> bool {
    for (
        auto index = ItemIndex::zero(); index.isWithin(a.elementCount()) && index.isWithin(b.elementCount()); ++index) {
        const auto aIsDirectory = index.incremented().isWithin(a.elementCount());
        const auto bIsDirectory = index.incremented().isWithin(b.elementCount());
        if (aIsDirectory != bIsDirectory) {
            return !aIsDirectory;
        }
        const auto left = a.element(index);
        const auto right = b.element(index);
        if (left != right) {
            return left < right;
        }
    }
    return a.elementCount() < b.elementCount();
}

void FileSourceResolver::throwError(String message, std::optional<Path> path, std::exception_ptr cause) {
    auto context = ConfErrorContext{ConfErrorCategory::Syntax, std::move(message)};
    if (path.has_value()) {
        context.setFilePath(std::move(path).value());
    }
    throw ConfError{std::move(context), std::move(cause)};
}

}
