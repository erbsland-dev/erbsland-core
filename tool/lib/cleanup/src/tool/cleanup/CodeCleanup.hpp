// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ClangFormat.hpp"
#include "CleanupPolicy.hpp"
#include "CleanupResult.hpp"
#include "CMakeCleanup.hpp"
#include "CppCleanup.hpp"
#include "IncludeIndex.hpp"

#include <erbsland/LogStream.hpp>
#include <tool/frame/TaskProgress.hpp>

namespace tool::cleanup {

/// Inventory once, transform independent files in parallel, then apply validated outputs.
class CodeCleanup final {
private:
    /// One cleanup input and its owning source policy.
    struct Input {
        el::Path path;      ///< Discovered source or CMake file.
        SourceTreePtr tree; ///< Owning C++ policy, or null for CMake.
    };

public:
    /// Bind the command's run-scoped services.
    /// @param policy Immutable configuration; must outlive this run.
    /// @param files Project file service; must outlive this run.
    /// @param log Destination for ordered main-thread diagnostics.
    /// @param progress Optional shared task progress.
    CodeCleanup(
        const CleanupPolicy &policy,
        const frame::ProjectFiles &files,
        el::LogStreamPtr log,
        frame::TaskProgress progress = {});

public: // execution
    /// Perform cleanup once and optionally write the results and successful format cache.
    /// @param workers Maximum number of concurrent file jobs, including clang-format processes.
    /// @param checkOnly Report changes without writing any source or cache.
    /// @param fullFormat Ignore previous formatting fingerprints.
    /// @param cmakeHeader Shared rendered copyright and license header.
    /// @return Number of changed source and CMake files.
    /// @throws el::Error If input validation, transformation, formatting, or file access fails.
    [[nodiscard]] auto run(unsigned int workers, bool checkOnly, bool fullFormat, const el::String &cmakeHeader)
        -> std::size_t;

private:
    /// Walk overlapping input and lookup roots only once and build immutable indexes.
    void inventory();
    /// Transform one C++ file and format only when its code fingerprint changed.
    auto cleanCpp(const Input &input, const CppCleanup &cpp, const ClangFormat &formatter) const -> CleanupResult;
    /// Transform one supported CMake source list or preserve the original file.
    auto cleanCMake(const Input &input, const CMakeCleanup &cmake, const el::String &header) const -> CleanupResult;
    /// Save the successful formatting baseline after all source updates.
    void writeCache(const el::String &text) const;
    /// Apply ordered results only after every transformation has succeeded.
    auto applyResults(const el::List<CleanupResult> &results, const ClangFormat &formatter, bool checkOnly)
        -> std::size_t;

private:
    frame::TaskProgress _progress;                   ///< Optional shared progress counters.
    const CleanupPolicy &_policy;                    ///< Validated policy owned by the command.
    const frame::ProjectFiles &_files;               ///< Shared confined filesystem service.
    el::LogStreamPtr _log;                           ///< Main-thread diagnostic output.
    IncludeIndex _index;                             ///< Library and consumer header lookup tables.
    el::List<Input> _inputs;                         ///< Deterministically ordered jobs.
    el::StringHashMap<el::StringList> _localSources; ///< Directory-local CMake source inventories.
};

}
