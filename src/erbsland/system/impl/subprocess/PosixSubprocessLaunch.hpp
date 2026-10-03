// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/StringList.hpp"
#include "../../SubprocessOptions.hpp"

#include <spawn.h>

#include <array>
#include <string>
#include <vector>

namespace erbsland::system::impl {

/// Own transient POSIX launch resources until reader threads accept the pipe descriptors.
/// @tested{SubprocessInteropTest SubprocessEventTest}
class PosixSubprocessLaunch final {
public:
    /// Prepare empty native resource ownership.
    PosixSubprocessLaunch() = default;
    /// Release temporary native resources not transferred to readers.
    ~PosixSubprocessLaunch();

    // defaults/deletions
    PosixSubprocessLaunch(const PosixSubprocessLaunch &) = delete;
    auto operator=(const PosixSubprocessLaunch &) -> PosixSubprocessLaunch & = delete;

public: // launch
    /// Prepare and launch a child; this helper must be used once.
    /// @param executable Native executable path.
    /// @param arguments Child arguments.
    /// @param options Validated launch options.
    /// @return Native PID.
    /// @throws PlatformError If setup or launch fails.
    auto start(const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options)
        -> int;

public: // accessors
    /// Get the output reader descriptor; set to -1 after successful transfer.
    auto outputDescriptor() noexcept -> int &;
    /// Get the error reader descriptor; set to -1 after successful transfer.
    auto errorDescriptor() noexcept -> int &;

private:
    /// Open only the pipes and null device required by the stream modes.
    void prepareStreams(const SubprocessOptions &options);
    /// Configure child descriptors and working directory in the spawn action list.
    void prepareActions(const SubprocessOptions &options);
    /// Add a child descriptor duplication action.
    void redirect(int source, int destination);
    /// Configure one captured, discarded or inherited output stream.
    void configureOutput(SubprocessOutputMode mode, const std::array<int, 2> &pipe, int destination);
    /// Add the optional native child directory change.
    void configureWorkingDirectory(const SubprocessOptions &options);
    /// Materialize native argument storage and its null-terminated pointer array.
    void prepareArguments(const path::Path &executable, const text::StringList &arguments);
    /// Apply inherited assignments and removals, then materialize native envp.
    void prepareEnvironment(const SubprocessOptions &options);
    /// Create a parent-reader/child-writer pipe or throw a native diagnostic.
    static void createPipe(std::array<int, 2> &pipe);
    /// Throw a native diagnostic for a failed POSIX spawn operation.
    static void checkAction(int result, const text::String &reason);
    /// Close and invalidate one parent copy of a child writer.
    static void closeWriter(int &descriptor) noexcept;

private:
    std::array<int, 2> _outputPipe{-1, -1};       ///< Temporary stdout pipe.
    std::array<int, 2> _errorPipe{-1, -1};        ///< Temporary stderr pipe.
    int _nullDescriptor{-1};                      ///< Optional null device.
    posix_spawn_file_actions_t _actions{};        ///< Native child setup actions.
    bool _actionsInitialized{};                   ///< Whether actions require destruction.
    std::vector<std::string> _argumentStorage;    ///< Native argument bytes.
    std::vector<char *> _argumentPointers;        ///< Native argv pointers.
    std::vector<std::string> _environmentStorage; ///< Native environment bytes.
    std::vector<char *> _environmentPointers;     ///< Native environment pointers.
};

}
