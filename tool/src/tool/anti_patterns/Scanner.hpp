// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Finding.hpp"
#include "FindingCache.hpp"
#include "Policy.hpp"
#include "RuleRegistry.hpp"

#include "../frame/ProjectFiles.hpp"
#include "../frame/TaskProgress.hpp"

#include <erbsland/ItemCount.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Path.hpp>

namespace tool::anti_patterns {

/// Inventory selected sources, scan changed files concurrently, and merge ordered findings.
class Scanner final {
    /// One source file and its inventory-time metadata.
    struct Job final {
        el::Path absolute;    ///< Confined source path.
        el::Path relative;    ///< Project-relative source path.
        el::String signature; ///< Metadata before worker read.
    };
    /// One worker scan and its post-read metadata.
    struct Result final {
        el::List<Finding> findings; ///< Source-level findings.
        el::String signature;       ///< Metadata after worker read.
    };

public:
    /// Bind validated policy, detectors, and confined file access.
    /// @param policy Validated scan policy.
    /// @param rules Configured detectors.
    /// @param files Project file service.
    /// @param progress Optional shared task progress.
    Scanner(
        const Policy &policy,
        const RuleRegistry &rules,
        const frame::ProjectFiles &files,
        frame::TaskProgress progress = {});

public: // execution
    /// Scan configured roots or explicit project-relative targets.
    [[nodiscard]] auto scan(
        const el::StringList &requested, unsigned int workers, const el::OptionValuePtr &requestedOption = {})
        -> el::List<Finding>;

public: // statistics
    /// Get the number of reused source results.
    [[nodiscard]] auto cacheHits() const noexcept -> el::ItemCount { return _cacheHits; }
    /// Get the number of scanned source files.
    [[nodiscard]] auto cacheMisses() const noexcept -> el::ItemCount { return _cacheMisses; }
    /// Get the number of selected source files.
    [[nodiscard]] auto fileCount() const noexcept -> el::ItemCount { return _fileCount; }

private:
    /// Walk roots and select supported files in deterministic order.
    [[nodiscard]] auto inventory(const el::StringList &requested, const el::OptionValuePtr &requestedOption) const
        -> el::List<Job>;
    /// Read and detect all rules against one source snapshot.
    [[nodiscard]] auto scanFile(const Job &job) const -> Result;

private:
    frame::TaskProgress _progress;     ///< Shared file counters, including cache hits.
    const Policy &_policy;             ///< Validated scan policy.
    const RuleRegistry &_rules;        ///< Immutable detector registry.
    const frame::ProjectFiles &_files; ///< Shared confined input service.
    el::ItemCount _cacheHits{};        ///< Reused file results.
    el::ItemCount _cacheMisses{};      ///< Scanned file results.
    el::ItemCount _fileCount{};        ///< Selected source files.
};

}
