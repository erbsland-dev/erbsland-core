// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SearchUpdate.hpp"

#include <erbsland/ItemIndex.hpp>
#include <erbsland/String.hpp>

#include <cstdint>
#include <optional>

namespace demo {

/// Main-loop result state; only updates for the current generation may change it.
class SearchResults final {
public: // search lifecycle
    /// Invalidate outstanding deliveries and clear the previous selection.
    void reset();
    /// Accept a delivery if its generation is still current.
    /// @param generation The query identity retained by the bound reply.
    /// @param update The owned result batch.
    void receive(std::uint64_t generation, SearchUpdate update);
    /// Set an input or progress message without changing the result list.
    void setStatus(el::String status) { _status = std::move(status); }

public: // selection
    /// Move the selection, clamping at the first and last result.
    /// @param down Whether to move down rather than up.
    /// @param distance Number of entries to move.
    void move(bool down, el::ItemCount distance);
    /// Update the viewport and keep the selection visible.
    /// @param rows Number of visible result rows, at least one.
    void resize(el::ItemCount rows);
    /// Get the selected absolute path, or no value if there are no results.
    [[nodiscard]] auto selectedPath() const -> std::optional<el::Path>;

public: // accessors
    [[nodiscard]] auto generation() const noexcept -> std::uint64_t { return _generation; }
    [[nodiscard]] auto paths() const noexcept -> const el::PathList & { return _paths; }
    [[nodiscard]] auto selected() const noexcept -> el::ItemIndex { return _selected; }
    [[nodiscard]] auto first() const noexcept -> el::ItemIndex { return _first; }
    [[nodiscard]] auto rows() const noexcept -> el::ItemCount { return _rows; }
    [[nodiscard]] auto status() const noexcept -> const el::String & { return _status; }

private:
    std::uint64_t _generation{0}; ///< Increased before cancelling or replacing a query.
    el::PathList _paths;          ///< All matches accepted for the current query.
    el::ItemIndex _selected;      ///< Selected row; unused for an empty list.
    el::ItemIndex _first;         ///< First row in the viewport.
    el::ItemCount _rows{1};       ///< Visible result rows.
    el::String _status;           ///< Query feedback and completion status.
};

}
