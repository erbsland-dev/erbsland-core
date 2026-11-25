// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CommandOutput.hpp"

#include <erbsland/ByteCount.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/StringList.hpp>

namespace tool::frame {

/// Resolve a configured executable in trusted directories and launch it without a shell.
class ExternalCommand final {
    /// Maximum retained bytes per output stream before the command result becomes unusable.
    static constexpr auto cCaptureLimit = el::ByteCount{8 * 1024 * 1024};

public: // configuration
    /// Validate absolute trusted directories, allowing entries absent on this host.
    /// @param value Validated list of directory strings.
    /// @return Physical existing directories and lexical missing directories.
    /// @throws el::conf::ConfError If a directory entry is invalid.
    [[nodiscard]] static auto directories(const el::conf::ValuePtr &value) -> el::PathList;
    /// Resolve an executable once, retaining its physical path for subsequent calls.
    /// @param value Executable basename or absolute path.
    /// @param directories Allowed physical directory roots.
    /// @param projectRoot Project directory, which must never supply an executable.
    /// @param required Fail if no trusted executable is available.
    /// @throws el::conf::ConfError If the setting is invalid or a required command is unavailable.
    void configure(
        const el::conf::ValuePtr &value,
        const el::PathList &directories,
        const el::Path &projectRoot,
        bool required = true);

public: // execution
    /// Test whether configuration found a trusted executable.
    [[nodiscard]] auto isAvailable() const noexcept -> bool { return !_executable.isEmpty(); }
    /// Execute with closed stdin, a restricted environment, and bounded capture.
    /// @param arguments Individual arguments, without shell quoting.
    /// @param workingDirectory Child working directory.
    /// @return Complete output and status; nonzero exit codes are interpreted by the caller.
    /// @throws el::ApplicationError If launch fails or output is truncated.
    [[nodiscard]] auto run(const el::StringList &arguments, const el::Path &workingDirectory) const -> CommandOutput;

public: // direct execution
    /// Launch an already resolved executable using the shared restricted environment and capture policy.
    /// @param executable Absolute executable selected by the caller.
    /// @param arguments Individual arguments, without shell quoting.
    /// @param workingDirectory Child working directory.
    /// @param removeOutputBom Whether to remove the initial UTF-8 signature from captured streams.
    /// @return Complete output and native exit status.
    /// @throws el::ApplicationError If launching or collecting output fails.
    [[nodiscard]] static auto runExecutable(
        const el::Path &executable,
        const el::StringList &arguments,
        const el::Path &workingDirectory,
        bool removeOutputBom = true) -> CommandOutput;

private:
    el::Path _executable; ///< Physical trusted executable, empty for an unavailable optional command.
};

}
