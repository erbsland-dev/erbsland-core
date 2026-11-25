// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DoxygenSymbol.hpp"

#include <erbsland/List.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/xml/XmlNode.hpp>
#include <tool/frame/ProjectFiles.hpp>

namespace tool::doc::impl {

/// Resolve public C++ references against Doxygen's XML index.
class DoxygenIndex final {
public:
    // defaults
    DoxygenIndex() = default;

public: // indexing
    /// Read a confined Doxygen index.
    /// @param files Project file service.
    /// @param path Existing index XML.
    void read(const frame::ProjectFiles &files, const el::Path &path);
    /// Rewrite inline references, collecting unresolved-reference warnings.
    /// @param text One physical RST line.
    /// @param warnings Collected unresolved or ambiguous targets.
    /// @return Text with explicit Sphinx references.
    auto link(const el::String &text, el::StringList &warnings) const -> el::String;

private:
    /// Add exact and suffix spellings for one symbol.
    void add(DoxygenSymbol symbol);
    /// Read a named XML child's character data.
    static auto childText(const el::data::xml::XmlNodePtr &node, const el::String &name) -> el::String;

private:
    el::StringMap<el::List<DoxygenSymbol>> _symbols; ///< Exact and suffix candidate sets.
};

}
