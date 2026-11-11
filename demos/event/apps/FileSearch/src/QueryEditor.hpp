// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/CpIndex.hpp>
#include <erbsland/cterm/Key.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>

namespace demo {

/// A single-line query whose cursor moves over base characters and their combining marks.
class QueryEditor final {
public: // editing
    /// Apply an editing key; navigation keys for the result list are left to the caller.
    /// @param key The decoded terminal key.
    /// @return Whether the query text changed.
    [[nodiscard]] auto edit(const el::cterm::Key &key) -> bool;
    /// Get an owning snapshot of the query.
    [[nodiscard]] auto text() const -> el::String { return _text; }
    /// Get the cursor position in code points.
    [[nodiscard]] auto cursor() const noexcept -> el::CpIndex { return _cursor; }

private:
    [[nodiscard]] auto previous() const noexcept -> el::CpIndex;
    [[nodiscard]] auto next() const noexcept -> el::CpIndex;

private:
    el::StringEditor _text; ///< Mutable text belongs to the main loop only.
    el::CpIndex _cursor;    ///< Insertion position, always on an editing-unit boundary.
};

}
