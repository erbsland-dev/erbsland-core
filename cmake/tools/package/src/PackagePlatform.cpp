// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PackagePlatform.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/Definitions.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalkDirection.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/PathWalkOptions.hpp>
#include <erbsland/PathWalkStatus.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/Subprocess.hpp>
#include <erbsland/SubprocessOptions.hpp>
#include <erbsland/SubprocessOutputMode.hpp>
#include <erbsland/zip/ArchiveDirectoryOptions.hpp>
#include <erbsland/zip/ArchiveWriter.hpp>

namespace erbsland::package {

using namespace path;
using namespace text;
using namespace text::literals;

auto PackagePlatform::run(const Path &program, const StringList &arguments, const String &secret) -> String {
    auto options = system::SubprocessOptions{};
    options.setStandardOutputMode(system::SubprocessOutputMode::Capture)
        .setStandardErrorMode(system::SubprocessOutputMode::Capture);
    auto process = system::Subprocess::create(program, arguments, options);
    process->start();
    const auto status = process->wait();
    auto output = StringFormat{"{}\n{}"_el}.build(process->standardOutput(), process->standardError());
    if (!secret.isEmpty()) {
        output = output.replacedAll(secret, "*****"_el);
    }
    if (!status.isSuccess()) {
        throw core::ApplicationError{
            StringFormat{"Package command failed: {}\n{}"_el}.build(program.toString(), output)};
    }
    return output;
}

auto PackagePlatform::windowsDependencies(
    const Path &cmake, const Path &executable, const Path &output, const std::vector<Path> &searchDirectories)
    -> std::vector<Path> {
    auto paths = StringList{};
    for (const auto &directory : searchDirectories) {
        paths.append(directory.toString());
    }
    run(cmake,
        StringList{
            StringFormat{"-DPACKAGE_EXECUTABLE={}"_el}.build(executable.toString()),
            StringFormat{"-DPACKAGE_DEPENDENCY_OUTPUT={}"_el}.build(output.toString()),
            StringFormat{"-DPACKAGE_LIBRARY_DIRECTORIES={}"_el}.build(paths.join(";"_el)),
            "-P"_el,
            (Path{ERBSLAND_CORE_PACKAGE_SCRIPT_DIR} / "windows-dependencies.cmake"_el).toString()});
    auto result = std::vector<Path>{};
    const auto lines =
        StringList::fromSplit(output.content().readTextOrThrow(), CharSet{"\n"_el}, unit::ItemCount::infinite(), true);
    for (const auto &line : lines) {
        if (!line.isEmpty()) {
            result.emplace_back(line.trimmed());
        }
    }
    return result;
}

void PackagePlatform::fixMacBundle(const Path &cmake, const Path &bundle, const std::vector<Path> &searchDirectories) {
    auto paths = StringList{};
    for (const auto &directory : searchDirectories) {
        paths.append(directory.toString());
    }
    run(cmake,
        StringList{
            StringFormat{"-DPACKAGE_BUNDLE={}"_el}.build(bundle.toString()),
            StringFormat{"-DPACKAGE_LIBRARY_DIRECTORIES={}"_el}.build(paths.join(";"_el)),
            "-P"_el,
            (Path{ERBSLAND_CORE_PACKAGE_SCRIPT_DIR} / "mac-fixup.cmake"_el).toString()});
}

void PackagePlatform::signMacBundle(const Path &bundle, const PackageSettings &settings) {
    if (settings.signingIdentity.isEmpty()) {
        throw core::ApplicationError{"macOS signing requires signing.identity."_el};
    }
    // A leaf-to-root walk signs embedded code before the bundle containing it.
    if (bundle.info().isDirectory()) {
        const auto options =
            PathWalkOptions{}.setSymlinkMode(SymlinkMode::Skip).setDirection(PathWalkDirection::LeafToRoot);
        bundle.walker().walkOrThrow(
            [&](const Path &item, const PathInfo &info) -> PathWalkStatus {
                if (item == bundle) {
                    return PathWalkStatus::Continue;
                }
                const auto name = item.name();
                const auto isNestedBundle = info.isDirectory() &&
                    (name.endsWith(".framework"_el) || name.endsWith(".app"_el) || name.endsWith(".bundle"_el) ||
                        name.endsWith(".appex"_el) || name.endsWith(".xpc"_el));
                const auto isNestedLibrary =
                    info.isRegularFile() && (name.endsWith(".dylib"_el) || name.endsWith(".so"_el));
                if (isNestedBundle || isNestedLibrary) {
                    run(Path{"/usr/bin/codesign"_el},
                        StringList{
                            "--force"_el,
                            "--options"_el,
                            "runtime"_el,
                            "--timestamp"_el,
                            "--sign"_el,
                            settings.signingIdentity,
                            item.toString()});
                }
                return PathWalkStatus::Continue;
            },
            options);
    }
    run(Path{"/usr/bin/codesign"_el},
        StringList{
            "--force"_el,
            "--options"_el,
            "runtime"_el,
            "--timestamp"_el,
            "--sign"_el,
            settings.signingIdentity,
            bundle.toString()});
    run(Path{"/usr/bin/codesign"_el}, StringList{"--verify"_el, "--deep"_el, "--strict"_el, bundle.toString()});
}

void PackagePlatform::notarizeMacBundle(const Path &bundle, const PackageSettings &settings, const Path &temporaryZip) {
    if (settings.notaryProfile.isEmpty()) {
        throw core::ApplicationError{"macOS notarization requires signing.notary_profile."_el};
    }
    run(Path{"/usr/bin/ditto"_el},
        StringList{
            "-c"_el, "-k"_el, "--sequesterRsrc"_el, "--keepParent"_el, bundle.toString(), temporaryZip.toString()});
    const auto result =
        run(Path{"/usr/bin/xcrun"_el},
            StringList{
                "notarytool"_el,
                "submit"_el,
                temporaryZip.toString(),
                "--keychain-profile"_el,
                settings.notaryProfile,
                "--wait"_el});
    if (!result.contains("Accepted"_el)) {
        throw core::ApplicationError{StringFormat{"Notarization was not accepted: {}"_el}.build(result)};
    }
    run(Path{"/usr/bin/xcrun"_el}, StringList{"stapler"_el, "staple"_el, bundle.toString()});
    run(Path{"/usr/bin/xcrun"_el}, StringList{"stapler"_el, "validate"_el, bundle.toString()});
}

void PackagePlatform::createArchive(const Path &packageRoot, const Path &temporaryZip) {
#if defined(ERBSLAND_OS_WINDOWS)
    auto writer = compression::zip::ArchiveWriter::create(temporaryZip);
    writer->addDirectory(packageRoot);
    writer->finalize();
#else
    run(Path{"/usr/bin/ditto"_el},
        StringList{"-c"_el, "-k"_el, "--sequesterRsrc"_el, packageRoot.toString(), temporaryZip.toString()});
#endif
}

void PackagePlatform::signWindows(const Path &executable, const PackageSettings &settings) {
#if defined(ERBSLAND_OS_WINDOWS)
    auto tool = Path{settings.signingToolPath};
    if (!tool.isValid()) {
        const auto environment = system::EnvironmentVariables{};
        const auto sdkDirectory = environment.get("WindowsSdkDir"_el, {});
        const auto sdkVersion = environment.get("WindowsSDKVersion"_el, {});
        auto hostArchitecture = settings.signingToolArchitecture.isEmpty()
            ? environment.get("PROCESSOR_ARCHITECTURE"_el, "x64"_el)
            : settings.signingToolArchitecture;
        if (hostArchitecture == "AMD64"_el) {
            hostArchitecture = "x64"_el;
        }
        if (hostArchitecture == "ARM64"_el) {
            hostArchitecture = "arm64"_el;
        }
        if (sdkDirectory.isEmpty() || sdkVersion.isEmpty()) {
            throw core::ApplicationError{
                "Windows signing requires signing.tool_path or WindowsSdkDir and WindowsSDKVersion."_el};
        }
        tool = Path{sdkDirectory} / "bin"_el / sdkVersion / hostArchitecture / "signtool.exe"_el;
    }
    if (!tool.info().isRegularFile()) {
        throw core::ApplicationError{StringFormat{"SignTool was not found: {}"_el}.build(tool.toString())};
    }
    auto arguments = StringList{"sign"_el, "/fd"_el, "sha256"_el, "/td"_el, "sha256"_el};
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
        arguments.append("/f"_el)
            .append(settings.certificateFile)
            .append("/csp"_el)
            .append(settings.csp)
            .append("/k"_el)
            .append(settings.key);
    }
    arguments.append(executable.toString());
    run(tool, arguments, settings.key);
    run(tool, StringList{"verify"_el, "/v"_el, "/pa"_el, executable.toString()});
#else
    (void)executable;
    (void)settings;
    throw core::ApplicationError{"Windows signing is unavailable on this platform."_el};
#endif
}

}
