// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Path_fwd.hpp"
#include "PathContent_fwd.hpp"
#include "PathFormat.hpp"
#include "PathInfo_fwd.hpp"
#include "PathInfoParts.hpp"
#include "PathOperations_fwd.hpp"
#include "PathResolveOptions.hpp"
#include "PathWalker_fwd.hpp"
#include "PathWindowsFormat.hpp"

#include "impl/PathData.hpp"

#include "../text/CharCompareFn.hpp"
#include "../text/String.hpp"
#include "../text/StringList.hpp"
#include "../unit/ElementCount.hpp"
#include "../unit/ElementIndex.hpp"
#include "../unit/ElementRange.hpp"
#include "../util/impl/ComparisonHelper.hpp"
#include "../util/List.hpp"

#include <filesystem>

namespace erbsland::path {

using PathList = util::List<Path>;

/// The convenience API to work with the filesystem.
///
/// Represents a relative, absolute, or partial path in the file system.
/// - Paths work with the slash ('/') character as a path element separator on all platforms.
/// - Separators are converted automatically where needed.
/// - Path itself is operating system agnostic and especially handles drives (like `c:/`), UNC paths
///   (like `//server/share/`) and POSIX roots like `/` on all platforms.
/// - There are `fromPosix()` and `fromWindows()` methods to construct paths from a defined format
///   if you want to make sure `c:/xyz` is interpreted as a relative Posix path.
///
/// Special cases - invalid paths vs. current directory:
/// - Empty paths are invalid, and operations on empty paths fail as documented.
/// - Paths with a single dot (`.`) are interpreted as the current directory and are valid.
///
/// Hard Limits:
/// - Independently of the operating system, path strings with more than 8k characters or more than 1k path elements
///   are rejected and turned into empty paths.
///
/// Windows-specific notes:
/// - Windows roots are normalized: Drive letters are stored lower-case, like `c:/`, server names, or IP
///   addresses in UNC paths are stored lower-case, like `//server/Share`.
/// - Absolute paths without drive letters are not supported as they require the current working directory.
///   They are converted into absolute POSIX paths.
/// - Only on the Windows platform:
///   Windows extended length paths are converted into its normalized form.
///   Like "//?/C:/" becomes "c:/" and "//?/UNC/server/Share" becomes "//server/Share".
/// - Only on the Windows platform:
///   Special Windows paths, like "/??/", "//./", "//?/Volume" and "//./PhysicalDrive0", etc. are not supported.
///   Any path that looks like a special path is converted into an empty path on construction for security reasons.
/// @seedoc{/topics/path/working_with_paths}
/// @tested{PathConstructionTest PathAccessTest PathModificationTest PathConversionTest PathResolveBackendTest}
class Path final {
    friend class PathInfo;
    friend class impl::PathBackend;

public:
    /// Creates an empty, invalid path.
    Path() = default;

    /// Convert a text into a path object.
    /// A call of this constructor always succeeds, even for malformed or invalid paths.
    /// Validate the path for the current platform using `isValid()` after construction.
    /// @param path The path to convert.
    explicit Path(const text::String &path) noexcept;

    /// Convert a path from the standard library.
    /// @param path The path to convert.
    explicit Path(const std::filesystem::path &path) noexcept;

    // defaults
    Path(const Path &) = default;
    Path(Path &&) = default;
    auto operator=(const Path &) -> Path & = default;
    auto operator=(Path &&) -> Path & = default;

public: // operators
    /// Compare two paths case-sensitively.
    auto operator<=>(const Path &other) const noexcept -> std::strong_ordering { return compare(other); }
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const Path &other, other);

    /// Join two paths (see `joined`)
    auto operator/(const Path &other) const -> Path;
    /// Join two paths (see `joined`)
    auto operator/(const text::String &other) const -> Path;
    /// Join two paths (see `join`)
    auto operator/=(const Path &other) -> Path &;
    /// Join two paths (see `join`)
    auto operator/=(const text::String &other) -> Path &;

public: // tests
    /// Test if this path is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Test if this path is valid for the current platform.
    /// - Empty paths are invalid paths.
    /// - If the number of elements in the path is in a valid range.
    /// - If the path elements are valid for the current platform.
    [[nodiscard]] auto isValid() const noexcept -> bool;
    /// Test if this path is relative
    [[nodiscard]] auto isRelative() const noexcept -> bool;
    /// Test if this path is absolute.
    /// A path is absolute if it contains a root element.
    [[nodiscard]] auto isAbsolute() const noexcept -> bool;
    /// Test if this is a root path.
    /// @return true, if the path is absolute and contains one root element.
    [[nodiscard]] auto isRoot() const noexcept -> bool;

