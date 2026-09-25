// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PackageApplication.hpp"

#include "PackageFiles.hpp"
#include "PackagePlatform.hpp"

#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/core/Definitions.hpp>
#include <erbsland/options/OptionEditor.hpp>
#include <erbsland/options/OptionParserFlag.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionType.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathCreateDirectoryOptions.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathMoveOptions.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/PathTempDirectoryOptions.hpp>
#include <erbsland/path/TempDirectory.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/Version.hpp>
#include <erbsland/unit/ItemIndex.hpp>

#include <map>

namespace erbsland::package {

using namespace text::literals;

auto PackageApplication::xmlEscape(const text::String &value) -> text::String {
    return value.replacedAll("&"_el, "&amp;"_el)
        .replacedAll("<"_el, "&lt;"_el)
        .replacedAll(">"_el, "&gt;"_el)
        .replacedAll("\""_el, "&quot;"_el)
        .replacedAll("'"_el, "&apos;"_el);
}

void PackageApplication::initialize() {
    info().setApplicationName("Erbsland Core Package Tool"_el);
    info().setApplicationVersion(unit::Version{1, 0, 0});
}

void PackageApplication::registerCommandLineOptions(const options::OptionsPtr &options) {
    options->setHelpTitle("Erbsland Core Package Tool"_el);
    options->setParserFlag(options::OptionParserFlag::DisableVersion);
    for (const auto &name : {"project-root"_el, "project-name"_el, "project-version"_el, "package-name"_el,
                            "config"_el, "output-directory"_el, "architecture"_el, "cmake"_el}) {
        options->addOption({text::StringFormat{"--{}"_el}.build(name), name})
            .setType(options::OptionType::Text).setRequired();
    }
    options->addOption({"--git"_el, "git"_el}).setType(options::OptionType::Text);
    for (const auto &name : {"target-name"_el, "target-path"_el, "bundle-target"_el}) {
        options->addOption({text::StringFormat{"--{}"_el}.build(name), name})
            .setType(options::OptionType::Text).setMaximum(unit::ArgumentCount{128U});
    }
}

auto PackageApplication::readTargets() const -> std::vector<PackageTarget> {
    const auto names = optionValues()->getTextList("target-name"_el);
    const auto paths = optionValues()->getTextList("target-path"_el);
    const auto bundles = optionValues()->getTextList("bundle-target"_el);
    if (names.isEmpty() || names.count() != paths.count()) {
        throw core::ApplicationError{"Each package target needs one name and executable path."_el};
    }
    auto targets = std::vector<PackageTarget>{};
    for (auto index = unit::ItemIndex::zero(); index.isWithin(names.count()); ++index) {
        const auto file = path::Path{paths.get(index)};
        if (!file.info().isRegularFile()) {
            throw core::ApplicationError{text::StringFormat{"Build the required executable before installing: {}"_el}
                                             .build(file.toString())};
        }
        const auto name = names.get(index);
        targets.push_back(PackageTarget{name, file, bundles.contains(name)});
    }
    return targets;
}

auto PackageApplication::main() -> unit::ExitCode {
    const auto values = optionValues();
    const auto projectRoot = path::Path{values->getText("project-root"_el)};
    const auto outputDirectory = path::Path{values->getText("output-directory"_el)};
    const auto configPath = path::Path{values->getText("config"_el)};
    const auto cmake = path::Path{values->getText("cmake"_el)};
    const auto packageName = values->getText("package-name"_el);
    const auto architectureInput = values->getText("architecture"_el);
    const auto architecture = architectureInput == "AMD64"_el || architectureInput == "x86_64"_el ?
        "x64"_el : architectureInput == "aarch64"_el || architectureInput == "ARM64"_el ?
        "arm64"_el : architectureInput;
#if defined(ERBSLAND_OS_WINDOWS)
    const auto platform = "windows"_el;
#else
    const auto platform = "macos"_el;
#endif
    const auto targets = readTargets();
    const auto packageTargetName = targets.size() == 1U ? targets.front().name : text::String{};
    const auto configuration = PackageConfig{configPath};
    auto settings = configuration.settings(
        packageName, platform, architecture, packageTargetName, values->getText("project-name"_el),
        values->getText("project-version"_el));
    if (settings.versionSource == "git"_el) {
        const auto git = values->getText("git"_el);
        if (git.isEmpty()) { throw core::ApplicationError{"Git version source requires Git."_el}; }
        const auto description = PackagePlatform::run(path::Path{git}, text::StringList{
            "-C"_el, projectRoot.toString(), "describe"_el, "--tags"_el, "--long"_el,
            "--match"_el, "v[0-9]*"_el}).trimmed();
        const auto match = re::RegEx::compile("^v([0-9]+\\.[0-9]+\\.[0-9]+)-([0-9]+)-g([0-9a-f]+)$"_el)
                               ->fullMatch(description);
        if (!match) { throw core::ApplicationError{"Git must provide a reachable vMAJOR.MINOR.PATCH tag."_el}; }
        settings.version = match->content(re::CaptureGroupIndex{1U});
        if (match->content(re::CaptureGroupIndex{2U}) != "0"_el) {
            settings.version = text::StringFormat{"{}-{}-{}"_el}.build(
                settings.version, match->content(re::CaptureGroupIndex{2U}),
                match->content(re::CaptureGroupIndex{3U}));
        }
    }
    const auto rootName = PackageConfig::expand(settings.targetDir, settings, platform, architecture, packageTargetName);
    const auto rootRelative = path::Path{rootName};
    PackageFiles::validateRelative(rootRelative);
    const auto filename = PackageConfig::expand(
        settings.filenameFormat, settings, platform, architecture, packageTargetName);
    PackageFiles::validateRelative(path::Path{filename});
    outputDirectory.operations().createDirectoryOrThrow(
        path::PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(path::PathCreateMode::CreateOrOverwrite));
    auto temporary = outputDirectory.operations().createTempDirectoryOrThrow(
        path::PathTempDirectoryOptions{}.setPrefix(".erbsland-package-"_el));
    const auto payload = temporary->path() / "payload"_el;
    const auto packageRoot = payload / rootRelative;
    packageRoot.operations().createDirectoryOrThrow(
        path::PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(path::PathCreateMode::CreateOrOverwrite));
    auto files = PackageFiles{configuration.baseDirectory(), packageRoot};
    for (const auto &entry : settings.files) { files.add(entry); }
    auto copiedLibraries = std::map<text::String, path::Path>{};
    for (const auto &target : targets) {
        auto targetSettings = configuration.settings(
            packageName, platform, architecture, target.name, values->getText("project-name"_el),
            values->getText("project-version"_el));
        targetSettings.version = settings.version;
        if (!targetSettings.certificateFile.isEmpty()) {
            const auto certificate = path::Path{targetSettings.certificateFile};
            if (certificate.isRelative()) {
                targetSettings.certificateFile = (configuration.baseDirectory() / certificate).toString();
            }
        }
        if (!targetSettings.signingToolPath.isEmpty()) {
            const auto tool = path::Path{targetSettings.signingToolPath};
            if (tool.isRelative()) {
                targetSettings.signingToolPath = (configuration.baseDirectory() / tool).toString();
            }
        }
        for (auto index = settings.files.size(); index < targetSettings.files.size(); ++index) {
            files.add(targetSettings.files[index]);
        }
        auto searchDirectories = std::vector<path::Path>{target.file.parent()};
        for (const auto &directory : targetSettings.dependencyDirectories) {
            searchDirectories.push_back(directory.isAbsolute() ? directory : configuration.baseDirectory() / directory);
        }
#if defined(ERBSLAND_OS_WINDOWS)
        const auto stagedExecutable = packageRoot / target.file.name();
        files.copyFile(target.file, path::Path{target.file.name()});
        for (const auto &dependency : PackagePlatform::windowsDependencies(
                 cmake, target.file, temporary->path() / "dependencies.txt"_el, searchDirectories)) {
            const auto found = copiedLibraries.find(dependency.name());
            if (found != copiedLibraries.end()) {
                if (found->second != dependency) { throw core::ApplicationError{"Conflicting DLL names."_el}; }
                if (targetSettings.signing) {
                    PackagePlatform::signWindows(packageRoot / dependency.name(), targetSettings);
                }
                continue;
            }
            files.copyFile(dependency, path::Path{dependency.name()});
            copiedLibraries.emplace(dependency.name(), dependency);
            if (targetSettings.signing) {
                PackagePlatform::signWindows(packageRoot / dependency.name(), targetSettings);
            }
        }
        if (targetSettings.signing) { PackagePlatform::signWindows(stagedExecutable, targetSettings); }
#else
        if (targetSettings.macApp) {
            const auto displayName = targetSettings.name == settings.name ? target.name : targetSettings.name;
            const auto bundleName = text::StringFormat{"{}.app"_el}.build(displayName);
            const auto bundle = packageRoot / bundleName;
            if (target.bundle) {
                files.copyDirectory(target.file.parent().parent().parent(), path::Path{bundleName});
            } else {
                const auto executableRelative = path::Path{bundleName} / "Contents/MacOS"_el / target.file.name();
                files.copyFile(target.file, executableRelative);
                const auto identifier = targetSettings.bundleId.isEmpty() ?
                    text::StringFormat{"local.{}.{}"_el}.build(settings.name, target.name) : targetSettings.bundleId;
                const auto plist = text::StringFormat{
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                    "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                    "<plist version=\"1.0\"><dict>\n"
                    "<key>CFBundleExecutable</key>\n<string>{}</string>\n"
                    "<key>CFBundleIdentifier</key>\n<string>{}</string>\n"
                    "<key>CFBundleName</key>\n<string>{}</string>\n"
                    "<key>CFBundlePackageType</key>\n<string>APPL</string>\n"
                    "<key>CFBundleShortVersionString</key>\n<string>{}</string>\n"
                    "</dict></plist>\n"_el}
                                       .build(
                                           xmlEscape(target.file.name()), xmlEscape(identifier),
                                           xmlEscape(displayName), xmlEscape(settings.version));
                (bundle / "Contents/Info.plist"_el).content().writeTextOrThrow(plist);
            }
            PackagePlatform::fixMacBundle(cmake, bundle, searchDirectories);
            if (targetSettings.signing) { PackagePlatform::signMacBundle(bundle, targetSettings); }
            if (targetSettings.notarization) {
                if (!targetSettings.signing) { throw core::ApplicationError{"Notarization requires signing."_el}; }
                PackagePlatform::notarizeMacBundle(
                    bundle, targetSettings, temporary->path() / "notarization.zip"_el);
            }
        } else {
            if (targetSettings.notarization) {
                throw core::ApplicationError{"Notarization requires macOS app packaging."_el};
            }
            files.copyFile(target.file, path::Path{target.file.name()});
            PackagePlatform::fixMacBundle(cmake, packageRoot / target.file.name(), searchDirectories);
            if (targetSettings.signing) {
                PackagePlatform::signMacBundle(packageRoot / target.file.name(), targetSettings);
            }
        }
#endif
    }
    const auto archive = temporary->path() / "package.zip"_el;
    PackagePlatform::createArchive(payload, archive);
    const auto destination = outputDirectory / text::StringFormat{"{}.zip"_el}.build(filename);
    archive.operations().moveToOrThrow(
        destination, path::PathMoveOptions{}.setCollisionMode(path::PathCollisionMode::Overwrite));
    return unit::ExitCode::success();
}

}
