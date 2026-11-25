// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProjectTextFile.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/conf/Document.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/OptionValue.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathType.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringBomMode.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/SymlinkMode.hpp>

#include <optional>

namespace tool::frame {

/// Confine all module files and configuration sources to the validated project root.
/// @tested{ProjectFilesTest}
class ProjectFiles final {
public:
    /// Create a file service for a root returned by ProjectRoot.
    /// @param projectRoot Validated physical project directory.
    explicit ProjectFiles(el::Path projectRoot);

public: // accessors
    /// Get the validated project directory.
    [[nodiscard]] auto projectRoot() const noexcept -> const el::Path & { return _projectRoot; }

public: // files
    /// Create confined directories and any missing parents, retaining existing directories.
    /// @param directory Project-relative or absolute output directory.
    /// @throws el::ApplicationError If creation fails or the path leaves the project or traverses a link.
    void createDirectories(const el::Path &directory) const;
    /// Collect filenames for inventory or name checks, without retaining a walk's trusted metadata cache.
    /// Use PathWalker directly and read inside its callback when file contents are needed.
    /// @param target Project-relative or absolute file or directory.
    /// @param suffix Required file suffix, including the dot.
    /// @param recursive Include nested directories.
    /// @return Sorted absolute paths to matching regular files, skipping links.
    /// @throws el::ApplicationError If the target is invalid, has the wrong suffix, or cannot be walked.
    [[nodiscard]] auto collectFilenamesOnly(
        const el::Path &target, const el::String &suffix, bool recursive = true) const -> el::PathList;
    /// Resolve a path lexically within the project, without accessing the filesystem.
    /// @param path Project-relative or absolute path.
    /// @return The confined absolute path; filesystem access must still reject links.
    /// @throws el::ApplicationError If the path is empty or leaves the project.
    [[nodiscard]] auto resolve(const el::Path &path) const -> el::Path;
    /// Read tolerant UTF-8 with metadata that can detect changes before reusing the contents.
    /// @param path Requested regular file.
    /// @return An immutable text snapshot with automatic BOM handling and no UTF-8 validation pass.
    /// @throws el::ApplicationError On access or confinement errors.
    /// @throws el::EncodingError If an initial BOM declares a different encoding.
    [[nodiscard]] auto readTextFile(const el::Path &path) const -> ProjectTextFile;
    /// Read an entry discovered by a walk that skips links below a validated project directory.
    /// Parent directories are trusted for this read; concurrent replacement is not guarded against.
    /// Queued workers may read after the callback returns, within the same inventory run.
    /// @param path Retained regular-file path reported by the walk.
    /// @return An immutable tolerant UTF-8 snapshot with automatic BOM handling.
    /// @throws el::ApplicationError On access or lexical containment errors.
    /// @throws el::EncodingError If an initial BOM declares a different encoding.
    [[nodiscard]] auto readWalkedTextFile(const el::Path &path) const -> ProjectTextFile;
    /// Check fresh metadata and compare generated text against a previously read file.
    /// @param file Snapshot obtained from readTextFile().
    /// @param text Proposed generated text.
    /// @return True if the file still matches the snapshot and the text is equivalent.
    [[nodiscard]] auto isUnchanged(const ProjectTextFile &file, const el::String &text) const -> bool;
    /// Resolve a root-relative or absolute path, rejecting escapes and links before accessing contents.
    /// @param path Requested path.
    /// @param allowMissingParents Allow a missing suffix when planning new output directories.
    /// @return The checked absolute path; a missing final file is allowed.
    /// @throws el::ApplicationError If the path leaves the root or traverses a link or non-directory.
    auto checked(const el::Path &path, bool allowMissingParents = false) const -> el::Path;
    /// Resolve a configured path while retaining the setting in validation diagnostics.
    /// @param value Text configuration value containing the path.
    /// @param allowMissingParents Allow missing output directories.
    /// @return The confined absolute path.
    /// @throws el::conf::ConfError If the configured path cannot be used.
    auto checked(const el::conf::ValuePtr &value, bool allowMissingParents = false) const -> el::Path;
    /// Resolve an option path while retaining the option in validation diagnostics.
    /// @param value Parsed text option containing the path.
    /// @param allowMissingParents Allow missing output directories.
    /// @return The confined absolute path.
    /// @throws el::OptionError If the option path cannot be used.
    auto checked(const el::OptionValuePtr &value, bool allowMissingParents = false) const -> el::Path;
    /// Resolve a configured path and validate its intended filesystem type before execution.
    /// @param value Configuration value containing the path.
    /// @param type Required file or directory type.
    /// @param allowMissing Allow a new final entry in an existing parent directory.
    /// @return Confined path with the requested type or permitted missing final entry.
    /// @throws el::conf::ConfError If the path cannot satisfy the requirement.
    [[nodiscard]] auto checked(const el::conf::ValuePtr &value, el::PathType type, bool allowMissing = false) const
        -> el::Path;
    /// Resolve an option path and validate its intended filesystem type before execution.
    /// @param value Parsed option containing the path.
    /// @param type Required file or directory type.
    /// @param allowMissing Allow a new final entry in an existing parent directory.
    /// @return Confined path with the requested type or permitted missing final entry.
    /// @throws el::OptionError If the path cannot satisfy the requirement.
    [[nodiscard]] auto checked(const el::OptionValuePtr &value, el::PathType type, bool allowMissing = false) const
        -> el::Path;
    /// Read a checked regular text file as tolerant UTF-8.
    /// @param path Requested path.
    /// @return UTF-8 text with automatic BOM handling and no validation pass.
    /// @throws el::ApplicationError On access or confinement errors.
    /// @throws el::EncodingError If an initial BOM declares a different encoding.
    [[nodiscard]] auto readText(const el::Path &path) const -> el::String;
    /// Read a checked regular binary file.
    /// @param path Requested path.
    /// @return Exact file bytes.
    /// @throws el::ApplicationError On access or confinement errors.
    [[nodiscard]] auto readData(const el::Path &path) const -> el::ByteBlock;
    /// Hash a confined text file independently of CRLF, LF, or CR line endings.
    /// @param path The regular UTF-8 input file; its BOM and other bytes remain significant.
    /// @param algorithm The digest algorithm.
    /// @return The binary digest.
    /// @throws el::ApplicationError If the file cannot be read safely.
    [[nodiscard]] auto textChecksum(const el::Path &path, el::HashAlgorithm algorithm) const -> el::ByteBlock;
    /// Parse a checked configuration with confined, nonrecursive path-glob includes.
    /// @param path Configuration file.
    /// @param includes Enable project-contained includes.
    /// @param variables Optional variables enabling built-in placeholder sources and filters.
    /// @return The parsed document.
    /// @throws el::ApplicationError On parsing, access, or confinement errors.
    [[nodiscard]] auto readConfiguration(
        const el::Path &path,
        bool includes = true,
        const std::optional<el::StringMap<el::String>> &variables = {}) const -> el::conf::DocumentPtr;
    /// Replace a changed regular file through a same-directory temporary file.
    /// @param path Output path, whose parent must exist.
    /// @param text Complete generated output.
    /// @param checkOnly Report differences without writing.
    /// @param bomMode Select whether the UTF-8 output starts with a byte order mark.
    /// @return True if the output differs from the existing file.
    /// @throws el::ApplicationError On writing or confinement errors; existing output is preserved on failure.
    auto updateText(
        const el::Path &path,
        const el::String &text,
        bool checkOnly = false,
        el::StringBomMode bomMode = el::StringBomMode::Automatic) const -> bool;

private:
    /// Check confinement and the expected filesystem type for a user-supplied path.
    [[nodiscard]] auto checkedType(const el::Path &path, el::PathType type, bool allowMissing) const -> el::Path;
    /// Capture text and metadata with the selected native open policy.
    [[nodiscard]] auto readTextFile(const el::Path &path, el::SymlinkMode symlinkMode) const -> ProjectTextFile;
    /// Read a confined regular file, rejecting links at the native open boundary.
    [[nodiscard]] auto readCheckedText(const el::Path &path) const -> el::String;
    /// Report a project file access or confinement failure.
    /// @param path The requested project path.
    /// @param description The reason access failed.
    [[noreturn]] void throwFileError(const el::Path &path, el::String description) const;
    /// Compare lines while ignoring terminators and first-line copyright years.
    /// @param left The existing file contents.
    /// @param right The proposed generated text.
    /// @return True if all meaningful content is equivalent.
    [[nodiscard]] static auto equivalentText(const el::String &left, const el::String &right) -> bool;
    /// Mask the year in a first-line copyright notice.
    /// @param line The first physical line without its terminator.
    /// @return The notice with its recognized year masked, or the unchanged line.
    [[nodiscard]] static auto normalizedCopyright(const el::String &line) -> el::String;
    /// Require an existing, confined regular input file.
    /// @param path The checked file path.
    /// @throws el::ApplicationError If the file is missing or is not regular.
    void requireRegularFile(const el::Path &path) const;

private:
    el::Path _projectRoot; ///< Validated physical project root.
};

}
