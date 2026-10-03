// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ItemIndex.hpp>
#include <erbsland/ItemRange.hpp>
#include <erbsland/List.hpp>
#include <erbsland/StringList.hpp>

namespace tool::doc::impl {

/// Resolve ordered, deduplicated source excerpts against original physical lines.
class SourceSelectors final {
public:
    /// Prepare original lines and C++ lexical masks.
    /// @param source Complete C++ source.
    explicit SourceSelectors(const el::String &source);

public: // selection
    /// Select and dedent source fragments, retaining gaps introduced by hiding.
    /// @param show Optional ordered selectors.
    /// @param hide Optional selectors removed from the displayed text.
    /// @return Display lines without wrapper code.
    auto select(const el::String &show, const el::String &hide) const -> el::StringList;

private:
    /// Report a failure in this operation, optionally identifying its input file.
    [[noreturn]] static void throwError(const el::String &description, const el::String &path = {});

private:
    /// Resolve one function, assignment, or comment endpoint.
    auto endpoint(const el::String &kind, el::String name) const -> el::ItemRange;
    /// Split selectors at commas outside quoted names and offset parentheses.
    static auto splitSelectors(const el::String &expression) -> el::StringList;
    /// Resolve selector expressions with optional offsets.
    auto ranges(const el::String &expression) const -> el::List<el::ItemRange>;
    /// Determine the default ///-comment-to-namespace-end excerpt.
    auto defaultRange() const -> el::ItemRange;
    /// Find a matching brace terminator after a declaration.
    auto blockEnd(el::ItemIndex start) const -> el::ItemIndex;

private:
    el::StringList _lines;    ///< Original physical source.
    el::StringList _code;     ///< Source with comments and literals masked.
    el::StringList _comments; ///< Per-line comment text.
};

}
