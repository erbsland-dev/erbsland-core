// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PackageApplication.hpp"

#include "PackageFiles.hpp"
#include "PackagePlatform.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/Definitions.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/OptionEditor.hpp>
#include <erbsland/OptionParserFlag.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathCreateDirectoryOptions.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathMoveOptions.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PathTempDirectoryOptions.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringLiteral.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/Version.hpp>

#include <map>

namespace erbsland::package {

using namespace options;
using namespace path;
using namespace text;
using namespace unit;
using namespace text::literals;

auto PackageApplication::xmlEscape(const String &value) -> String {
    return value.replacedAll("&"_el, "&amp;"_el)
        .replacedAll("<"_el, "&lt;"_el)
        .replacedAll(">"_el, "&gt;"_el)
        .replacedAll("\""_el, "&quot;"_el)
        .replacedAll("'"_el, "&apos;"_el);
}

void PackageApplication::initialize() {
    info().setApplicationName("Erbsland Core Package Tool"_el);
    info().setApplicationVersion(Version{1, 0, 0});
}

void PackageApplication::registerCommandLineOptions(const OptionsPtr &options) {
    options->setHelpTitle("Erbsland Core Package Tool"_el);
    options->setParserFlag(OptionParserFlag::DisableVersion);
    for (
        const auto &name :
        {"project-root"_el,
            cProjectNameOption,
            cProjectVersionOption,
            "package-name"_el,
            "config"_el,
            "output-directory"_el,
            "architecture"_el,
            "cmake"_el}) {
        options->addOption({StringFormat{"--{}"_el}.build(name), name}).setType(OptionType::Text).setRequired();
    }
    options->addOption({"--git"_el, "git"_el}).setType(OptionType::Text);
    for (const auto &name : {"target-name"_el, "target-path"_el, "bundle-target"_el}) {
        options->addOption({StringFormat{"--{}"_el}.build(name), name})
            .setType(OptionType::Text)
            .setMaximum(ArgumentCount{128U});
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
    for (auto index = ItemIndex::zero(); index.isWithin(names.count()); ++index) {
        const auto file = Path{paths.get(index)};
        if (!file.info().isRegularFile()) {
            throw core::ApplicationError{
                StringFormat{"Build the required executable before installing: {}"_el}.build(file.toString())};
        }
        const auto name = names.get(index);
        targets.push_back(PackageTarget{name, file, bundles.contains(name)});
    }
    return targets;
}

auto PackageApplication::main() -> ExitCode {
    const auto values = optionValues();
    const auto projectRoot = Path{values->getText("project-root"_el)};
    const auto outputDirectory = Path{values->getText("output-directory"_el)};
    const auto configPath = Path{values->getText("config"_el)};
    const auto cmake = Path{values->getText("cmake"_el)};
    const auto packageName = values->getText("package-name"_el);
    const auto architectureInput = values->getText("architecture"_el);
    const auto architecture = architectureInput == "AMD64"_el || architectureInput == "x86_64"_el ? "x64"_el
        : architectureInput == "aarch64"_el || architectureInput == "ARM64"_el                    ? "arm64"_el
                                                                                                  : architectureInput;
#if defined(ERBSLAND_OS_WINDOWS)
    const auto platform = "windows"_el;
#else
    const auto platform = "macos"_el;
#endif
    const auto targets = readTargets();
    const auto packageTargetName = targets.size() == 1U ? targets.front().name : String{};
    const auto configuration = PackageConfig{configPath};
    auto settings = configuration.settings(
        packageName,
        platform,
        architecture,
        packageTargetName,
        values->getText(cProjectNameOption),
        values->getText(cProjectVersionOption));
    if (settings.versionSource == "git"_el) {
        const auto git = values->getText("git"_el);
        if (git.isEmpty()) {
            throw core::ApplicationError{"Git version source requires Git."_el};
        }
        const auto args = StringList{
            "-C"_el,
            projectRoot.toString(),
            "describe"_el,
            "--tags"_el,
            "--long"_el,
            "--match"_el,
            "v[0-9]*"_el,
        };
        const auto description = PackagePlatform::run(Path{git}, args).trimmed();
        const auto match =
            re::RegEx::compile("^v([0-9]+\\.[0-9]+\\.[0-9]+)-([0-9]+)-g([0-9a-f]+)$"_el)->fullMatch(description);
        if (!match) {
            throw core::ApplicationError{"Git must provide a reachable vMAJOR.MINOR.PATCH tag."_el};
        }
        settings.version = match->content(re::CaptureGroupIndex{1U});
        if (match->content(re::CaptureGroupIndex{2U}) != "0"_el) {
            settings.version = StringFormat{"{}-{}-{}"_el}.build(
                settings.version, match->content(re::CaptureGroupIndex{2U}), match->content(re::CaptureGroupIndex{3U}));
        }
    }
    const auto rootName =
        PackageConfig::expand(settings.targetDir, settings, platform, architecture, packageTargetName);
    const auto rootRelative = Path{rootName};
    PackageFiles::validateRelative(rootRelative);
    const auto filename =
        PackageConfig::expand(settings.filenameFormat, settings, platform, architecture, packageTargetName);
    PackageFiles::validateRelative(Path{filename});
    outputDirectory.operations().createDirectoryOrThrow(
        PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(PathCreateMode::CreateOrOverwrite));
    auto temporary = outputDirectory.operations().createTempDirectoryOrThrow(
        PathTempDirectoryOptions{}.setPrefix(".erbsland-package-"_el));
    const auto payload = temporary->path() / "payload"_el;
    const auto packageRoot = payload / rootRelative;
    packageRoot.operations().createDirectoryOrThrow(
        PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(PathCreateMode::CreateOrOverwrite));
    auto files = PackageFiles{configuration.baseDirectory(), packageRoot};
    for (const auto &entry : settings.files) {
        files.add(entry);
    }
    auto copiedLibraries = StringMap<Path>{};
    for (const auto &target : targets) {
        auto targetSettings = configuration.settings(
            packageName,
            platform,
            architecture,
            target.name,
            values->getText(cProjectNameOption),
            values->getText(cProjectVersionOption));
        targetSettings.version = settings.version;
        if (!targetSettings.certificateFile.isEmpty()) {
            const auto certificate = Path{targetSettings.certificateFile};
            if (certificate.isRelative()) {
                targetSettings.certificateFile = (configuration.baseDirectory() / certificate).toString();
            }
        }
        if (!targetSettings.signingToolPath.isEmpty()) {
            const auto tool = Path{targetSettings.signingToolPath};
            if (tool.isRelative()) {
                targetSettings.signingToolPath = (configuration.baseDirectory() / tool).toString();
            }
        }
        for (auto index = settings.files.size(); index < targetSettings.files.size(); ++index) {
            files.add(targetSettings.files[index]);
        }
        auto searchDirectories = std::vector<Path>{target.file.parent()};
        for (const auto &directory : targetSettings.dependencyDirectories) {
            searchDirectories.push_back(directory.isAbsolute() ? directory : configuration.baseDirectory() / directory);
        }
#if defined(ERBSLAND_OS_WINDOWS)
        const auto stagedExecutable = packageRoot / target.file.name();
        files.copyFile(target.file, Path{target.file.name()});
        for (
            const auto &dependency : PackagePlatform::windowsDependencies(
                cmake, target.file, temporary->path() / "dependencies.txt"_el, searchDirectories)) {
            if (const auto found = copiedLibraries.get(dependency.name())) {
                if (*found != dependency) {
                    throw core::ApplicationError{"Conflicting DLL names."_el};
                }
                if (targetSettings.signing) {
                    PackagePlatform::signWindows(packageRoot / dependency.name(), targetSettings);
                }
                continue;
            }
            files.copyFile(dependency, Path{dependency.name()});
            copiedLibraries.set(dependency.name(), dependency);
            if (targetSettings.signing) {
                PackagePlatform::signWindows(packageRoot / dependency.name(), targetSettings);
            }
        }
        if (targetSettings.signing) {
            PackagePlatform::signWindows(stagedExecutable, targetSettings);
        }
#else
        if (targetSettings.macApp) {
            const auto displayName = targetSettings.name == settings.name ? target.name : targetSettings.name;
            const auto bundleName = StringFormat{"{}.app"_el}.build(displayName);
            const auto bundle = packageRoot / bundleName;
            if (target.bundle) {
                files.copyDirectory(target.file.parent().parent().parent(), Path{bundleName});
            } else {
                const auto executableRelative = Path{bundleName} / "Contents/MacOS"_el / target.file.name();
                files.copyFile(target.file, executableRelative);
                const auto identifier = targetSettings.bundleId.isEmpty()
                    ? StringFormat{"local.{}.{}"_el}.build(settings.name, target.name)
                    : targetSettings.bundleId;
                const auto plist = StringFormat{"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
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
                                           xmlEscape(target.file.name()),
                                           xmlEscape(identifier),
                                           xmlEscape(displayName),
                                           xmlEscape(settings.version));
                (bundle / "Contents/Info.plist"_el).content().writeTextOrThrow(plist);
            }
            PackagePlatform::fixMacBundle(cmake, bundle, searchDirectories);
            if (targetSettings.signing) {
                PackagePlatform::signMacBundle(bundle, targetSettings);
            }
            if (targetSettings.notarization) {
                if (!targetSettings.signing) {
                    throw core::ApplicationError{"Notarization requires signing."_el};
                }
                PackagePlatform::notarizeMacBundle(bundle, targetSettings, temporary->path() / "notarization.zip"_el);
            }
        } else {
            if (targetSettings.notarization) {
                throw core::ApplicationError{"Notarization requires macOS app packaging."_el};
            }
            files.copyFile(target.file, Path{target.file.name()});
            PackagePlatform::fixMacBundle(cmake, packageRoot / target.file.name(), searchDirectories);
            if (targetSettings.signing) {
                PackagePlatform::signMacBundle(packageRoot / target.file.name(), targetSettings);
            }
        }
#endif
    }
    const auto archive = temporary->path() / "package.zip"_el;
    PackagePlatform::createArchive(payload, archive);
    const auto destination = outputDirectory / StringFormat{"{}.zip"_el}.build(filename);
    archive.operations().moveToOrThrow(destination, PathMoveOptions{}.setCollisionMode(PathCollisionMode::Overwrite));
    return ExitCode::success();
}

}
