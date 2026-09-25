// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PackagePlatform.hpp"

#include <erbsland/compression/zip/ArchiveWriter.hpp>
#include <erbsland/compression/zip/ArchiveDirectoryOptions.hpp>
#include <erbsland/core/Definitions.hpp>
#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathWalkDirection.hpp>
#include <erbsland/path/PathWalkOptions.hpp>
#include <erbsland/path/PathWalkStatus.hpp>
#include <erbsland/path/PathWalker.hpp>
#include <erbsland/system/Subprocess.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/system/SubprocessOptions.hpp>
#include <erbsland/system/SubprocessOutputMode.hpp>
#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringFormat.hpp>

namespace erbsland::package {

using namespace text::literals;

auto PackagePlatform::run(
    const path::Path &program, const text::StringList &arguments, const text::String &secret) -> text::String {
    auto options = system::SubprocessOptions{};
    options.setStandardOutputMode(system::SubprocessOutputMode::Capture)
        .setStandardErrorMode(system::SubprocessOutputMode::Capture);
    auto process = system::Subprocess::start(program, arguments, options);
    const auto status = process.wait();
    auto output = text::StringFormat{"{}\n{}"_el}.build(process.standardOutput(), process.standardError());
    if (!secret.isEmpty()) { output = output.replacedAll(secret, "*****"_el); }
    if (!status.isSuccess()) {
        throw core::ApplicationError{text::StringFormat{"Package command failed: {}\n{}"_el}
                                         .build(program.toString(), output)};
    }
    return output;
}

auto PackagePlatform::windowsDependencies(
    const path::Path &cmake, const path::Path &executable, const path::Path &output,
    const std::vector<path::Path> &searchDirectories) -> std::vector<path::Path> {
    auto paths = text::StringList{};
    for (const auto &directory : searchDirectories) { paths.append(directory.toString()); }
    run(cmake, text::StringList{
                   text::StringFormat{"-DPACKAGE_EXECUTABLE={}"_el}.build(executable.toString()),
                   text::StringFormat{"-DPACKAGE_DEPENDENCY_OUTPUT={}"_el}.build(output.toString()),
                   text::StringFormat{"-DPACKAGE_LIBRARY_DIRECTORIES={}"_el}.build(paths.join(";"_el)),
                   "-P"_el, (path::Path{ERBSLAND_CORE_PACKAGE_SCRIPT_DIR} / "windows-dependencies.cmake"_el).toString()});
    auto result = std::vector<path::Path>{};
    const auto lines = text::StringList::fromSplit(
        output.content().readTextOrThrow(), text::CharSet{"\n"_el}, unit::ItemCount::infinite(), true);
    for (const auto &line : lines) {
        if (!line.isEmpty()) { result.emplace_back(line.trimmed()); }
    }
    return result;
}

void PackagePlatform::fixMacBundle(
    const path::Path &cmake, const path::Path &bundle, const std::vector<path::Path> &searchDirectories) {
    auto paths = text::StringList{};
    for (const auto &directory : searchDirectories) { paths.append(directory.toString()); }
    run(cmake, text::StringList{
                   text::StringFormat{"-DPACKAGE_BUNDLE={}"_el}.build(bundle.toString()),
                   text::StringFormat{"-DPACKAGE_LIBRARY_DIRECTORIES={}"_el}.build(paths.join(";"_el)),
                   "-P"_el, (path::Path{ERBSLAND_CORE_PACKAGE_SCRIPT_DIR} / "mac-fixup.cmake"_el).toString()});
}

void PackagePlatform::signMacBundle(const path::Path &bundle, const PackageSettings &settings) {
    if (settings.signingIdentity.isEmpty()) {
        throw core::ApplicationError{"macOS signing requires signing.identity."_el};
    }
    // A leaf-to-root walk signs embedded code before the bundle containing it.
    if (bundle.info().isDirectory()) {
        const auto options = path::PathWalkOptions{}
            .setSymlinkMode(path::SymlinkMode::Skip)
            .setDirection(path::PathWalkDirection::LeafToRoot);
        bundle.walker().walkOrThrow([&](const path::Path &item, const path::PathInfo &info) -> path::PathWalkStatus {
            if (item == bundle) { return path::PathWalkStatus::Continue; }
            const auto name = item.name();
            const auto isNestedBundle = info.isDirectory() &&
                (name.endsWith(".framework"_el) || name.endsWith(".app"_el) || name.endsWith(".bundle"_el) ||
                 name.endsWith(".appex"_el) || name.endsWith(".xpc"_el));
            const auto isNestedLibrary = info.isRegularFile() &&
                (name.endsWith(".dylib"_el) || name.endsWith(".so"_el));
            if (isNestedBundle || isNestedLibrary) {
                run(path::Path{"/usr/bin/codesign"_el}, text::StringList{
                    "--force"_el, "--options"_el, "runtime"_el, "--timestamp"_el,
                    "--sign"_el, settings.signingIdentity, item.toString()});
            }
            return path::PathWalkStatus::Continue;
        }, options);
    }
    run(path::Path{"/usr/bin/codesign"_el}, text::StringList{
        "--force"_el, "--options"_el, "runtime"_el, "--timestamp"_el,
        "--sign"_el, settings.signingIdentity, bundle.toString()});
    run(path::Path{"/usr/bin/codesign"_el}, text::StringList{
        "--verify"_el, "--deep"_el, "--strict"_el, bundle.toString()});
}

void PackagePlatform::notarizeMacBundle(
    const path::Path &bundle, const PackageSettings &settings, const path::Path &temporaryZip) {
    if (settings.notaryProfile.isEmpty()) {
        throw core::ApplicationError{"macOS notarization requires signing.notary_profile."_el};
    }
    run(path::Path{"/usr/bin/ditto"_el}, text::StringList{
        "-c"_el, "-k"_el, "--sequesterRsrc"_el, "--keepParent"_el, bundle.toString(), temporaryZip.toString()});
    const auto result = run(path::Path{"/usr/bin/xcrun"_el}, text::StringList{
        "notarytool"_el, "submit"_el, temporaryZip.toString(), "--keychain-profile"_el,
        settings.notaryProfile, "--wait"_el});
    if (!result.contains("Accepted"_el)) {
        throw core::ApplicationError{text::StringFormat{"Notarization was not accepted: {}"_el}.build(result)};
    }
    run(path::Path{"/usr/bin/xcrun"_el}, text::StringList{"stapler"_el, "staple"_el, bundle.toString()});
    run(path::Path{"/usr/bin/xcrun"_el}, text::StringList{"stapler"_el, "validate"_el, bundle.toString()});
}

void PackagePlatform::createArchive(const path::Path &packageRoot, const path::Path &temporaryZip) {
#if defined(ERBSLAND_OS_WINDOWS)
    auto writer = compression::zip::ArchiveWriter::create(temporaryZip);
    writer->addDirectory(packageRoot);
    writer->finalize();
#else
    run(path::Path{"/usr/bin/ditto"_el}, text::StringList{
        "-c"_el, "-k"_el, "--sequesterRsrc"_el,
        packageRoot.toString(), temporaryZip.toString()});
#endif
}

void PackagePlatform::signWindows(const path::Path &executable, const PackageSettings &settings) {
#if defined(ERBSLAND_OS_WINDOWS)
    auto tool = path::Path{settings.signingToolPath};
    if (!tool.isValid()) {
        const auto environment = system::EnvironmentVariables{};
        const auto sdkDirectory = environment.get("WindowsSdkDir"_el, {});
        const auto sdkVersion = environment.get("WindowsSDKVersion"_el, {});
        auto hostArchitecture = settings.signingToolArchitecture.isEmpty() ?
            environment.get("PROCESSOR_ARCHITECTURE"_el, "x64"_el) : settings.signingToolArchitecture;
        if (hostArchitecture == "AMD64"_el) { hostArchitecture = "x64"_el; }
        if (hostArchitecture == "ARM64"_el) { hostArchitecture = "arm64"_el; }
        if (sdkDirectory.isEmpty() || sdkVersion.isEmpty()) {
            throw core::ApplicationError{
                "Windows signing requires signing.tool_path or WindowsSdkDir and WindowsSDKVersion."_el};
        }
        tool = path::Path{sdkDirectory} / "bin"_el / sdkVersion / hostArchitecture / "signtool.exe"_el;
    }
    if (!tool.info().isRegularFile()) {
        throw core::ApplicationError{text::StringFormat{"SignTool was not found: {}"_el}.build(tool.toString())};
    }
    auto arguments = text::StringList{"sign"_el, "/fd"_el, "sha256"_el, "/td"_el, "sha256"_el};
    if (settings.timestampServer.isEmpty()) {
        throw core::ApplicationError{"Windows signing requires signing.timestamp_server."_el};
    }
    arguments.append("/tr"_el).append(settings.timestampServer);
    if (!settings.certificateSha1.isEmpty()) {
        arguments.append("/sha1"_el).append(settings.certificateSha1);
    } else {
        if (settings.certificateFile.isEmpty() || settings.csp.isEmpty() || settings.key.isEmpty()) {
            throw core::ApplicationError{"Windows signing requires a certificate SHA-1 or file, CSP, and key."_el};
        }
        arguments.append("/f"_el).append(settings.certificateFile)
            .append("/csp"_el).append(settings.csp).append("/k"_el).append(settings.key);
    }
    arguments.append(executable.toString());
    run(tool, arguments, settings.key);
    run(tool, text::StringList{"verify"_el, "/v"_el, "/pa"_el, executable.toString()});
#else
    (void)executable;
    (void)settings;
    throw core::ApplicationError{"Windows signing is unavailable on this platform."_el};
#endif
}

}
