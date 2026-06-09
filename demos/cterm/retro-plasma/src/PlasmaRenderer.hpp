// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TerminalApplication.hpp>

#include <vector>

/// Render an animated plasma effect into a terminal buffer.
class PlasmaRenderer final {
public:
    /// Draw the plasma effect into the selected rectangle.
    /// @param buffer The destination buffer.
    /// @param rect The target rectangle.
    /// @param phase The animation phase.
    /// @param paletteIndex The selected palette.
    void render(Buffer &buffer, BlockRectangle rect, double phase, std::size_t paletteIndex) const noexcept;

private:
    [[nodiscard]] static auto palettes() -> const std::vector<ColorSequence> &;
    [[nodiscard]] static auto valueAt(BlockPosition position, BlockRectangle rect, double phase) noexcept -> double;
    [[nodiscard]] static auto cellForValue(double normalizedValue, std::size_t paletteIndex) -> Block;
};
