// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DemoCapture.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/EnvironmentChanges.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/Subprocess.hpp>
#include <erbsland/TimeDelta.hpp>

#include <algorithm>

namespace tool::doc::impl {

using namespace el::text::literals;

auto DemoCapture::run(
    const el::Path &executable,
    const el::StringList &arguments,
    const el::Path &directory,
    const int expectedExitCode,
    const int width) -> el::String {
    if (!executable.info().isRegularFile() || !executable.info().isExecutable()) {
        throwError("Demo file is not executable."_el, executable.toString());
    }
    auto environment = el::EnvironmentChanges{};
    environment.setInherit(
        el::StringList{
            "HOME"_el,
            "LANG"_el,
            "LC_ALL"_el,
            "LC_CTYPE"_el,
            "PATH"_el,
            "TMPDIR"_el,
            "TEMP"_el,
            "TMP"_el,
            "SystemRoot"_el,
            "WINDIR"_el});
    environment.setVariable("ERBSLAND_VIRTUAL_TTY"_el, "1"_el);
    environment.setVariable(
        "ERBSLAND_VIRTUAL_TTY_SIZE"_el, el::StringFormat{"{}x40"_el}.build(std::clamp(width, 90, 2048)));
    const auto options = el::SubprocessOptions{}
                             .setWorkingDirectory(directory)
                             .setEnvironmentChanges(std::move(environment))
                             .setInheritStandardInput(false)
                             .setStandardOutputMode(el::SubprocessOutputMode::Capture)
                             .setMergeStandardError(true)
                             .setCaptureLimit(el::ByteCount{1024U * 1024U});
    const auto process = el::Subprocess::create(executable, arguments, options);
    process->start();
    const auto status = process->wait(el::TimeDelta{el::Seconds{10}});
    if (!status) {
        process->kill();
        const auto killedStatus = process->wait();
        (void)killedStatus;
        throwError("Demo execution timed out after 10 seconds."_el, executable.toString());
    }
    if (process->wasStandardOutputTruncated()) {
        throwError("Demo output exceeds 1 MiB."_el, executable.toString());
    }
    if (status->exitCode() != expectedExitCode) {
        throwError("Demo exited with an unexpected status."_el, executable.toString());
    }
    return process->standardOutput();
}

auto DemoCapture::create() -> std::unique_ptr<DemoCapture> {
    return std::make_unique<DemoCapture>();
}

void DemoCapture::throwError(const el::String &description, const el::String &path) {
    throw el::ApplicationError{"Demo Capture Failed"_el, description}.withSourcePath(path);
}

}
