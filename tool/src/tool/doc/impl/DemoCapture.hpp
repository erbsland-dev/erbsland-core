// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Path.hpp>
#include <erbsland/StringList.hpp>

#include <memory>

namespace tool::doc::impl {

/// Replaceable boundary for bounded, noninteractive subprocess demo capture.
class DemoCapture {
public:
    // defaults/deletions
    DemoCapture() = default;
    virtual ~DemoCapture() = default;
    DemoCapture(const DemoCapture &) = delete;
    auto operator=(const DemoCapture &) -> DemoCapture & = delete;

public: // capture
    /// Run one validated executable, returning raw merged terminal bytes.
    /// @param executable Confined regular executable.
    /// @param arguments Validated arguments, excluding argv[0].
    /// @param directory Project working directory.
    /// @param expectedExitCode Accepted ordinary exit status.
    /// @param width Virtual terminal width large enough to redact temporary paths before conversion.
    /// @return Raw UTF-8/ANSI byte capture.
    virtual auto run(
        const el::Path &executable,
        const el::StringList &arguments,
        const el::Path &directory,
        int expectedExitCode,
        int width = 90) -> el::String;

public: // factory
    /// Create a portable subprocess capture backend.
    /// @return A backend with exclusive ownership.
    static auto create() -> std::unique_ptr<DemoCapture>;

private:
    /// Report a failure in this operation, optionally identifying its input file.
    [[noreturn]] static void throwError(const el::String &description, const el::String &path = {});
};

}
