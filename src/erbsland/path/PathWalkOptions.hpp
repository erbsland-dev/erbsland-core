// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathGlobPattern.hpp"
#include "PathInfoParts.hpp"
#include "PathType.hpp"
#include "PathWalkDirection.hpp"
#include "PathWalkFn.hpp"
#include "SymlinkMode.hpp"

#include <utility>

namespace erbsland::path {

/// Options for the path walk function.
/// @tested{PathWalkerTest PathWalkerFilterTest PosixPathWalkerTest}
class PathWalkOptions final {
public:
    /// Create path-walk options with their default values.
    PathWalkOptions() = default;

public:
    /// Get the optional glob selecting paths relative to the walk's starting directory.
    /// @return The compiled glob, or null when glob filtering is disabled.
    [[nodiscard]] auto globPattern() const noexcept -> const PathGlobPatternPtr & { return _globPattern; }
    /// Set a compiled reporting and descent glob; null disables glob filtering.
    /// @param pattern Immutable glob selecting reporting and feasible descendant paths.
    /// @return These options.
    auto setGlobPattern(PathGlobPatternPtr pattern) noexcept -> PathWalkOptions & {
        _globPattern = std::move(pattern);
        return *this;
    }
    /// Compile and set a glob without changing the matching mode.
    /// @param pattern Pattern text, matched relative to the starting directory.
    /// @param flags Compile syntax; explicit flags replace defaults.
    /// @return These options; compilation failure leaves the previous pattern intact.
    /// @throws err::ParseError If compilation fails.
    /// @throws err::ParameterFailure If the flags form an invalid combination.
    auto setGlobPattern(const text::String &pattern, PathGlobPatternFlags flags = PathGlobPatternFlag::Default)
        -> PathWalkOptions & {
        return setGlobPattern(PathGlobPattern::compile(pattern, flags));
    }
    /// Get the glob matching mode; full matching is the default.
    /// @return The mode applied to relative paths.
    [[nodiscard]] auto globMatchMode() const noexcept -> PathGlobMatchMode { return _globMatchMode; }
    /// Select full or suffix glob matching.
    /// @param mode Matching mode for reporting and descendant feasibility.
    /// @return These options.
    auto setGlobMatchMode(PathGlobMatchMode mode) noexcept -> PathWalkOptions & {
        _globMatchMode = mode;
        return *this;
    }
    /// If all errors should be ignored.
    [[nodiscard]] auto ignoreErrors() const noexcept -> bool { return _ignoreErrors; }
    /// Set if all errors should be ignored.
    auto setIgnoreErrors(const bool value) noexcept -> PathWalkOptions & {
        _ignoreErrors = value;
        return *this;
    }
    /// The reported path types while walking.
    /// If directories are not part of this set, they will still be scanned, but the walk function will
    /// not be called for them.
    [[nodiscard]] auto types() const noexcept -> PathTypes { return _types; }
    /// Set the reported path types while walking.
    auto setTypes(const PathTypes value) noexcept -> PathWalkOptions & {
        _types = value;
        return *this;
    }
    /// How to handle symlinks.
    [[nodiscard]] auto symlinkMode() const noexcept -> SymlinkMode { return _symlinkMode; }
    /// Set how to handle symlinks.
    auto setSymlinkMode(const SymlinkMode value) noexcept -> PathWalkOptions & {
        _symlinkMode = value;
        return *this;
    }
    /// The walk direction.
    [[nodiscard]] auto direction() const noexcept -> PathWalkDirection { return _direction; }
    /// Set the walk direction.
    auto setDirection(const PathWalkDirection value) noexcept -> PathWalkOptions & {
        _direction = value;
        return *this;
    }
    /// The info parts to request and cache for entries accepted by the reporting filter.
    [[nodiscard]] auto infoParts() const noexcept -> PathInfoParts { return _infoParts; }
    /// Set the info parts to initially request and cache for a walk with path information.
    auto setInfoParts(const PathInfoParts value) noexcept -> PathWalkOptions & {
        _infoParts = value;
        return *this;
    }
    /// The optional reporting filter; an empty callback accepts every entry matching types().
    [[nodiscard]] auto filter() const noexcept -> const PathWalkFilterFn & { return _filter; }
    /// Set a reporting filter applied to the root and children before requesting infoParts().
    /// Rejected directories are still traversed. The type reflects the selected symlink policy.
    auto setFilter(PathWalkFilterFn value) -> PathWalkOptions & {
        _filter = std::move(value);
        return *this;
    }
    /// The optional descent filter; an empty callback allows traversal into every directory.
    [[nodiscard]] auto descendFilter() const noexcept -> const PathWalkFilterFn & { return _descendFilter; }
    /// Set a filter deciding whether to enter a directory, independently of reporting and direction().
    /// It receives the root and each child directory with its effective Directory type.
    auto setDescendFilter(PathWalkFilterFn value) -> PathWalkOptions & {
        _descendFilter = std::move(value);
        return *this;
    }

private:
    PathGlobPatternPtr _globPattern;                           ///< Optional immutable glob filter.
    PathGlobMatchMode _globMatchMode{PathGlobMatchMode::Full}; ///< Relative-path matching mode.
    bool _ignoreErrors = false;
    PathTypes _types = PathTypes{PathType::RegularFile, PathType::Directory};
    SymlinkMode _symlinkMode = SymlinkMode::Follow;
    PathWalkDirection _direction = PathWalkDirection::RootToLeaf;
    PathInfoParts _infoParts = PathInfoPart::Default;
    PathWalkFilterFn _filter;        ///< Selects reported entries before loading additional metadata.
    PathWalkFilterFn _descendFilter; ///< Selects directories whose children are visited.
};

}
