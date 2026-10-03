// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CleanupPolicy.hpp"
#include "IncludeIndex.hpp"

#include <optional>

namespace tool::cleanup {

/// Normalize the deliberately small, directory-local CMake source-list grammar.
class CMakeCleanup final {
private:
    /// One supported command with its already split arguments.
    struct Command {
        el::String name;                          ///< Lowercase command name.
        el::StringList arguments;                 ///< Plain whitespace-separated arguments.
        el::ItemIndex end{el::ItemIndex::zero()}; ///< One past the last physical line.
    };
    /// Per-file parse result; no mutable state is shared between workers.
    struct Document {
        el::Path directory;          ///< Directory whose local sources are being maintained.
        el::String target;           ///< Configured source-list target.
        el::StringList directories;  ///< Validated subdirectories.
        el::StringList mainFiles;    ///< Unconditional source filenames.
        el::StringHashSet covered;   ///< Filenames already owned by a source list.
        el::String invalidFile;      ///< First missing or nonlocal filename, reported only for supported files.
        el::String invalidDirectory; ///< First missing subdirectory, reported only for supported files.
        el::StringList suffix;       ///< Preserved conditional lines with normalized source commands.
    };

public:
    /// Bind repository policy and discovered regular files.
    /// @param policy Immutable configuration; must outlive this cleaner.
    /// @param index Immutable file inventory; must outlive this cleaner.
    CMakeCleanup(const CleanupPolicy &policy, const IncludeIndex &index);

public: // cleanup
    /// Normalize an eligible CMake file, or return no value for unsupported syntax.
    /// @param path Absolute CMakeLists.txt path.
    /// @param text Original text.
    /// @param localFiles Source filenames in this directory.
    /// @param header The shared generated-file copyright and license lines.
    /// @return Normalized text, or no value when preserving a complex file.
    [[nodiscard]] auto apply(
        const el::Path &path, const el::String &text, const el::StringList &localFiles, const el::String &header) const
        -> std::optional<el::String>;

private:
    /// Parse one simple command starting at a physical line.
    static auto command(const el::StringList &lines, el::ItemIndex start) -> std::optional<Command>;
    /// Parse one command into a document; reject grammar that could change CMake semantics.
    auto accept(Document &document, const Command &command, bool conditional) const -> bool;
    /// Parse and validate source-list arguments, recording ownership once.
    auto sources(Document &document, const Command &command, bool conditional) const -> bool;
    /// Parse the complete file, preserving conditional control-flow lines.
    auto parse(Document &document, const el::StringList &lines) const -> bool;
    /// Render the validated document and add newly discovered local files.
    auto renderDocument(Document document, const el::StringList &localFiles, const el::String &header) const
        -> el::String;
    /// Render a sorted, deduplicated private source list.
    static auto render(const el::String &target, const el::StringList &files, const el::String &indent = {})
        -> el::String;
    /// Validate a filename in a target_sources command.
    auto validFile(const el::Path &directory, const el::String &name) const -> bool;

private:
    const CleanupPolicy &_policy; ///< CMake mappings and version.
    const IncludeIndex &_index;   ///< Regular file inventory.
};

}
