// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CommonPathBackend.hpp"

#include "../Path.hpp"
#include "../PathCreateMode.hpp"
#include "../PathError.hpp"

#include "../../stream/impl/EncodedTextOutputStream.hpp"
#include "../../stream/impl/InputStreamFactory.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../unit/ItemIndex.hpp"

namespace erbsland::path::impl {

using namespace text::literals;

auto CommonPathBackend::toAbsoluteOrThrow(const Path &path, std::optional<Path> base) const -> Path {
    if (path.isEmpty()) {
        throw PathError{PathErrorContext{
            "Path could not be made absolute"_el, "An empty path cannot be converted to an absolute path."_el}
                .setHelp("Provide a non-empty path."_el)};
    }
    if (base.has_value() && (base->isEmpty() || base->isRelative())) {
        throw PathError{
            PathErrorContext{"Path could not be made absolute"_el, "The base path must already be absolute."_el}
                .setSourcePath(base->toString())
                .setHelp("Provide an absolute base path."_el)};
    }
    if (path.isAbsolute()) {
        return path;
    }
    const auto effectiveBase = base.value_or(currentDirectoryOrThrow());
    return joinedLexicalPath(effectiveBase, path);
}

auto CommonPathBackend::toRelativeOrThrow(const Path &path, std::optional<Path> base) const -> Path {
    if (path.isEmpty()) {
        throw PathError{PathErrorContext{
            "Path could not be made relative"_el, "An empty path cannot be converted to a relative path."_el}
                .setHelp("Provide a non-empty path."_el)};
    }
    if (base.has_value() && (base->isEmpty() || base->isRelative())) {
        throw PathError{PathErrorContext{"Path could not be made relative"_el, "The base path must be absolute."_el}
                .setSourcePath(base->toString())
                .setHelp("Provide an absolute base path."_el)};
    }
    if (path.isRelative()) {
        return path;
    }

    const auto absolutePath = lexicalPath(path);
    const auto absoluteBase = lexicalPath(base.value_or(currentDirectoryOrThrow()));
    if (!haveSameRoot(absolutePath, absoluteBase)) {
        throw PathError{PathErrorContext{
            "Path could not be made relative"_el, "The path and base path do not share a common root."_el}
                .setSourcePath(absolutePath.toString())
                .setTargetPath(absoluteBase.toString())};
    }

    const auto pathElements = absolutePath.elements();
    const auto baseElements = absoluteBase.elements();

    auto commonIndex = unit::ItemIndex::one();
    while (
        commonIndex.isWithin(pathElements.count()) && commonIndex.isWithin(baseElements.count()) &&
        pathElements.get(commonIndex) == baseElements.get(commonIndex)) {
        ++commonIndex;
    }

    auto relativeElements = text::StringList{};
    for (auto index = commonIndex; index.isWithin(baseElements.count()); ++index) {
        relativeElements.append(".."_el);
    }
    for (auto index = commonIndex; index.isWithin(pathElements.count()); ++index) {
        relativeElements.append(pathElements.get(index));
    }
    if (relativeElements.isEmpty()) {
        return Path{"."_el};
    }
    return assemblePath({}, relativeElements);
}

auto CommonPathBackend::isRelativeTo(const Path &path, std::optional<Path> base) const noexcept -> bool {
    if (path.isRelative()) {
        return true;
    }
    try {
        if (path.isEmpty() || (base.has_value() && (base->isEmpty() || base->isRelative()))) {
            return false;
        }
        const auto absolutePath = lexicalPath(path);
        const auto absoluteBase = lexicalPath(base.value_or(currentDirectoryOrThrow()));
        if (!haveSameRoot(absolutePath, absoluteBase)) {
            return false;
        }

        const auto pathElements = absolutePath.elements();
        const auto baseElements = absoluteBase.elements();
        if (baseElements.count() > pathElements.count()) {
            return false;
        }
        for (auto index = unit::ItemIndex::zero(); index.isWithin(baseElements.count()); ++index) {
            if (pathElements.get(index) != baseElements.get(index)) {
                return false;
            }
        }
        return true;
    } catch (const PathError &) {
        return false;
    }
}

auto CommonPathBackend::commonAncestor(const Path &path, std::optional<Path> base) const noexcept -> Path {
    try {
        if (path.isEmpty() || (base.has_value() && (base->isEmpty() || base->isRelative()))) {
            return {};
        }
        const auto absolutePath = lexicalPath(path.isAbsolute() ? path : toAbsoluteOrThrow(path, std::nullopt));
        const auto absoluteBase = lexicalPath(base.value_or(currentDirectoryOrThrow()));
        if (!haveSameRoot(absolutePath, absoluteBase)) {
            return {};
        }

        const auto pathElements = absolutePath.elements();
        const auto baseElements = absoluteBase.elements();

        auto commonElements = text::StringList{};
        commonElements.append(pathElements.first());
        auto index = unit::ItemIndex::one();
        while (
            index.isWithin(pathElements.count()) && index.isWithin(baseElements.count()) &&
            pathElements.get(index) == baseElements.get(index)) {
            commonElements.append(pathElements.get(index));
            ++index;
        }
        auto ancestorElements = commonElements;
        ancestorElements.removeFirst();
        return assemblePath(commonElements.first(), ancestorElements);
    } catch (const PathError &) {
        return {};
    }
}

auto CommonPathBackend::openTextInputStreamOrThrow(const Path &path, const PathReadTextOptions options) const
    -> stream::TextInputStreamPtr {
    return stream::impl::createEncodedTextInputStream(
        openByteInputStreamOrThrow(path, PathReadDataOptions{}.setStreamSettings(options.streamSettings())),
        options.encoding(),
        options.bomMode(),
        options.encodingMode());
}

auto CommonPathBackend::openByteOutputStreamOrThrow(const Path &path, const PathWriteDataOptions options) const
    -> stream::ByteOutputStreamPtr {
    auto result = openByteOutputStreamWithExistingContentOrThrow(path, options).stream;
    invalidateInfo(path);
    return result;
}

auto CommonPathBackend::openTextOutputStreamOrThrow(const Path &path, const PathWriteTextOptions options) const
    -> stream::TextOutputStreamPtr {
    auto dataOptions = PathWriteDataOptions{};
    dataOptions.setCreateParents(options.createParents());
    dataOptions.setCreationMode(options.creationMode());
    dataOptions.setAccessProfile(options.accessProfile());
    dataOptions.setStreamSettings(options.streamSettings());
    auto openResult = openByteOutputStreamWithExistingContentOrThrow(path, dataOptions);
    invalidateInfo(path);
    const auto initialBomAlreadyHandled =
        options.creationMode() == PathCreateMode::CreateOrAppend && openResult.hasExistingContent;
    return std::make_shared<stream::impl::EncodedTextOutputStream>(
        std::move(openResult.stream), options.encoding(), options.bomMode(), initialBomAlreadyHandled);
}

auto CommonPathBackend::absoluteLexicalPathOrThrow(const Path &path) const -> Path {
    return lexicalPath(toAbsoluteOrThrow(path, std::nullopt));
}

auto CommonPathBackend::lexicalPath(const Path &path) -> Path {
    if (path.isEmpty()) {
        return {};
    }

    const auto isAbsolute = path.isAbsolute();
    auto normalizedElements = text::StringList{};
    for (const auto &element : path.elements()) {
        if (isAbsolute && element == path.root()) {
            continue;
        }
        if (element == "."_el) {
            continue;
        }
        if (element == ".."_el) {
            if (!normalizedElements.isEmpty() && normalizedElements.last() != ".."_el) {
                normalizedElements.removeLast();
            } else if (!isAbsolute) {
                normalizedElements.append(element);
            }
            continue;
        }
        normalizedElements.append(element);
    }

    if (!isAbsolute && normalizedElements.isEmpty()) {
        return Path{"."_el};
    }
    return assemblePath(path.root(), normalizedElements);
}

auto CommonPathBackend::joinedLexicalPath(const Path &base, const Path &suffix) -> Path {
    return lexicalPath(base / suffix);
}

auto CommonPathBackend::haveSameRoot(const Path &left, const Path &right) noexcept -> bool {
    return left.isAbsolute() && right.isAbsolute() && left.root() == right.root();
}

auto CommonPathBackend::assemblePath(const text::String &root, const text::StringList &elements) -> Path {
    auto result = text::StringEditor{};
    if (!root.isEmpty()) {
        result.append(root);
    }
    auto needsSeparator = !root.isEmpty() && !root.endsWith("/"_el);
    for (const auto &element : elements) {
        if (needsSeparator) {
            result.append("/"_el);
        }
        result.append(element);
        needsSeparator = true;
    }
    if (result.isEmpty()) {
        result.append("."_el);
    }
    return Path{result};
}

}
