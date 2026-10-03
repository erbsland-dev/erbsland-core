// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

#include <exception>
#include <utility>

namespace tool::frame {

using namespace el::text::literals;

/// Locate and validate the repository before accessing project files.
/// @tested{ProjectRootTest}
class ProjectRoot final {
    /// Diagnostic title used when project-root discovery or validation fails.
    constexpr static auto cNotFoundTitle = "Could not find a valid Erbsland Core project directory."_el;

public:
    /// Create a new project root tool with the given unchecked directory.
    explicit ProjectRoot(el::Path uncheckedProjectDirectory) :
        _uncheckedProjectDirectory(std::move(uncheckedProjectDirectory)) {}

    // defaults/deletions
    ~ProjectRoot() = default;
    ProjectRoot(const ProjectRoot &) = delete;
    auto operator=(const ProjectRoot &) -> ProjectRoot & = delete;

public: // discovery
    /// Search the supplied directory and its ancestors, inspecting at most 1024 characters per candidate.
    /// @return The physical project root.
    /// @throws el::ApplicationError If discovery fails or access is denied.
    [[nodiscard]] auto discover() -> el::Path;
    /// Validate an explicitly supplied project directory.
    /// @return The physical project root.
    /// @throws el::ApplicationError If the directory has no readable project marker.
    [[nodiscard]] auto validate() -> el::Path;

private:
    /// Check if the directory contains a `CMakeLists.txt` with a project marker.
    /// @param directory The candidate root to inspect.
    /// @return `true` if the given directory is a valid Erbsland Core project root.
    [[nodiscard]] auto hasMarker(const el::Path &directory) -> bool;
    /// Check the initial contents of a CMake file for the Erbsland Core marker.
    /// @param cmakeFile The candidate project file.
    /// @return `true` if the CMake file contains a valid Erbsland Core project root marker.
    [[nodiscard]] auto isErbslandCoreProjectFile(const el::Path &cmakeFile) -> bool;
    /// Throw an application if no project root could be discovered or validated.
    /// @param description A description of the error that occurred (why was it not found?).
    /// @param cause An optional exception to include in the application's stack trace.
    /// @param sourcePath The failing source path, or empty to use the project directory.
    [[noreturn]] void throwNotFound(
        el::String description, const std::exception_ptr &cause = {}, el::String sourcePath = {});

private:
    el::Path _uncheckedProjectDirectory; ///< The unchecked project directory provided by the caller.
    el::Path _resolvedProjectDirectory;  ///< The resolved project directory, if resolving succeeded.
};

}
