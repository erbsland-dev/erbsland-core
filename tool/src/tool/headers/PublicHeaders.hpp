// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HeaderDeclarations.hpp"
#include "HeaderOutput.hpp"
#include "HeaderPolicy.hpp"
#include "HeaderRenderJob.hpp"
#include "PublicHeader.hpp"

#include "impl/BinaryStringCompare.hpp"

#include "../frame/ProjectFiles.hpp"
#include "../frame/TaskProgress.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/LogStream.hpp>
#include <erbsland/Map.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/Set.hpp>
#include <erbsland/system/SystemInfo.hpp>
#include <erbsland/WorkerPool.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace tool::headers {

using namespace el::text::literals;

/// Build and apply a fully validated public-header generation plan.
class PublicHeaders final {
    /// Renderer key listing the headers included by generated wrappers and umbrellas.
    constexpr static auto cIncludesKey = "includes"_el;

    /// An ordered internal path index using byte comparison.
    template <typename T>
    using FileMap = el::util::Map<el::String, T, impl::BinaryStringCompare>;
    /// A set of internal path spellings using byte comparison.
    using FileSet = el::util::Set<el::String, impl::BinaryStringCompare>;

public:
    /// Bind a policy to the application's project file service.
    /// @param policy Validated generation policy.
    /// @param files Project-scoped file access.
    /// @param log The owning tool's log stream.
    /// @param workers Positive read, parsing and rendering concurrency limit.
    /// @param progress Optional task progress.
    PublicHeaders(
        HeaderPolicy policy,
        const frame::ProjectFiles &files,
        el::LogStreamPtr log,
        unsigned int workers = el::system::info::logicalCpuCount(),
        frame::TaskProgress progress = {});

public: // generation
    /// Discover inputs, validate all relationships, and render every output without writing.
    void prepare();
    /// Apply the prepared plan, or report whether it differs without changing files.
    /// @param checkOnly Whether to suppress writes and deletions.
    /// @return Number of changed or obsolete output files.
    [[nodiscard]] auto apply(bool checkOnly) const -> std::size_t;

private:
    /// Inventory both source and wrapper trees once and reject uncovered directories.
    void discover(el::WorkerPool &workers);
    /// Inventory one tree for a bulk read and check source directory coverage.
    void inventory(
        const el::Path &base,
        bool sourceTree,
        el::StringSet &seenDirectories,
        el::StringList &uncovered,
        std::vector<std::pair<el::Path, bool>> &inputs);
    /// Register a selected source header and report whether its declarations are needed.
    [[nodiscard]] auto selectInput(const el::Path &path, const el::Path &relative) -> bool;
    /// Test a namespace prefix at a namespace-component boundary without allocating a prefix.
    [[nodiscard]] static auto isChildNamespace(const el::String &name, const el::String &parent) -> bool;
    /// Verify selected declarations and resolve all explicit namespace exports.
    void validateHeaders(el::WorkerPool &workers);
    /// Parse canonical forward headers needed by the discovered inputs.
    void scanForwardHeaders(el::WorkerPool &workers);
    /// Validate namespace placement in one primary or forward header.
    static void validateNamespaces(
        const HeaderDeclarations &scan, const DirectoryPolicy &directory, const el::String &relative);
    /// Register selected exports and choose their safe forward declarations.
    auto selectExports(
        const HeaderDeclarations &declarations,
        const HeaderDeclarations &forward,
        const DirectoryPolicy &directory,
        const el::String &relative,
        const el::Path &forwardPath) -> bool;
    /// Inspect one source header and its canonical forward header from the inventory.
    void inspectHeader(const el::Path &path, const DirectoryPolicy &directory);
    /// Recover configured and existing compatibility wrappers from the inventory.
    void preserveAliases();
    /// Plan complete domain and organizational subgroup umbrellas.
    void createUmbrellas();
    /// Plan namespace declarations, forward declarations and consumer exports.
    void createNamespaces();
    /// Resolve canonical and alias paths, rejecting unintended merges.
    void createWrappers();
    /// Plan canonical-first source-to-include mapping rows.
    void createMapping();
    /// Render all planned templates after their relationships are validated.
    void renderOutputs(el::WorkerPool &workers);
    /// Find managed wrappers and source umbrellas absent from the new plan.
    void collectObsolete();
    /// Validate ownership, confinement and portable path collisions before writing.
    void validatePlan();
    /// Validate one destination and its existing ownership without repeating parent checks.
    void validateOutput(
        const el::String &name, const HeaderOutput &output, const FileSet &managedOutputs, FileSet &checkedParents);
    /// Reject case collisions and destinations that are also parent directories.
    void validatePathCollisions(const el::Path &path, FileMap<el::String> &portablePaths);
    /// Enqueue an output template and reject duplicate destinations.
    void addOutput(const el::Path &path, const el::String &layout, el::text::render::Context context);
    /// Resolve an output lexically, rejecting escapes and the project root.
    [[nodiscard]] auto outputPath(const el::Path &path) const -> el::Path;
    /// Check confinement and every existing ancestor of a proposed output.
    [[nodiscard]] auto checkedOutput(const el::Path &path) const -> el::Path;
    /// Create checked output ancestors when applying a validated plan.
    void ensureDirectory(const el::Path &path) const;
    /// Recognize generated markers in the initial banner, within the first ten lines.
    [[nodiscard]] auto generated(const el::String &text) const -> bool;

private:
    frame::TaskProgress _progress;                  ///< Shared phase counters.
    HeaderPolicy _policy;                           ///< Validated directory and export configuration.
    const frame::ProjectFiles &_files;              ///< Confined project access.
    el::LogStreamPtr _log;                          ///< Owning tool's log stream.
    unsigned int _workerCount;                      ///< Maximum independent maintenance jobs.
    FileMap<frame::ProjectTextFile> _existingFiles; ///< Text snapshots and reusable paths from the inventory.
    el::Path _sourceBase;                           ///< Absolute source root.
    el::Path _includeBase;                          ///< Absolute wrapper root.
    el::StringMap<el::String> _inputs;              ///< Discovered headers and their configured directories.
    FileMap<std::shared_ptr<const HeaderDeclarations>> _scans; ///< Worker-parsed source and forward declarations.
    el::StringMap<PublicHeader> _headers;                      ///< Validated publication relationships.
    FileMap<HeaderRenderJob> _renderJobs;                      ///< Template names and structured rendering data.
    FileMap<HeaderOutput> _outputs;                            ///< Complete rendered output, indexed by absolute path.
    FileMap<el::Path> _obsolete;                               ///< Recognized generated files absent from the plan.
    el::StringSet _forwardIncludes;                    ///< Canonical forward headers required by selective exports.
    el::StringMap<el::StringSet> _forwardDeclarations; ///< Extracted declarations grouped by namespace.
    el::StringMap<el::StringSet> _selectedExports;     ///< Resolved explicit exports grouped by namespace.
    el::text::render::EnvironmentPtr _renderer;        ///< Shared rendering environment for the output stage.
};

}
