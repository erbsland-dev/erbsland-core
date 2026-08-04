// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockString.hpp"
#include "BlockStyle.hpp"

#include "../text/String.hpp"
#include "../text/u32/U32String.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::cterm {

/// A visible marker used for bullet and numbered terminal document list items.
/// @tested{TerminalDocumentStyleTest}
class TerminalDocumentStyleMarker final {
public:
    /// Marker rendering mode.
    enum class Kind : uint8_t {
        None,    ///< No visible marker.
        Literal, ///< Use the literal marker text.
        Ordered, ///< Render the item number followed by the suffix.
    };

public:
    /// Create an empty marker.
    TerminalDocumentStyleMarker() = default;

    // defaults
    ~TerminalDocumentStyleMarker() = default;
    TerminalDocumentStyleMarker(const TerminalDocumentStyleMarker &) = default;
    TerminalDocumentStyleMarker(TerminalDocumentStyleMarker &&) = default;
    auto operator=(const TerminalDocumentStyleMarker &) -> TerminalDocumentStyleMarker & = default;
    auto operator=(TerminalDocumentStyleMarker &&) -> TerminalDocumentStyleMarker & = default;

public: // accessors
    /// Get the marker kind.
    [[nodiscard]] auto kind() const noexcept -> Kind { return _kind; }
    /// Get the marker style overlay.
    [[nodiscard]] auto style() const noexcept -> BlockStyle { return _style; }
    /// Get the literal marker text.
    [[nodiscard]] auto literal() const noexcept -> BlockString { return _literal; }
    /// Get the ordered-list suffix text.
    [[nodiscard]] auto suffix() const noexcept -> BlockString { return _suffix; }

public:
    /// Clear the marker.
    /// @return Reference to this marker.
    auto clear() noexcept -> TerminalDocumentStyleMarker &;
    /// Set the marker style overlay.
    /// @param style The marker style overlay.
    /// @return Reference to this marker.
    auto setStyle(BlockStyle style) noexcept -> TerminalDocumentStyleMarker &;
    /// Use a literal marker string.
    /// @param literal The literal marker text.
    /// @param style Optional marker style overlay.
    /// @return Reference to this marker.
    auto setLiteral(BlockString literal, BlockStyle style = {}) noexcept -> TerminalDocumentStyleMarker &;
    /// Use a literal marker string.
    /// @param literal The literal marker text.
    /// @param style Optional marker style overlay.
    /// @return Reference to this marker.
    auto setLiteral(const text::U32String &literal, BlockStyle style = {}) -> TerminalDocumentStyleMarker &;
    /// Use a literal marker string.
    /// @param literal The literal marker text.
    /// @param style Optional marker style overlay.
    /// @return Reference to this marker.
    auto setLiteral(const text::String &literal, BlockStyle style = {}) -> TerminalDocumentStyleMarker &;
    /// Use an ordered marker with the default suffix and style.
    /// @return Reference to this marker.
    auto setOrdered() -> TerminalDocumentStyleMarker &;
    /// Use an ordered marker with the given suffix.
    /// @param suffix The suffix appended after the item number.
    /// @param style Optional marker style overlay.
    /// @return Reference to this marker.
    auto setOrdered(BlockString suffix, BlockStyle style = {}) -> TerminalDocumentStyleMarker &;
    /// Use an ordered marker with the given suffix.
    /// @param suffix The suffix appended after the item number.
    /// @param style Optional marker style overlay.
    /// @return Reference to this marker.
    auto setOrdered(const text::U32String &suffix, BlockStyle style = {}) -> TerminalDocumentStyleMarker &;
    /// Render this marker for one list item number.
    /// @param number The one-based item number for ordered lists.
    /// @param baseStyle The base text style used for marker text.
    /// @return The rendered marker.
    [[nodiscard]] auto render(std::size_t number, BlockStyle baseStyle) const -> BlockString;

private:
    /// Combine the marker's configured style with the base style.
    [[nodiscard]] auto markerStyle(BlockStyle baseStyle) const noexcept -> BlockStyle;

private:
    Kind _kind{Kind::None}; ///< Marker rendering mode.
    BlockStyle _style;      ///< Marker style overlay.
    BlockString _literal;   ///< Literal marker text storage.
    BlockString _suffix;    ///< Ordered suffix text storage.
};

}
