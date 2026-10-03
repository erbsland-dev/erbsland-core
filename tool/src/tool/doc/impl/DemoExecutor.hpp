// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DemoCapture.hpp"
#include "DemoRunOutput.hpp"

#include "../../frame/ProjectFiles.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/StringMap.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

/// Validate demo command arguments and normalize bounded ANSI output.
class DemoExecutor final {
    /// Validated argument expansion and the metadata needed to redact its output.
    struct ExpandedArguments {
        el::StringList values;                ///< Native argument values passed to the process.
        el::StringList displayed;             ///< Redacted argument values shown in documentation.
        el::StringMap<el::String> redactions; ///< Native path-to-display substitutions.
        int width{90};                        ///< Capture width including space for expanded paths.
    };

public:
    /// Bind execution to confined project files and a replaceable capture backend.
    /// @param files Project file service.
    /// @param capture Backend; omitted selects subprocess capture.
    /// @param executableRoot Project-relative or absolute directory containing demo executables.
    explicit DemoExecutor(
        const frame::ProjectFiles &files,
        std::unique_ptr<DemoCapture> capture = DemoCapture::create(),
        el::Path executableRoot = el::Path{"cmake-build-debug/demo-apps"_el});

public: // parsing and execution
    /// Parse shell-like quoting without expansion or shell execution.
    /// @param command Author-supplied argument text.
    /// @return Parsed arguments.
    static auto arguments(const el::String &command) -> el::StringList;
    /// Resolve document-relative input files with unique basenames.
    /// @param document Document owning the files option.
    /// @param value Quoted input paths.
    /// @return Basename-to-confined-path map.
    auto resolveFiles(const el::Path &document, const el::String &value) const -> el::StringMap<el::Path>;
    /// Execute one validated command and normalize its terminal output.
    /// @param command Author-supplied executable and arguments.
    /// @param fixtures Document-relative input files indexed by basename.
    /// @param expectedExitCode Accepted ordinary exit status.
    /// @return Capture and redacted command for display.
    auto run(const el::String &command, const el::StringMap<el::Path> &fixtures, int expectedExitCode) -> DemoRunOutput;

private:
    /// Report a failure in this operation, optionally identifying its input file.
    [[noreturn]] static void throwError(const el::String &description, const el::String &path = {});

private:
    /// Resolve and validate the requested executable below the configured root.
    auto resolveExecutable(const el::StringList &parsed) const -> el::Path;
    /// Create one supported temporary fixture and return its display name.
    auto createPlaceholder(
        const el::String &kind, const el::String &content, const el::Path &temporary, int &fileIndex) const
        -> el::String;
    /// Copy one bounded project fixture into the execution directory.
    auto copyDemoFixture(const el::String &argument, const el::Path &temporary, int &fixtureIndex) const -> el::Path;
    /// Validate and expand arguments while preserving their display names.
    auto expandArguments(
        const el::StringList &parsed, const el::StringMap<el::Path> &fixtures, const el::Path &temporary) const
        -> ExpandedArguments;
    /// Bound, redact, and flatten captured terminal output.
    static auto renderOutput(el::String raw, const el::StringMap<el::String> &redactions) -> el::String;

private:
    const frame::ProjectFiles &_files;     ///< Confined project access.
    el::Path _executableRoot;              ///< Configured root for executable selection.
    std::unique_ptr<DemoCapture> _capture; ///< Native or injected capture boundary.
};

}