    /// Compare two paths.
    /// @param other The other path for comparison.
    /// @param compareFn The character comparison function to use.
    [[nodiscard]] auto compare(const Path &other, text::CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;

public: // path element accessors
    /// Get the format of this path.
    [[nodiscard]] auto format() const noexcept -> PathFormat;
    /// Get the number of path elements.
    [[nodiscard]] auto elementCount() const noexcept -> unit::ElementCount;
    /// Access a single element of the path.
    /// @param index The index of the element to access.
    /// @return The element at the given index or an empty string if the index is out of range.
    [[nodiscard]] auto element(unit::ElementIndex index) const noexcept -> text::String;
    /// Access the individual path elements.
    [[nodiscard]] auto elements() const noexcept -> text::StringList;
    /// Get the parent path.
    /// A call of this method returns this path without the last element.
    /// An empty path is returned if this path is empty.
    /// No validation is performed.
    [[nodiscard]] auto parent() const noexcept -> Path;
    /// Get all parent paths.
    /// The returned paths are ordered from the closest to the furthest parent.
    [[nodiscard]] auto parents() const noexcept -> PathList;
    /// Get the root element of this path.
    /// If the path is relative, an empty string is returned.
    /// Root elements for Windows always use the slash (`/`) path separator, even for UNC paths.
    /// Drives always use a lower-case letter, like `c:/`.
    /// Server names/IP-Addresses in UNC paths are always lower-case, like `//example/Share`.
    [[nodiscard]] auto root() const noexcept -> text::String;
    /// Get the name of the last element of this path.
    /// For a file, this is the filename, for a directory, this is the directory name.
    /// @return The name of the last element of this path, or an empty string if the path is empty.
    [[nodiscard]] auto name() const noexcept -> text::String;
    /// Get the suffix for this path.
    /// - If `name` is `example.txt`, `.txt` is returned.
    /// - If `name` is `example.tar.gz`, `.gz` is returned.
    /// - If `name` is `example`, an empty string is returned.
    /// - If `name` *starts* with a `.`, this is *not* considered a suffix.
    /// - If `name` *ends* with a `.`, this is considered a (empty) suffix (e.g. `name.`)
    [[nodiscard]] auto suffix() const noexcept -> text::String;
    /// Get all suffixes for this path.
    /// - If `name` is `example.txt`, `.txt` is returned.
    /// - If `name` is `example.tar.gz`, `.tar.gz` is returned.
    /// - If `name` is `example`, an empty string is returned.
    /// - If `name` *starts* with a `.`, this is *not* considered a suffix.
    /// - If `name` *ends* with a `.`, this is considered a (empty) suffix (e.g. `name.`)
    [[nodiscard]] auto suffixes() const noexcept -> text::String;
    /// Get the name, without any suffixes.
    /// - If `name` is `example`, `example` is returned.
    /// - If `name` is `example.tar.gz`, `example` is returned.
    /// - If `name` is `.hidden`, `.hidden` is returned.
    [[nodiscard]] auto stem() const noexcept -> text::String;

public: // common path tools
    /// Return this path with the last path element replaced.
    /// @param name The name to use. Like `example.txt`.
    /// @return The new path.
    [[nodiscard]] auto withName(const text::String &name) const noexcept -> Path;
    /// Return this path with *all* suffixes replaced.
    /// @param replacement The replacement to use. Like `.txt`. Can be empty to remove all suffixes.
    /// @return The new path.
    [[nodiscard]] auto withSuffix(const text::String &replacement) const noexcept -> Path;
    /// Return this path with the stem replaced.
    /// @param replacement The replacement to use. Like `example`.
    /// @return The new path
    [[nodiscard]] auto withStem(const text::String &replacement) const noexcept -> Path;

public: // path element tools
    /// Join one or more path elements.
    /// - If `other` on the right is an absolute path, it is added as a relative path without its root.
    /// - If `other` is a string, it is converted into a path, then joined.
    /// - No validation is performed on the resulting path.
    /// @param other The path to join.
    auto join(const Path &other) noexcept -> Path &;
    /// @overload
    auto join(const text::String &other) noexcept -> Path &;
    /// Join one or more path elements.
    /// - If `other` on the right is an absolute path, it is added as a relative path without its root.
    /// - If `other` is a string, it is converted into a path, then joined.
    /// - No validation is performed on the resulting path.
    /// @param other The path to join.
    [[nodiscard]] auto joined(const Path &other) const noexcept -> Path;
    /// @overload
    [[nodiscard]] auto joined(const text::String &other) const noexcept -> Path;
    /// Get a slice of this path.
    /// @param range The range of elements to return.
    /// @return A new path containing the specified elements, clamped to the available tail. Returns empty if the start
    /// index is out of bounds.
    [[nodiscard]] auto slice(unit::ElementRange range) const noexcept -> Path;
    /// Split the path after a given number of elements.
    /// For an absolute path, if the front contains a root, the front is absolute.
    /// There are several special cases:
    /// - If this path is empty, both returned paths are empty too.
    /// - In case `count` is zero, front is an invalid empty path, and back contains the whole path.
    /// - In case `count` is larger than element count or infinite, front contains the whole path
    /// and back contains a `.` path.
    /// @param count The number of front elements for the split.
    /// @return The front and back part of the path.
    [[nodiscard]] auto splitAfter(unit::ElementCount count) const noexcept -> std::pair<Path, Path>;

public: // path resolving
    /// Resolve this path into a canonical absolute path.
    /// Depending on the flags, it resolves any symlinks, "." and ".." elements from the path.
    /// @param options The options to use.
    /// @return The resolved path or an empty path on error (or throw on error).
    [[nodiscard]] auto resolve(PathResolveOptions options = {}) const noexcept -> Path;
    /// @overload
    [[nodiscard]] auto resolveOrThrow(PathResolveOptions options = {}) const -> Path;
    /// Convert this path to an absolute path.
    /// If the path is already absolute, it is returned unchanged.
    /// If the path is `.`, the current directory is returned.
    /// Errors: If the path is empty, or if a passed `currentDirectory` is empty or a relative path.
    /// @param base The base directory to use. If not specified, the current working directory is used.
    /// @return An absolute path or an empty path on error (or throw on error).
    [[nodiscard]] auto toAbsolute(std::optional<Path> base = std::nullopt) const noexcept -> Path;
    /// @overload
    [[nodiscard]] auto toAbsoluteOrThrow(std::optional<Path> base = std::nullopt) const -> Path;
    /// Convert this path to a relative path.
    /// Tries to convert an absolute path into a relative path to `directory`.
    /// If the path is already relative, it is returned unchanged.
    /// Errors: If the path is empty, or if a passed `directory` is empty, a relative path or does not have a
    /// common ancestor with this path.
    /// @param base The base directory to use. If not specified, the current working directory is used.
    /// @return A relative path or an empty path on error (or throw on error).
    [[nodiscard]] auto toRelative(std::optional<Path> base = std::nullopt) const noexcept -> Path;
    /// @overload
    [[nodiscard]] auto toRelativeOrThrow(std::optional<Path> base = std::nullopt) const -> Path;
    /// Tests if this path is relative to the given base.
    /// Always returns `Result::Success` if this path is a relative path.
    /// @param base The base directory to use. If not specified, the current working directory is used.
    /// @return `Result::Success` if this path is relative to the given base, `Result::Failure` otherwise.
    [[nodiscard]] auto isRelativeTo(std::optional<Path> base = std::nullopt) const noexcept -> bool;
    /// Get the common ancestor with the given path.
    /// @param base The base directory to use. If not specified, the current working directory is used.
    /// @return The common ancestor path or an empty path if no common ancestor exists.
    [[nodiscard]] auto commonAncestor(std::optional<Path> base = std::nullopt) const noexcept -> Path;

public: // components
    /// Access information about the file or directory of this path.
    /// Repeated calls on this path or one of its copies share the attached information cache.
    /// @param parts The parts to initially request and cache.
    [[nodiscard]] auto info(PathInfoParts parts = PathInfoPart::Default) const noexcept -> PathInfo;
    /// Access the path walker component for this path.
    [[nodiscard]] auto walker() const noexcept -> PathWalker;
    /// Access the content of the path.
    [[nodiscard]] auto content() const -> PathContent;
    /// Access the path operations.
    [[nodiscard]] auto operations() const noexcept -> PathOperations;

public: // conversion
    /// Convert this path to a standard library path.
    [[nodiscard]] auto toStdPath() const noexcept -> std::filesystem::path;
    /// Convert this path to a POSIX path.
    /// Returns an empty string if the path is an absolute Windows path.
    /// @return A string with the path, using slash (`/`) path separators.
    [[nodiscard]] auto toPosix() const noexcept -> text::String;
    /// Convert this path to a Windows native path.
    /// Returns an empty string if the path is an absolute Posix path.
    /// @param format The output format for the Windows path.
    /// @return The window native path.
    [[nodiscard]] auto toWindows(PathWindowsFormat format = PathWindowsFormat::Extended) const noexcept -> text::String;
    /// Creates a string for display, joining the path using (`/`) path separators.
    /// @return A platform-agnostic string for display.
    [[nodiscard]] auto toString() const noexcept -> text::String;
    /// Assemble a path for several path elements.
    /// @param elements The path elements to join into a path.
    /// @return The assembled path.
    [[nodiscard]] static auto fromElements(const text::StringList &elements) noexcept -> Path;
    /// Convert a POSIX path.
    /// A call of this method ignores any Window-specific handling.
    /// @param path The string with the path to convert.
    /// @return The converted path or an empty path if the path is not a valid POSIX path.
    [[nodiscard]] static auto fromPosix(const text::String &path) noexcept -> Path;
    /// Convert a POSIX path.
    /// A call of this method ignores any Window-specific handling.
    /// @param path The string with the path to convert.
    /// @throws err::ParseError If the path is not a valid POSIX path.
    /// @return The converted path.
    [[nodiscard]] static auto fromPosixOrThrow(const text::String &path) -> Path;
    /// Convert a Windows path.
    /// A call of this method ignores any Posix-specific handling.
    /// @param path The string with the path to convert.
    /// @return The converted path or an empty path if the path is not a valid Windows path.
    [[nodiscard]] static auto fromWindows(const text::String &path) noexcept -> Path;
    /// Convert a Windows path.
    /// A call of this method ignores any Posix-specific handling.
    /// @param path The string with the path to convert.
    /// @throws err::ParseError If the path is not a valid Windows path.
    /// @return The converted path.
    [[nodiscard]] static auto fromWindowsOrThrow(const text::String &path) -> Path;
    /// Convert a POSIX or Windows path, depending on the current platform.
    /// This is the same as calling fromPosix() or fromWindows().
    /// @param path The string with the path to convert.
    /// @return The converted path or an empty path if the path is not valid for the platform.
    [[nodiscard]] static auto fromNative(const text::String &path) noexcept -> Path;
    /// Convert a POSIX or Windows path, depending on the current platform.
    /// This is the same as calling fromPosix() or fromWindows().
    /// @param path The string with the path to convert.
    /// @throws err::ParseError If the path is not valid for the current platform.
    /// @return The converted path.
    [[nodiscard]] static auto fromNativeOrThrow(const text::String &path) -> Path;

public: // factory methods
    /// Return the shared empty, invalid path.
    /// @return An empty path.
    [[nodiscard]] static auto empty() noexcept -> const Path &;
    /// Return a path to the current working directory of the process.
    /// @return An absolute path to the current working directory.
    [[nodiscard]] static auto currentDirectory() noexcept -> Path;
    /// Return the home directory for the effective user of this process.
    /// This lookup uses the operating-system account database and does not inspect environment variables.
    /// @return An absolute path to the effective user's home directory, or an empty path on error.
    [[nodiscard]] static auto userHomeDirectory() noexcept -> Path;
    /// Return the home directory for the effective user of this process.
    /// This lookup uses the operating-system account database and does not inspect environment variables.
    /// @return An absolute path to the effective user's home directory.
    /// @throws PathError if the home directory cannot be determined or converted.
    [[nodiscard]] static auto userHomeDirectoryOrThrow() -> Path;
    /// Return the system directory for temporary files and directories.
    /// @return An absolute path to the system temporary directory, or an empty path on error.
    [[nodiscard]] static auto systemTempDirectory() noexcept -> Path;
    /// Return the system directory for temporary files and directories.
    /// @return An absolute path to the system temporary directory.
    /// @throws PathError if the temporary directory cannot be determined or converted.
    [[nodiscard]] static auto systemTempDirectoryOrThrow() -> Path;
    /// Return a "current" path (".").
    /// @return The special valid current path.
    [[nodiscard]] static auto currentElement() noexcept -> Path;
    /// Return a "parent" path ("..").
    /// @return The special valid parent path.
    [[nodiscard]] static auto parentElement() noexcept -> Path;

private:
    explicit Path(impl::PathDataPtr data) noexcept;

private:
    impl::PathDataPtr _data;
};

}
