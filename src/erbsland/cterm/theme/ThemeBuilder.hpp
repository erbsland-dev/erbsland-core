// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PropertyEditor.hpp"
#include "Theme.hpp"

namespace erbsland::cterm::theme {

/// Builder for immutable themes.
class ThemeBuilder final {
public:
    /// Create an empty theme builder.
    ThemeBuilder() = default;

public:
    /// Register a new theme tag.
    /// @throws std::logic_error After 64 tags.
    [[nodiscard]] auto registerTag() -> Tag;
    /// Edit one property sheet.
    /// @param selector The selector for the sheet.
    [[nodiscard]] auto edit(Selector selector) -> PropertyEditor;
    /// Build an immutable theme.
    [[nodiscard]] auto build() const -> ThemeConstPtr;

public:
    /// Create a builder from an existing theme.
    [[nodiscard]] static auto from(const ThemeConstPtr &theme) -> ThemeBuilder;
    /// Create the dark default theme builder.
    [[nodiscard]] static auto dark() -> ThemeBuilder;
    /// Create the light default theme builder.
    [[nodiscard]] static auto light() -> ThemeBuilder;
    /// Create the monochrome default theme builder.
    [[nodiscard]] static auto monochrome() -> ThemeBuilder;
    /// Create the zero theme builder.
    [[nodiscard]] static auto zero() -> ThemeBuilder;

private:
    /// Edit a definition and preserve insertion order.
    [[nodiscard]] auto definitionFor(Selector selector) -> Theme::Definition &;
    /// Install common structural blocks and margins.
    void addClassicBlocks();
    /// Install plain structural blocks and margins.
    void addPlainBlocks();

private:
    Theme::Definitions _definitions; ///< Authored definitions.
    uint64_t _nextOrder{0};          ///< Next insertion order.
    uint8_t _nextTagBit{0};          ///< Next tag bit.
};

}
