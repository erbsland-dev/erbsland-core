// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PackageConfig.hpp"
#include "impl/PackagePlaceholderSource.hpp"

#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/NamePath.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/placeholder/Replacer.hpp>
#include <erbsland/text/placeholder/ReplacerOptions.hpp>

#include <memory>

namespace erbsland::package {

using namespace text::literals;

PackageConfig::PackageConfig(path::Path file) : _baseDirectory{file.parent()} {
    if (file.info().exists()) {
        auto parser = conf::Parser{};
        parser.addPlaceholderEnvironmentSource();
        parser.addPlaceholderTextFilters();
        _document = parser.parseFileOrThrow(file);
        if (!_document->getSectionWithNames(conf::NamePath::fromText("main"_el))) {
            throw core::ApplicationError{"Package configuration requires a [main] section."_el};
        }
    }
}

auto PackageConfig::settings(
    const text::String &name, const text::String &platform, const text::String &architecture,
    const text::String &target, const text::String &projectName,
    const text::String &projectVersion) const -> PackageSettings {
    auto result = PackageSettings{};
    result.name = name;
    result.projectName = projectName;
    result.version = projectVersion;
    if (_document) {
        applySection(result, "main"_el);
        applySection(result, text::StringFormat{"platform.{}"_el}.build(platform));
        applySection(result, text::StringFormat{"platform.{}.{}"_el}.build(platform, architecture));
        const auto packagePath = text::StringFormat{"package.{}"_el}.build(pathSegment(name));
        applySection(result, packagePath);
        applySection(result, text::StringFormat{"{}.platform.{}"_el}.build(packagePath, platform));
        applySection(result, text::StringFormat{"{}.platform.{}.{}"_el}.build(packagePath, platform, architecture));
        if (!target.isEmpty()) {
            const auto targetPath = text::StringFormat{"target.{}"_el}.build(pathSegment(target));
            applySection(result, targetPath);
            applySection(result, text::StringFormat{"{}.platform.{}"_el}.build(targetPath, platform));
            applySection(result, text::StringFormat{"{}.platform.{}.{}"_el}.build(targetPath, platform, architecture));
        }
    }
    if (result.versionSource == "cmake"_el) {
        if (result.version.isEmpty()) {
            throw core::ApplicationError{"Package version is missing. Set project(VERSION ...) or configure a version source."_el};
        }
    } else if (result.versionSource == "file"_el) {
        if (result.versionFile.isEmpty()) { throw core::ApplicationError{"version_file is required."_el}; }
        const auto versionPath = path::Path{result.versionFile};
        const auto source = versionPath.isAbsolute() ? versionPath : _baseDirectory / versionPath;
        if (!source.info().isRegularFile()) { throw core::ApplicationError{"Version file is missing."_el}; }
        const auto content = source.content().readTextOrThrow();
        if (!result.versionPattern.isEmpty()) {
            const auto match = re::RegEx::compile(result.versionPattern)->findFirst(content);
            if (!match) { throw core::ApplicationError{"Version pattern did not match."_el}; }
            result.version = match->content(re::CaptureGroupIndex{1U});
        } else {
            const auto lines = text::StringList::fromSplit(
                content, text::CharSet{"\n"_el}, unit::ItemCount::infinite(), true);
            result.version = {};
            for (const auto &line : lines) {
                const auto trimmed = line.trimmed();
                if (!trimmed.isEmpty() && !trimmed.startsWith("#"_el)) {
                    result.version = trimmed;
                    break;
                }
            }
        }
    } else if (result.versionSource == "git"_el) {
        // Git version selection is performed by the application to retain useful process diagnostics.
        result.version = {};
    } else {
        throw core::ApplicationError{"Unknown version_source; expected cmake, git, or file."_el};
    }
    if (result.version.isEmpty() && result.versionSource != "git"_el) {
        throw core::ApplicationError{"Package version is empty."_el};
    }
    if (result.targetDir.isEmpty()) { result.targetDir = "%{package:name}-%{version}"_el; }
    if (result.filenameFormat.isEmpty()) {
        result.filenameFormat = "%{package:name}-%{version}-%{sys:platform}-%{sys:architecture}"_el;
    }
    return result;
}

auto PackageConfig::pathSegment(const text::String &name) -> text::String {
    if (re::RegEx::compile("^[A-Za-z_][A-Za-z0-9_]*$"_el)->fullMatch(name)) { return name; }
    if (name.contains("\""_el) || name.contains("\\"_el)) {
        throw core::ApplicationError{"Package or target name cannot contain quotes or backslashes."_el};
    }
    return text::StringFormat{"\"{}\""_el}.build(name);
}

void PackageConfig::applySection(PackageSettings &result, const text::String &sectionPath) const {
    const auto section = _document->value(conf::NamePath::fromText(sectionPath));
    if (!section) { return; }
    if (!section->isSectionWithNames() && !section->isSectionWithTexts()) {
        throw core::ApplicationError{"Package configuration override must be a named section."_el};
    }
    const auto text = [&](const text::String &key, text::String &value) -> void {
        const auto path = conf::NamePath::fromText(key);
        if (section->hasValue(path)) { value = section->getTextOrThrow(path); }
    };
    text("version_source"_el, result.versionSource);
    text("version_file"_el, result.versionFile);
    text("version_pattern"_el, result.versionPattern);
    text("target_dir"_el, result.targetDir);
    text("filename_format"_el, result.filenameFormat);
    text("bundle_id"_el, result.bundleId);
    text("signing.identity"_el, result.signingIdentity);
    text("signing.notary_profile"_el, result.notaryProfile);
    text("signing.certificate_sha1"_el, result.certificateSha1);
    text("signing.certificate_file"_el, result.certificateFile);
    text("signing.csp"_el, result.csp);
    text("signing.key"_el, result.key);
    text("signing.timestamp_server"_el, result.timestampServer);
    text("signing.tool_architecture"_el, result.signingToolArchitecture);
    text("signing.tool_path"_el, result.signingToolPath);
    if (section->hasValue("app"_el)) { result.macApp = section->getBooleanOrThrow("app"_el); }
    if (section->hasValue(conf::NamePath::fromText("signing.enabled"_el))) {
        result.signing = section->getBooleanOrThrow(conf::NamePath::fromText("signing.enabled"_el));
    }
    if (section->hasValue(conf::NamePath::fromText("signing.notarize"_el))) {
        result.notarization = section->getBooleanOrThrow(conf::NamePath::fromText("signing.notarize"_el));
    }
    for (const auto &directory : section->getList<text::String>("dependency_directories"_el)) {
        result.dependencyDirectories.emplace_back(directory);
    }
    addFiles(result, sectionPath);
}

void PackageConfig::addFiles(PackageSettings &result, const text::String &sectionPath) const {
    const auto path = text::StringFormat{"{}.files"_el}.build(sectionPath);
    const auto entries = _document->getSectionList(conf::NamePath::fromText(path));
    if (!entries) { return; }
    for (const auto &entry : *entries) {
        auto file = FileEntry{};
        file.path = path::Path{entry->getTextOrThrow("path"_el)};
        file.target = path::Path{entry->getText("target"_el)};
        file.recursive = entry->getBoolean("recursive"_el, false);
        for (const auto &include : entry->getList<text::String>("include_pattern"_el)) {
            file.includes.append(include);
        }
        for (const auto &exclude : entry->getList<text::String>("exclude_pattern"_el)) {
            file.excludes.append(exclude);
        }
        for (const auto &include : entry->getList<text::String>("include_regex"_el)) {
            file.includeRegex.append(include);
        }
        for (const auto &exclude : entry->getList<text::String>("exclude_regex"_el)) {
            file.excludeRegex.append(exclude);
        }
        result.files.push_back(std::move(file));
    }
}

auto PackageConfig::expand(
    const text::String &format, const PackageSettings &settings, const text::String &platform,
    const text::String &architecture, const text::String &target) -> text::String {
    auto options = text::placeholder::ReplacerOptions{};
    options.setFrame("%{"_el, "}"_el);
    auto replacer = text::placeholder::Replacer{options};
    replacer.addSource(std::make_shared<impl::PackagePlaceholderSource>(settings, platform, architecture, target));
    return replacer.replaceOrThrow(format);
}

}
