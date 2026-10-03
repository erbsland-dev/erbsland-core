// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ExternalCommand.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PlatformError.hpp>
#include <erbsland/Subprocess.hpp>

namespace tool::frame {

using namespace el::text::literals;

auto ExternalCommand::directories(const el::conf::ValuePtr &value) -> el::PathList {
    auto result = el::PathList{};
    for (const auto &entry : value->toValueList()) {
        try {
            auto path = el::Path{entry->asTextOrThrow()};
            if (!path.isAbsolute()) {
                throw el::conf::ConfError{
                    entry, "Invalid Executable Directory"_el, "Use an absolute directory path."_el};
            }
            if (path.info().exists()) {
                path = path.resolveOrThrow();
                if (!path.info().isDirectory()) {
                    throw el::conf::ConfError{
                        entry, "Invalid Executable Directory"_el, "The path is not a directory."_el};
                }
            } else {
                path = path.resolveOrThrow(el::PathResolveMode::Lexical);
            }
            result.append(path);
        } catch (const el::PathError &) {
            throw el::conf::ConfError{
                entry,
                "Invalid Executable Directory"_el,
                "The directory could not be resolved."_el,
                std::current_exception()};
        }
    }
    return result;
}

void ExternalCommand::configure(
    const el::conf::ValuePtr &value,
    const el::PathList &directories,
    const el::Path &projectRoot,
    const bool required) {
    _executable = {};
    const auto name = value->asTextOrThrow();
    try {
        const auto path = el::Path{name};
        if (name.isEmpty() || (!path.isAbsolute() && path.elementCount() != el::ItemCount{1})) {
            throw el::conf::ConfError{
                value, "Invalid Executable"_el, "Use an executable basename or an absolute path."_el};
        }
        for (const auto &candidate : el::EnvironmentVariables{}.executableCandidatesOrThrow(name)) {
            const auto resolved = candidate.resolve();
            if (resolved.isEmpty() || resolved.isRelativeTo(projectRoot) || !resolved.info().isRegularFile() ||
                !resolved.info().isExecutable()) {
                continue;
            }
            for (const auto &directory : directories) {
                if (resolved.isRelativeTo(directory)) {
                    _executable = resolved;
                    return;
                }
            }
        }
    } catch (const el::PathError &) {
        throw el::conf::ConfError{
            value, "Invalid Executable"_el, "Executable lookup failed."_el, std::current_exception()};
    }
    if (required) {
        throw el::conf::ConfError{
            value,
            "Trusted Executable Unavailable"_el,
            "Install the command and include its resolved directory in Safe Binary Directories."_el};
    }
}

auto ExternalCommand::run(const el::StringList &arguments, const el::Path &workingDirectory) const -> CommandOutput {
    return runExecutable(_executable, arguments, workingDirectory);
}

auto ExternalCommand::runExecutable(
    const el::Path &executable,
    const el::StringList &arguments,
    const el::Path &workingDirectory,
    const bool removeOutputBom) -> CommandOutput {
    const auto inherited = el::StringList{
        "HOME"_el,
        "PATH"_el,
        "LANG"_el,
        "LC_ALL"_el,
        "LC_CTYPE"_el,
        "TMPDIR"_el,
        "TEMP"_el,
        "TMP"_el,
        "SystemRoot"_el,
        "WINDIR"_el};
    const auto options =
        el::SubprocessOptions{}
            .setWorkingDirectory(workingDirectory)
            .setInheritStandardInput(false)
            .setStandardOutputMode(el::SubprocessOutputMode::Capture)
            .setStandardErrorMode(el::SubprocessOutputMode::Capture)
            .setCaptureLimit(cCaptureLimit)
            .setRemoveOutputBom(removeOutputBom)
            .setEnvironmentChanges(
                el::EnvironmentChanges{}.setInherit(inherited).setVariable("GIT_TERMINAL_PROMPT"_el, "0"_el));
    try {
        auto process = el::Subprocess::create(executable, arguments, options);
        process->start();
        const auto status = process->wait();
        if (process->wasStandardOutputTruncated() || process->wasStandardErrorTruncated()) {
            throw el::ApplicationError{
                "Command Output Truncated"_el, "The command exceeded the capture limit; its output cannot be used."_el}
                .withSourcePath(executable.toString());
        }
        return {status, process->standardOutput(), process->standardError()};
    } catch (const el::PlatformError &) {
        throw el::ApplicationError{
            "External Command Failed"_el,
            "The command could not be launched or collected."_el,
            std::current_exception()}
            .withSourcePath(executable.toString());
    }
}

}
