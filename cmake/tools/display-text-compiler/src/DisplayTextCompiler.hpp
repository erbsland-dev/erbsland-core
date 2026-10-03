// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DisplayTextOutput.hpp"

#include <erbsland/conf/Value_fwd.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/text/render/Environment_fwd.hpp>

#include <map>

namespace erbsland::i18n::compiler {

/// Compiles an ELCL display-text catalog into permanent C++ literal tables.
/// @tested{DisplayTextCompilerTest}
class DisplayTextCompiler final {
public:
    /// Configure the generated class.
    /// @param className A non-keyword C++ identifier.
    /// @param namespaceName An optional qualified C++ namespace without a leading separator.
    DisplayTextCompiler(text::String className, text::String namespaceName = {});

public: // compilation
    /// Validate and render both catalog files without writing them.
    /// @param document Non-null parsed ELCL catalog with named sections.
    /// @param headerInclude Header path relative to the generated source file.
    /// @param renderer Optional fresh C++ render environment supplying copyright and license globals; omitted by
    /// default.
    /// @return Both rendered files.
    /// @throws conf::ConfError If the catalog has unsupported values.
    /// @throws text::render::RenderError If template loading or rendering fails.
    [[nodiscard]] auto generate(
        const conf::ValuePtr &document, const text::String &headerInclude, text::render::EnvironmentPtr renderer = {})
        -> DisplayTextOutput;
    /// Parse, validate, and generate a catalog, preserving unchanged output files.
    /// @param input The ELCL source file.
    /// @param header The generated header path.
    /// @param source The generated implementation path.
    /// @throws conf::ConfError if the catalog is malformed or has unsupported values.
    /// @throws path::PathError if a source or output file cannot be accessed.
    /// @throws text::render::RenderError If template loading or rendering fails.
    void compile(const path::Path &input, const path::Path &header, const path::Path &source);

public: // validation
    /// Check a C++ identifier, including reserved keywords and implementation names.
    /// @param name Candidate class or namespace component.
    /// @return True if the compiler accepts the identifier.
    [[nodiscard]] static auto validIdentifier(const text::String &name) -> bool;

private:
    using Entries = text::StringMap<text::String>;
    using Domains = text::StringMap<Entries>;

private:
    /// Collect named sections and text leaves.
    void collect(const conf::ValuePtr &section, const text::String &domain);
    /// Rewrite a file only when its content differs.
    static void writeIfChanged(const path::Path &path, const text::String &content);

private:
    text::String _className;     ///< The generated class name.
    text::String _namespaceName; ///< The optional generated namespace.
    Domains _domains;            ///< Sorted domain and entry values.
};

}
