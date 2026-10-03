// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../frame/ProjectFiles.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/StringMap.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

/// Prepare a reviewable demo creation plan before writing any source files.
class DemoCreation final {
    /// Source files and callable parts of a multi-part demo.
    struct Parts {
        el::StringList sources;                  ///< Ordered source filenames.
        el::StringList functions;                ///< Function declarations.
        el::StringMap<el::String> registrations; ///< Public names and their functions.
    };

    /// Renderer and registration-capture key naming the function for a demo part.
    static constexpr auto cFunction = "function"_el;

public:
    /// Validate a three-part demo path and prepare its file changes.
    /// @param files Confined project files.
    /// @param renderer Compiled template renderer.
    /// @param path Domain, DemoName, and DemoPart separated by slashes.
    DemoCreation(const frame::ProjectFiles &files, el::render::EnvironmentPtr renderer, const el::String &path);

public: // accessors
    /// Get planned file names and complete contents.
    auto updates() const noexcept -> const el::StringMap<el::String> & { return _updates; }
    /// Get suspicious domain/header naming diagnostics.
    auto warnings() const noexcept -> const el::StringList & { return _warnings; }
    /// Get the documentation directive for the new demo part.
    auto documentation() const -> el::String;

public: // application
    /// Apply the reviewed plan through confined atomic file updates.
    void apply() const;

private:
    /// Report a failure in this operation, optionally identifying its input file.
    [[noreturn]] static void throwError(const el::String &description, const el::String &path = {});

private:
    /// Validate the requested path and derive its demo names.
    void parsePath(const el::String &path);
    /// Merge an existing demo's sources, declarations, and registrations.
    void readExistingParts(const el::Path &demo, Parts &parts);
    /// Render all files for the combined parts into the update plan.
    void prepareFiles(const el::Path &demo, Parts &parts);
    /// Record absent library domains or matching headers without rejecting the demo.
    void warnMissingHeader();
    /// Plan a changed file after validating its destination.
    void add(const el::Path &path, const el::String &text);
    /// Render a template using common demo fields and supplied fields.
    auto render(const el::String &layout, el::render::Context context = {}) const -> el::String;
    /// Extend the sorted subdirectory list of one CMake source file.
    void directory(const el::Path &path, const el::String &entry);

private:
    const frame::ProjectFiles &_files;    ///< Confined project access.
    el::render::EnvironmentPtr _renderer; ///< Compiled resource templates.
    el::String _domain;                   ///< Library domain name.
    el::String _name;                     ///< Demo collection name.
    el::String _part;                     ///< New demo part name.
    el::String _function;                 ///< Lowercase-first function name.
    el::String _target;                   ///< Existing or generated CMake target.
    el::StringMap<el::String> _updates;   ///< Planned file contents keyed by absolute path.
    el::StringList _warnings;             ///< Domain/header diagnostics.
};

}
