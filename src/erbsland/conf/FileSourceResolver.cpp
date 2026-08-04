// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileSourceResolver.hpp"

#include "impl/char/CharClass.hpp"
#include "impl/constants/Defaults.hpp"
#include "impl/constants/Limits.hpp"

#include "../err/ParameterError.hpp"
#include "../path/PathError.hpp"
#include "../path/PathInfo.hpp"
#include "../path/PathWalker.hpp"
#include "../text/String.hpp"
#include "../text/StringCharReader.hpp"
#include "../text/StringFormat.hpp"

#include <cstddef>

namespace erbsland::conf {

using namespace text::literals;

void FileSourceResolver::enable(const Feature feature) {
    if (feature >= _featureCount) {
        throw err::ParameterError{"Invalid file source resolver feature."_el, "feature"_el};
    }
    _features.set(feature);
}

void FileSourceResolver::disable(const Feature feature) {
    if (feature >= _featureCount) {
        throw err::ParameterError{"Invalid file source resolver feature."_el, "feature"_el};
    }
    _features.reset(feature);
}

auto FileSourceResolver::isEnabled(const Feature feature) const -> bool {
    if (feature >= _featureCount) {
        throw err::ParameterError{"Invalid file source resolver feature."_el, "feature"_el};
    }
    return _features.test(feature);
}

auto FileSourceResolver::resolve(const SourceResolverContext &context) -> SourceListPtr {
    // An empty include text is not valid.
    if (context.includeText.isEmpty()) {
        throwError("The include path is empty."_el);
    }
    // It makes no sense for having more than 500 characters.
    if (context.includeText.characterLength().toSizeT() > 500) {
        throwError("The include path is too long."_el);
    }
    auto pathString = context.includeText;
    removeFileProtocol(pathString);
    normalizePathSeparators(pathString);
    auto [directory, filename] = splitDirectoryAndFilename(pathString);
    const auto filenamePattern = getFilenamePattern(filename);
    if (filenamePattern.hasWildcard && !isEnabled(FilenameWildcard)) {
        throwError("The filename wildcard '*' is not supported."_el);
    }
    bool isRecursive = false;
    std::tie(directory, isRecursive) = validateDirectoryWildcard(directory);
    if (isRecursive && !isEnabled(RecursiveWildcard)) {
        throwError("The recursive wildcard '**' is not supported."_el);
    }
    const auto directoryPath = buildDirectory(context.sourceIdentifier, directory);
    const auto paths = scanForPaths(directoryPath, isRecursive, filenamePattern);
    return createSourcesFromPaths(paths);
}

auto FileSourceResolver::FilenamePattern::matches(const path::Path &path) const noexcept -> bool {
    const auto filename = path.name();
    if (hasWildcard) {
        if (!prefix.isEmpty() && !filename.startsWith(prefix)) {
            return false;
        }
        if (!suffix.isEmpty() && !filename.endsWith(suffix)) {
            return false;
        }
        return true;
    }
    return filename == prefix;
}

void FileSourceResolver::removeFileProtocol(text::String &path) const {
    if (path.startsWith("file:"_el)) {
        if (!isEnabled(FileProtocol)) {
            throwError("File protocol prefix 'file:' is not supported."_el);
        }
        path = path.slice(text::StringSide::Back, unit::ByteIndex{5U});
    }
}

void FileSourceResolver::normalizePathSeparators(text::String &path) {
    static const auto backslashCharacters = text::CharSet{"\\"_el};
    path = path.replacedAll(backslashCharacters, U'/');
    auto searchStart = unit::ByteIndex::zero();
    if (isEnabled(WindowsUNCPath) && path.startsWith("//"_el)) {
        verifyUncPath(path);
        searchStart = unit::ByteIndex{2U};
    }
    while (true) {
        const auto separator = path.find("//"_el, searchStart);
        if (separator.isNoIndex()) {
            break;
        }
        path = path.removed(unit::ByteRange{separator, unit::ByteLength::one()});
        searchStart = separator.advanced(unit::ByteLength::one());
    }
    if (path.endsWith("/"_el)) {
        throwError("An include path must not end with a path separator."_el);
    }
}

void FileSourceResolver::verifyUncPath(const text::String &path) {
    static const auto slashCharacters = text::CharSet{"/"_el};
    // On Windows, accept a UNC path, starting with //server/...
    const auto slashPosAfterServerName = path.findFirstOf(slashCharacters, unit::ByteIndex{2U});
    if (slashPosAfterServerName.isNoIndex()) {
        throwError("A slash is required after the Windows UNC path server name."_el);
    }
    if (slashPosAfterServerName == unit::ByteIndex{2U}) {
        throwError("The UNC path has no server name. Found three consecutive slashes."_el);
    }
    const auto serverName = path.slice(unit::ByteRange{unit::ByteIndex{2U}, slashPosAfterServerName});
    auto reader = text::StringCharReader{serverName};
    for (auto c = reader.read(); c != text::Char::endOfData(); c = reader.read()) {
        if (c == impl::CharClass::InvalidWindowsServerName) {
            throwError("The server name in the Windows UNC path contains invalid characters."_el);
        }
    }
    if (unit::ByteIndex::end(path.length()) < slashPosAfterServerName.advanced(unit::ByteLength{2U})) {
        throwError("There is no filename after the server in the UNC path."_el);
    }
}

auto FileSourceResolver::splitDirectoryAndFilename(const text::String &path) noexcept
    -> std::tuple<text::String, text::String> {

    static const auto slashCharacters = text::CharSet{"/"_el};
    const auto lastSlashPos = path.findLastOf(slashCharacters);
    if (lastSlashPos.isNoIndex()) {
        return std::make_tuple(text::String{}, path);
    }
    return std::make_tuple(
        path.slice(unit::ByteRange{unit::ByteIndex::zero(), lastSlashPos}),
        path.slice(text::StringSide::Back, lastSlashPos.advanced(unit::ByteLength::one())));
}

auto FileSourceResolver::getFilenamePattern(const text::String &filename) -> FilenamePattern {
    static const auto asteriskCharacters = text::CharSet{"*"_el};
    if (!filename.find("***"_el).isNoIndex()) {
        throwError("The include path contains an unsupported wildcard pattern."_el);
    }
    if (!filename.find("**"_el).isNoIndex()) {
        throwError("An include path must not contain the recursive '**' wildcard in the filename."_el);
    }
    auto asteriskCount = std::size_t{};
    for (auto reader = text::StringCharReader{filename};;) {
        const auto character = reader.read();
        if (character == text::Char::endOfData()) {
            break;
        }
        if (character == U'*') {
            ++asteriskCount;
        }
    }
    if (asteriskCount > 1) {
        throwError("An include path must not contain more than one '*' wildcard in the filename."_el);
    }
    if (asteriskCount > 0) {
        const auto asteriskPos = filename.findFirstOf(asteriskCharacters);
        return FilenamePattern{
            .prefix = filename.slice(unit::ByteRange{unit::ByteIndex::zero(), asteriskPos}),
            .suffix = filename.slice(text::StringSide::Back, asteriskPos.advanced(unit::ByteLength::one())),
            .hasWildcard = true};
    }
    return FilenamePattern{.prefix = filename, .suffix = {}, .hasWildcard = false};
}

auto FileSourceResolver::validateDirectoryWildcard(const text::String &directory) -> std::tuple<text::String, bool> {
    static const auto asteriskCharacters = text::CharSet{"*"_el};
    static const auto slashCharacters = text::CharSet{"/"_el};
    if (!directory.find("***"_el).isNoIndex()) {
        throwError("The include path contains an unsupported wildcard pattern."_el);
    }
    const auto recursiveWildcardPos = directory.find("**"_el);
    const auto firstAsterisk = directory.findFirstOf(asteriskCharacters);
    if (recursiveWildcardPos.isNoIndex()) {
        if (!firstAsterisk.isNoIndex()) {
            throwError("An include path must not contain '*' wildcard in the directory."_el);
        }
        return std::make_tuple(directory, false);
    }
    if (firstAsterisk != recursiveWildcardPos) {
        throwError("An include path must not contain '*' wildcard in the directory."_el);
    }
    const auto afterWildcard = recursiveWildcardPos.advanced(unit::ByteLength{2U});
    if (!directory.find("**"_el, afterWildcard).isNoIndex()) {
        throwError("An include path must not contain more than one '**' wildcard in the directory."_el);
    }
    if (!directory.findFirstOf(slashCharacters, recursiveWildcardPos.advanced(unit::ByteLength::one())).isNoIndex()) {
        throwError("The recursive wildcard '**' must not be the last directory element in the path."_el);
    }
    if (!directory.findFirstNotOf(slashCharacters, afterWildcard).isNoIndex()) {
        throwError("The recursive wildcard '**' must be an individual path element."_el);
    }
    if (!recursiveWildcardPos.isZero() && directory[recursiveWildcardPos.retreated(unit::ByteLength::one())] != U'/') {
        throwError("The recursive wildcard '**' must be an individual path element."_el);
    }
    if (recursiveWildcardPos.isZero()) {
        return std::make_tuple(text::String{}, true);
    }
    return std::make_tuple(
        directory.slice(
            unit::ByteRange{unit::ByteIndex::zero(), recursiveWildcardPos.retreated(unit::ByteLength::one())}),
        true);
}

auto FileSourceResolver::getBaseDirectory(const SourceIdentifierPtr &sourceIdentifier) -> path::Path {
    if (sourceIdentifier == nullptr) {
        throw err::LogicError{"sourceIdentifier must not be null"};
    }
    const auto withErrorPrefix = [](const text::String &message) -> text::String {
        text::StringEditor result{"Cannot determine the base directory the including document. "_el};
        result.append(message);
        return result;
    };
    if (sourceIdentifier->name() != text::String{impl::defaults::fileSourceIdentifier}) {
        throwError(withErrorPrefix("The document is not a file source."_el));
    }
    auto result = path::Path{sourceIdentifier->path()};
    if (!result.isAbsolute()) {
        throwError(withErrorPrefix("The path of the document is not absolute."_el), result);
    }
    try {
        result = result.resolveOrThrow(path::PathResolveMode::Physical);
    } catch (const path::PathError &) {
        throwError(
            withErrorPrefix("The path of the document cannot be canonicalized."_el), result, std::current_exception());
    }
    auto baseDirectory = result.parent();
    if (baseDirectory.isEmpty()) {
        throwError(withErrorPrefix("Could not determine the directory of the document."_el), result);
    }
    try {
        auto directoryInfo = baseDirectory.info();
        directoryInfo.reload(path::PathInfoPart::Type);
        if (!directoryInfo.isDirectory()) {
            throwError(withErrorPrefix("The parent path of the document is not a directory."_el), baseDirectory);
        }
    } catch (const path::PathError &) {
        throwError(
            withErrorPrefix("The parent path of the document cannot be inspected."_el),
            baseDirectory,
            std::current_exception());
    }
    return baseDirectory;
}

auto FileSourceResolver::buildDirectory(
    const SourceIdentifierPtr &sourceIdentifier, const text::String &directory) const -> path::Path {

    path::Path result;
    if (directory.isEmpty()) {
        result = getBaseDirectory(sourceIdentifier);
    } else {
        result = path::Path{directory};
        if (!result.isAbsolute()) {
            result = getBaseDirectory(sourceIdentifier) / result;
        } else if (!isEnabled(AbsolutePaths)) {
            throwError("Absolute include paths are not allowed."_el);
        }
    }
    try {
        result = result.resolveOrThrow(path::PathResolveMode::Physical);
        auto directoryInfo = result.info();
        directoryInfo.reload(path::PathInfoPart::Type);
        if (!directoryInfo.isDirectory()) {
            throwError("The base of an include path is not a directory."_el, result);
        }
    } catch (const path::PathError &) {
        throwError(
            "Could not canonicalize the base directory of an include path."_el, result, std::current_exception());
    }
    return result;
}

auto FileSourceResolver::scanForPaths(
    const path::Path &directory, const bool isRecursive, const FilenamePattern &filenamePattern) -> path::PathList {

    auto paths = path::PathList{};
    if (!isRecursive && !filenamePattern.hasWildcard) {
        paths.append(directory / text::String{filenamePattern.prefix});
        return paths;
    }
    try {
        auto options = path::PathWalkOptions{}
                           .setTypes(path::PathTypes{path::PathType::Directory, path::PathType::RegularFile})
                           .setInfoParts(path::PathInfoPart::Type);
        directory.walker().walkOrThrow(
            [&](const path::Path &candidate, const path::PathInfo &info) {
                if (info.isDirectory()) {
                    return !isRecursive && candidate != directory ? path::PathWalkStatus::Skip
                                                                  : path::PathWalkStatus::Continue;
                }
                if (filenamePattern.matches(candidate)) {
                    if (paths.count().toSizeT() >= impl::limits::maxIncludeSources) {
                        throw ConfError(
                            ConfErrorCategory::LimitExceeded,
                            text::StringFormat{"This include directive includes more than {} documents."_el}.build(
                                impl::limits::maxIncludeSources));
                    }
                    paths.append(candidate);
                }
                return path::PathWalkStatus::Continue;
            },
            options);
    } catch (const path::PathError &) {
        throwError("Scanning the include directory failed."_el, directory, std::current_exception());
    }
    return paths;
}

auto FileSourceResolver::createSourcesFromPaths(const path::PathList &paths) -> SourceListPtr {
    auto result = std::make_shared<SourceList>();
    for (auto sourcePath : paths) {
        try {
            sourcePath = sourcePath.resolveOrThrow(path::PathResolveMode::Physical);
            auto sourceInfo = sourcePath.info();
            sourceInfo.reload(path::PathInfoPart::Type);
            if (!sourceInfo.isRegularFile()) {
                throwError("The path of an included file is not a regular file."_el, sourcePath);
            }
        } catch (const path::PathError &) {
            throwError("Could not find the path of an included file."_el, sourcePath, std::current_exception());
        }
        result->push_back(Source::fromFile(sourcePath));
    }
    std::ranges::stable_sort(result->begin(), result->end(), sortLess);
    return result;
}

auto FileSourceResolver::sortLess(const SourcePtr &a, const SourcePtr &b) noexcept -> bool {
    const auto pathA = splitPath(a->path());
    const auto pathB = splitPath(b->path());
    auto itA = pathA.begin();
    auto itB = pathB.begin();
    for (; itA != pathA.end() && itB != pathB.end(); ++itA, ++itB) {
        const auto aIsDir = itA->endsWith(text::String{"/"_el}) || itA->endsWith(text::String{"\\"_el});
        const auto bIsDir = itB->endsWith(text::String{"/"_el}) || itB->endsWith(text::String{"\\"_el});
        if (aIsDir == bIsDir) {
            if (*itA == *itB) {
                continue; // check the next element.
            }
            return *itA < *itB;
        }
        return !aIsDir;
    }
    if (itA == pathA.end() && itB == pathB.end()) {
        return false;          // equal
    }
    return itA == pathA.end(); // the shorter path = less.
}

auto FileSourceResolver::splitPath(const text::String &path) noexcept -> text::StringList {
    static const auto separatorCharacters = text::CharSet{"/\\"_el};
    auto result = text::StringList{};
    auto position = unit::ByteIndex::zero();
    auto lastPosition = unit::ByteIndex::zero();
    const auto pathEnd = unit::ByteIndex::end(path.length());
    while (position < pathEnd) {
        position = path.findFirstOf(separatorCharacters, position);
        if (position.isNoIndex()) {
            result.append(path.slice(unit::ByteRange{lastPosition, pathEnd}));
            break;
        }
        position.advance(unit::ByteLength::one());
        result.append(path.slice(unit::ByteRange{lastPosition, position}));
        lastPosition = position;
    }
    return result;
}

void FileSourceResolver::throwError(text::String message, std::optional<path::Path> path, std::exception_ptr cause) {

    auto context = ConfErrorContext{ConfErrorCategory::Syntax, std::move(message)};
    if (path.has_value()) {
        context.setFilePath(std::move(path).value());
    }
    throw ConfError{std::move(context), std::move(cause)};
}

}
