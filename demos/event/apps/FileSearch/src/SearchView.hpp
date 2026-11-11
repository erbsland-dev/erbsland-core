// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "QueryEditor.hpp"
#include "SearchResults.hpp"

#include <erbsland/cterm/Buffer.hpp>

namespace demo {

/// Draw the search screen without accessing terminal input or worker state.
class SearchView final {
    struct Layout {
        el::block::Coordinate width;        ///< Full screen width.
        el::block::Coordinate contentWidth; ///< Text width inside the frames.
        el::block::Coordinate resultRows;   ///< Rows available for selectable paths.
        el::block::Coordinate inputY;       ///< Editable pattern row.
        el::block::Coordinate helpY;        ///< Keyboard help row.
    };

public: // rendering
    /// Render one complete frame, adapting the result viewport to the screen.
    /// @param root The absolute search root.
    /// @param query The editable pattern.
    /// @param results Main-loop results and selection.
    /// @param size Current terminal dimensions.
    /// @param cursorVisible Whether this frame shows the blinking input cursor.
    /// @return The retained frame, valid until the next render.
    [[nodiscard]] auto render(
        const el::Path &root,
        const QueryEditor &query,
        SearchResults &results,
        el::block::Size size,
        bool cursorVisible) -> const el::cterm::Buffer &;
    /// Keep the right end of text and mark a cropped prefix.
    /// @param text The complete display text.
    /// @param width Available terminal cells.
    /// @return Text fitting the available cells.
    [[nodiscard]] static auto cropLeft(const el::String &text, el::block::Coordinate width) -> el::cterm::BlockString;

private:
    void drawRoot(const el::Path &root, const Layout &layout);
    void drawResults(const SearchResults &results, const Layout &layout);
    void drawResultRow(const SearchResults &results, const Layout &layout, el::block::Coordinate row);
    void drawInput(const QueryEditor &query, const Layout &layout, bool cursorVisible);
    void drawHelp(const Layout &layout);

private:
    el::cterm::Buffer _buffer; ///< Retained screen, mutated only by main.
};

}
