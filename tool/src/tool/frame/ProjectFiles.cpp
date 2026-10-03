// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFiles.hpp"

#include "ProjectAccessCheck.hpp"
#include "ProjectRoot.hpp"
#include "TextChecksum.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/FileSourceResolver.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionValue.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathCreateDirectoryOptions.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PathReplaceTextOptions.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSide.hpp>
#include <erbsland/StringSplitter.hpp>

#include <memory>
#include <utility>

namespace tool::frame {

auto ProjectFiles::textChecksum(const el::Path &path, const el::HashAlgorithm algorithm) const -> el::ByteBlock {
    return calculateTextChecksum(readData(path), algorithm);
}

using namespace el::text::literals;

ProjectFiles::ProjectFiles(el::Path projectRoot) : _projectRoot{std::move(projectRoot)} {
}

void ProjectFiles::createDirectories(const el::Path &directory) const {
    try {
        checked(directory, true)
            .operations()
            .createDirectoryOrThrow(
                el::PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(
                    el::PathCreateMode::CreateOrAppend));
    } catch (const el::PathError &) {
        throw el::ApplicationError{
            "Failed to Create Project Directories"_el, directory.toString(), std::current_exception()};
    }
}

auto ProjectFiles::collectFilenamesOnly(const el::Path &target, const el::String &suffix, const bool recursive) const
    -> el::PathList {
    try {
        const auto root = checked(target);
        if (root.info().isRegularFile() && root.suffix() != suffix) {
            throwFileError(root, "Unexpected input suffix."_el);
        }
        if (!root.info().isRegularFile() && !root.info().isDirectory()) {
            throwFileError(root, "Input file or directory does not exist."_el);
        }
        auto result = el::PathList{};
        root.walker().walkOrThrow(
            [&](const el::Path &path) -> el::PathWalkStatus {
                result.append(path);
                return el::PathWalkStatus::Continue;
            },
            el::PathWalkOptions{}
                .setTypes(el::PathType::RegularFile)
                .setSymlinkMode(el::SymlinkMode::Skip)
                .setFilter([&](const el::Path &path, el::PathType) -> bool { return path.suffix() == suffix; })
                .setDescendFilter(
                    [&](const el::Path &path, el::PathType) -> bool { return recursive || path == root; }));
        result.sort();
        return result;
    } catch (const el::PathError &) {
        throw el::ApplicationError{
            "Failed to Collect Project Filenames"_el, target.toString(), std::current_exception()};
    }
}

auto ProjectFiles::resolve(const el::Path &path) const -> el::Path {
    if (path.isEmpty()) {
        throwFileError(path, "An empty project path is not allowed."_el);
    }
    const auto absolute = path.toAbsoluteOrThrow(_projectRoot).resolveOrThrow(el::PathResolveMode::Lexical);
    if (!absolute.isRelativeTo(_projectRoot)) {
        throwFileError(absolute, "The requested path leaves the validated project root."_el);
    }
    return absolute;
}

auto ProjectFiles::checked(const el::conf::ValuePtr &value, const bool allowMissingParents) const -> el::Path {
    try {
        return checked(el::Path{value->asTextOrThrow()}, allowMissingParents);
    } catch (const el::ApplicationError &error) {
        throw el::conf::ConfError{value, "Invalid Configured Path"_el, error.description(), std::current_exception()};
    } catch (const el::PathError &) {
        throw el::conf::ConfError{
            value,
            "Invalid Configured Path"_el,
            "The configured path cannot be interpreted."_el,
            std::current_exception()};
    }
}

auto ProjectFiles::checked(const el::OptionValuePtr &value, const bool allowMissingParents) const -> el::Path {
    try {
        return checked(el::Path{value->getText()}, allowMissingParents);
    } catch (const el::ApplicationError &error) {
        throw el::OptionError{"Invalid Path Option"_el, error.description(), value, std::current_exception()};
    } catch (const el::PathError &) {
        throw el::OptionError{
            "Invalid Path Option"_el, "The option path cannot be interpreted."_el, value, std::current_exception()};
    }
}

auto ProjectFiles::checked(const el::Path &path, const bool allowMissingParents) const -> el::Path {
    try {
        const auto absolute = resolve(path);
        auto componentPath = _projectRoot;
        const auto elements = absolute.toRelativeOrThrow(_projectRoot).elements();
        for (const auto &element : elements) {
            if (element == "."_el) {
                continue;
            }
            if (!componentPath.info().isDirectory()) {
                throwFileError(componentPath, "A parent of the requested path is not an accessible directory."_el);
            }
            componentPath.joinNative(element);
            // PathInfo does not follow the final link. Validate each parent before visiting its child.
            const auto info = componentPath.info();
            if (allowMissingParents && !info.exists()) {
                return absolute;
            }
            if (info.isSymlink() || info.isReparsePoint()) {
                throwFileError(componentPath, "Symbolic links and reparse points are not allowed in project paths."_el);
            }
        }
        return componentPath;
    } catch (const el::PathError &) {
        // Wrap path errors into application errors.
        throw el::ApplicationError{
            "A Project Path Check Failed"_el,
            "An error occurred while checking a project path."_el,
            std::current_exception()};
    }
}

auto ProjectFiles::checkedType(const el::Path &path, const el::PathType type, const bool allowMissing) const
    -> el::Path {
    const auto result = checked(path);
    const auto info = result.info();
    if (info.type() != type && (info.exists() || !allowMissing)) {
        if (type == el::PathType::Directory) {
            throwFileError(result, "Expected a directory at the selected path."_el);
        }
        throwFileError(result, "Expected a regular file at the selected path."_el);
    }
    return result;
}

auto ProjectFiles::checked(const el::conf::ValuePtr &value, const el::PathType type, const bool allowMissing) const
    -> el::Path {
    try {
        return checkedType(el::Path{value->asTextOrThrow()}, type, allowMissing);
    } catch (const el::ApplicationError &error) {
        throw el::conf::ConfError{value, "Invalid Configured Path"_el, error.description(), std::current_exception()};
    } catch (const el::PathError &) {
        throw el::conf::ConfError{
            value,
            "Invalid Configured Path"_el,
            "The configured path cannot be interpreted."_el,
            std::current_exception()};
    }
}

auto ProjectFiles::checked(const el::OptionValuePtr &value, const el::PathType type, const bool allowMissing) const
    -> el::Path {
    try {
        return checkedType(el::Path{value->getText()}, type, allowMissing);
    } catch (const el::ApplicationError &error) {
        throw el::OptionError{"Invalid Path Option"_el, error.description(), value, std::current_exception()};
    } catch (const el::PathError &) {
        throw el::OptionError{
            "Invalid Path Option"_el, "The option path cannot be interpreted."_el, value, std::current_exception()};
    }
}

void ProjectFiles::requireRegularFile(const el::Path &path) const {
    if (!path.info().isRegularFile()) {
        throwFileError(path, "The requested path is not a regular project file."_el);
    }
}

auto ProjectFiles::readText(const el::Path &path) const -> el::String {
    try {
        const auto file = resolve(path);
        return readCheckedText(file);
    } catch (const el::PathError &) {
        // Wrap path errors into application errors.
        throw el::ApplicationError{
            "Failed to Read a Text File from the Project"_el,
            "An error occurred trying to read the contents of a project file."_el,
            std::current_exception()};
    }
}

auto ProjectFiles::readTextFile(const el::Path &path) const -> ProjectTextFile {
    return readTextFile(path, el::SymlinkMode::Skip);
}

auto ProjectFiles::readWalkedTextFile(const el::Path &path) const -> ProjectTextFile {
    return readTextFile(path, el::SymlinkMode::Follow);
}

auto ProjectFiles::readTextFile(const el::Path &path, const el::SymlinkMode symlinkMode) const -> ProjectTextFile {
    try {
        auto result = ProjectTextFile{resolve(path)};
        requireRegularFile(result.path());
        result._text = result.path().content().readTextOrThrow(
            el::PathReadTextOptions{}.setEncodingMode(el::EncodingMode::Tolerant).setSymlinkMode(symlinkMode));
        return result;
    } catch (const el::PathError &) {
        throw el::ApplicationError{
            "Failed to Read a Text File from the Project"_el,
            "An error occurred trying to read the contents of a project file."_el,
            std::current_exception()};
    }
}

auto ProjectFiles::isUnchanged(const ProjectTextFile &file, const el::String &text) const -> bool {
    return file.path().isRelativeTo(_projectRoot) && equivalentText(file.text(), text) && file.isCurrent();
}

auto ProjectFiles::readCheckedText(const el::Path &path) const -> el::String {
    requireRegularFile(path);
    return path.content().readTextOrThrow(
        el::PathReadTextOptions{}.setEncodingMode(el::EncodingMode::Tolerant).setSymlinkMode(el::SymlinkMode::Skip));
}

auto ProjectFiles::readData(const el::Path &path) const -> el::ByteBlock {
    try {
        const auto file = resolve(path);
        requireRegularFile(file);
        return file.content().readDataOrThrow(el::PathReadDataOptions{}.setSymlinkMode(el::SymlinkMode::Skip));
    } catch (const el::PathError &) {
        // Wrap path errors into application errors.
        throw el::ApplicationError{
            "Failed to Read a Data File from the Project"_el,
            "An error occurred before the file could be successfully updated."_el,
            std::current_exception()};
    }
}

auto ProjectFiles::readConfiguration(
    const el::Path &path, const bool includes, const std::optional<el::StringMap<el::String>> &variables) const
    -> el::conf::DocumentPtr {
    try {
        const auto file = checked(path);
        requireRegularFile(file);
        auto parser = el::conf::Parser{};
        if (variables) {
            parser.setPlaceholderVariableSource(*variables);
            parser.addPlaceholderEnvironmentSource();
            parser.addPlaceholderTextFilters();
        }
        parser.setAccessCheck(std::make_shared<ProjectAccessCheck>(_projectRoot));
        if (includes) {
            auto resolver = el::conf::FileSourceResolver::create();
            resolver->disable(el::conf::FileSourceResolver::RecursiveWildcard);
            parser.setSourceResolver(resolver);
        } else {
            parser.setSourceResolver(nullptr);
        }
        return parser.parseFileOrThrow(file); // don't wrap these exceptions, they are already user facing.
    } catch (const el::PathError &) {
        // Wrap path errors into application errors.
        throw el::ApplicationError{
            "Failed to Read a Project Configuration"_el,
            "An error occurred before the file could be successfully updated."_el,
            std::current_exception()};
    }
}

auto ProjectFiles::updateText(
    const el::Path &path, const el::String &text, const bool checkOnly, const el::StringBomMode bomMode) const -> bool {
    try {
        const auto file = checked(path);
        if (file.info().exists()) {
            requireRegularFile(file);
            if (equivalentText(readCheckedText(file), text)) {
                return false;
            }
        }
        if (checkOnly) {
            return true;
        }
        const auto parent = checked(file.parent());
        if (!parent.info().isDirectory()) {
            throwFileError(file, "The output directory does not exist."_el);
        }
        file.content().replaceTextOrThrow(
            text,
            el::PathReplaceTextOptions{}.setBomMode(bomMode).setBeforeCommit(
                [this](const el::Path &source, const el::Path &destination) -> void {
                    checked(source);
                    checked(destination);
                }));
        return true;
    } catch (const el::RuntimeError &) {
        // Wrap path errors into application errors.
        throw el::ApplicationError{
            "Failed to Update Project File"_el,
            "An error occurred before the file could be successfully updated."_el,
            std::current_exception()};
    }
}

auto ProjectFiles::equivalentText(const el::String &left, const el::String &right) -> bool {
    if (left == right) {
        return true;
    }
    if (left.isEmpty() || right.isEmpty()) {
        return false;
    }
    auto oldLines = el::StringSplitter{left, U'\n', el::StringSplitMode::KeepSeparator};
    auto newLines = el::StringSplitter{right, U'\n', el::StringSplitMode::KeepSeparator};
    const auto terminators = el::CharSet{U'\r', U'\n'};
    auto firstLine = true;
    while (!oldLines.isAtEnd() && !newLines.isAtEnd()) {
        auto oldLine = oldLines.next().trimmed(terminators, el::StringSide::Back);
        auto newLine = newLines.next().trimmed(terminators, el::StringSide::Back);
        if (firstLine) {
            oldLine = normalizedCopyright(oldLine);
            newLine = normalizedCopyright(newLine);
            firstLine = false;
        }
        if (oldLine != newLine) {
            return false;
        }
    }
    return oldLines.isAtEnd() && newLines.isAtEnd();
}

auto ProjectFiles::normalizedCopyright(const el::String &line) -> el::String {
    static const auto pattern = el::re::RegEx::compile(
        R"/((?i)(?<prefix>[ \t]*(?://|#)[ \t]*Copyright (?:\(c\)|©) ?)[0-9]{4}(?:-[0-9]{4})?(?<suffix>[^0-9].*)?)/"_el);
    if (const auto match = pattern->fullMatch(line)) {
        return el::join(match->content("prefix"_el), "????"_el, match->content("suffix"_el));
    }
    return line;
}

void ProjectFiles::throwFileError(const el::Path &path, el::String description) const {
    auto sourcePath = path.toString();
    if (path.isEmpty()) {
        sourcePath = _projectRoot.toString();
    }
    throw el::ApplicationError{"Failed to Access Project File"_el, std::move(description)}.withSourcePath(
        std::move(sourcePath));
}

}
