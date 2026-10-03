// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../environment/EnvironmentChangesProcessor.hpp"

#include "../../../core/impl/WindowsApi.hpp"
#include "../../../text/StringList.hpp"
#include "../../SubprocessOptions.hpp"

#include <string>
#include <vector>

namespace erbsland::system::impl {

/// Own temporary Windows launch handles until reader threads accept them.
/// @tested{SubprocessInteropTest SubprocessEventTest}
class WindowsSubprocessLaunch final {
    /// Native UTF-16 environment representation for CreateProcessW.
    using Environment = EnvironmentChangesProcessor::Environment<text::U16String>;

public:
    /// Prepare empty native resource ownership.
    WindowsSubprocessLaunch() = default;
    /// Release temporary native resources not transferred to readers.
    ~WindowsSubprocessLaunch();

    // defaults/deletions
    WindowsSubprocessLaunch(const WindowsSubprocessLaunch &) = delete;
    auto operator=(const WindowsSubprocessLaunch &) -> WindowsSubprocessLaunch & = delete;

public: // launch
    /// Prepare and launch one child; this helper must be used once.
    /// @param executable Native executable path.
    /// @param arguments Child arguments.
    /// @param options Validated launch options.
    /// @return Process information with ownership of hProcess transferred to the caller; hThread is closed.
    /// @throws PlatformError If setup or launch fails.
    auto start(const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options)
        -> PROCESS_INFORMATION;

public: // accessors
    /// Get stdout pipe ownership; clear after successful transfer.
    auto outputHandle() noexcept -> HANDLE &;
    /// Get stderr pipe ownership; clear after successful transfer.
    auto errorHandle() noexcept -> HANDLE &;

private:
    /// Snapshot the native environment, including drive-current-directory entries.
    static auto inheritedEnvironment() -> Environment;
    /// Build a double-null-terminated Unicode environment block.
    static auto encodeEnvironment(const Environment &environment) -> std::vector<wchar_t>;
    /// Open only the pipes and null device required by the stream modes.
    void prepareStreams(const SubprocessOptions &options);
    /// Select child standard handles for native process creation.
    auto startupInfo(const SubprocessOptions &options) const -> STARTUPINFOW;
    /// Select a captured, discarded or inherited native output handle.
    auto outputHandle(SubprocessOutputMode mode, HANDLE writer, DWORD inherited) const -> HANDLE;
    /// Close parent copies of child handles so readers can observe EOF.
    void closeWriters() noexcept;
    /// Quote each argument according to Windows command-line escaping rules.
    static auto commandLine(const std::wstring &nativeExecutablePath, const text::StringList &arguments)
        -> std::wstring;
    /// Build the configured environment or return empty for unchanged inheritance.
    static auto environmentBlock(const SubprocessOptions &options) -> std::vector<wchar_t>;
    /// Escape one argument for the Windows native command-line boundary.
    static auto quoteArgument(const std::wstring &argument) -> std::wstring;

private:
    HANDLE _outputRead{};  ///< Parent stdout reader.
    HANDLE _outputWrite{}; ///< Child stdout writer.
    HANDLE _errorRead{};   ///< Parent stderr reader.
    HANDLE _errorWrite{};  ///< Child stderr writer.
    HANDLE _nullHandle{};  ///< Optional null device.
};

}
