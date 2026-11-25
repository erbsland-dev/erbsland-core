// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Settings.hpp"

#include "Configuration.hpp"

#include <erbsland/CharSet.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/PathInfo.hpp>

namespace tool::security_hashes {

using namespace el::text::literals;

auto Settings::rules() -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Security hashes"_el}, Description{"Portable integrity hashes for infrastructure source files."_el});
    rules.addRule(
        config::cOutputFile,
        RuleType::Text,
        Minimum{1},
        Default{"security_hashes.txt"_el},
        Title{"Manifest output"_el},
        Description{"Project-relative manifest file; its parent directory must exist."_el});
    rules.addRule(
        config::cDirectories,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Source directories"_el},
        Description{"Existing project-relative directories scanned recursively for selected source files."_el});
    rules.addRule(
        config::cFiles,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Individual files"_el},
        Description{
            "Existing project-relative files included regardless of filename or suffix selection; exclusions still apply."_el});
    rules.addRule(
        config::cIncludedFileNames,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Included filenames"_el},
        Description{"Exact case-sensitive filenames selected during recursive scanning."_el});
    rules.addRule(
        config::cIncludedFileSuffixes,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Included file suffixes"_el},
        Description{"Case-sensitive dotted source suffixes selected during recursive scanning."_el});
    rules.addRule(
        config::cExcludedDirectoryPaths,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded directory paths"_el},
        Description{"Project-relative directory subtrees to prune; missing excluded directories are allowed."_el});
    rules.addRule(
        config::cExcludedDirectoryNames,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded directory names"_el},
        Description{"Directory basenames pruned at any depth, including explicitly selected roots."_el});
    rules.addRule(
        config::cExcludedFileNames,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded filenames"_el},
        Description{"Exact case-sensitive filenames omitted from every selection."_el});
    rules.addRule(
        config::cExcludedFileSuffixes,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Excluded file suffixes"_el},
        Description{"Case-sensitive dotted suffixes omitted from every selection."_el});
    return rules.takeRules();
}

auto Settings::isManifestPath(const el::String &text) -> bool {
    static const auto cForbidden = el::CharSet{U'\0', U'\r', U'\n', U'\\'};
    return !text.isEmpty() && !text.containsOneOf(cForbidden);
}

auto Settings::readNames(const el::conf::ValuePtr &value, const bool suffixes) -> el::StringSet {
    auto result = el::StringSet{};
    if (!value) {
        return result;
    }
    for (const auto &entry : value->toValueList()) {
        const auto text = entry->asTextOrThrow();
        if (!isManifestPath(text) || text.contains("/"_el) || text == "."_el || text == ".."_el ||
            (suffixes && !text.startsWith("."_el))) {
            throw el::conf::ConfError{
                entry,
                "Invalid Source Selection"_el,
                "Use a local filename or a dotted suffix without path separators or line breaks."_el};
        }
        result.insert(text);
    }
    return result;
}

auto Settings::readPath(
    const el::conf::ValuePtr &value, const frame::ProjectFiles &files, const bool allowMissingParents) -> el::Path {
    const auto text = value->asTextOrThrow();
    const auto path = el::Path{text};
    if (!isManifestPath(text) || !path.isRelative() || path.elements().contains(".."_el)) {
        throw el::conf::ConfError{
            value,
            "Invalid Security Hash Path"_el,
            "Use a project-relative POSIX path without parent traversal or line breaks."_el};
    }
    return files.checked(value, allowMissingParents);
}

void Settings::read(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files) {
    _root = files.projectRoot();
    const auto outputValue = configuration->valueOrThrow(config::cOutputFile);
    output = readPath(outputValue, files);
    if (!output.parent().info().isDirectory() || (output.info().exists() && !output.info().isRegularFile())) {
        throw el::conf::ConfError{
            outputValue,
            "Invalid Manifest Destination"_el,
            "The manifest must be a regular file or a new file in an existing directory."_el};
    }
    directories.clear();
    individualFiles.clear();
    _excludedPaths.clear();
    if (const auto values = configuration->value(config::cDirectories)) {
        for (const auto &value : values->toValueList()) {
            const auto path = readPath(value, files);
            if (!path.info().isDirectory()) {
                throw el::conf::ConfError{value, "Invalid Source Directory"_el, "The source directory must exist."_el};
            }
            directories.append(path);
        }
    }
    if (const auto values = configuration->value(config::cFiles)) {
        for (const auto &value : values->toValueList()) {
            const auto path = readPath(value, files);
            if (!path.info().isRegularFile()) {
                throw el::conf::ConfError{
                    value, "Invalid Source File"_el, "The source must be an existing regular file."_el};
            }
            individualFiles.append(path);
        }
    }
    if (const auto values = configuration->value(config::cExcludedDirectoryPaths)) {
        for (const auto &value : values->toValueList()) {
            const auto path = readPath(value, files, true);
            if (path.info().exists() && !path.info().isDirectory()) {
                throw el::conf::ConfError{
                    value, "Invalid Excluded Directory"_el, "An excluded path must name a directory."_el};
            }
            _excludedPaths.append(path);
        }
    }
    _directoryNames = readNames(configuration->value(config::cExcludedDirectoryNames), false);
    _fileNames = readNames(configuration->value(config::cExcludedFileNames), false);
    _fileSuffixes = readNames(configuration->value(config::cExcludedFileSuffixes), true);
    _includedNames = readNames(configuration->value(config::cIncludedFileNames), false);
    _includedSuffixes = readNames(configuration->value(config::cIncludedFileSuffixes), true);
}

auto Settings::excludesDirectory(const el::Path &path) const -> bool {
    for (const auto &excluded : _excludedPaths) {
        if (path.isRelativeTo(excluded)) {
            return true;
        }
    }
    for (const auto &name : path.toRelativeOrThrow(_root).elements()) {
        if (_directoryNames.contains(name)) {
            return true;
        }
    }
    return false;
}

auto Settings::excludesFile(const el::Path &path) const -> bool {
    return path == output || excludesDirectory(path.parent()) || _fileNames.contains(path.name()) ||
        _fileSuffixes.contains(path.suffix());
}

auto Settings::selects(const el::Path &path) const -> bool {
    return !excludesFile(path) && (_includedNames.contains(path.name()) || _includedSuffixes.contains(path.suffix()));
}

}
