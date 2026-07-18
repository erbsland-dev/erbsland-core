// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "String.hpp"

#include "../unit/ColumnCount.hpp"
#include "../unit/ColumnIndex.hpp"
#include "../unit/LineIndex.hpp"
#include "../util/List.hpp"

#include <utility>

namespace erbsland::text {

/// A marker range for a line-oriented code snippet.
/// @tested{TextDocumentTest}
class CodeSnippetMarker final {
public:
    CodeSnippetMarker() = default;
    /// Create a marker for a snippet line.
    /// @param line The original zero-based line index.
    /// @param column The zero-based logical source-code-point column.
    /// @param length The marker length in logical source code points, or zero for a point marker.
    /// @param label Optional label rendered after the marker.
    /// @param style Optional style token for renderers.
    CodeSnippetMarker(
        unit::LineIndex line,
        unit::ColumnIndex column,
        unit::ColumnCount length = unit::ColumnCount::one(),
        String label = {},
        String style = {}) noexcept :
        _line{line}, _column{column}, _length{length}, _label{std::move(label)}, _style{std::move(style)} {}

public: // accessors
    /// Get the original zero-based line index.
    [[nodiscard]] auto line() const noexcept -> unit::LineIndex { return _line; }
    /// Get the zero-based logical source-code-point column.
    [[nodiscard]] auto column() const noexcept -> unit::ColumnIndex { return _column; }
    /// Get the marker length in logical source code points.
    [[nodiscard]] auto length() const noexcept -> unit::ColumnCount { return _length; }
    /// Get the optional marker label.
    [[nodiscard]] auto label() const noexcept -> String { return _label; }
    /// Get the optional marker style token.
    [[nodiscard]] auto style() const noexcept -> String { return _style; }

private:
    unit::LineIndex _line{unit::LineIndex::noIndex()};       ///< Original line index.
    unit::ColumnIndex _column{unit::ColumnIndex::noIndex()}; ///< Marker start column.
    unit::ColumnCount _length{unit::ColumnCount::one()};     ///< Marker length.
    String _label;                                           ///< Optional marker label.
    String _style;                                           ///< Optional style token.
};

/// A list of code snippet markers.
using CodeSnippetMarkerList = util::List<CodeSnippetMarker>;

}
