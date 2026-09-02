// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PlasmaRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace demo {

void PlasmaRenderer::render(
    Buffer &buffer, const Rectangle rect, const double phase, const std::size_t paletteIndex) const noexcept {

    rect.forEach([&](const Position position) -> void {
        buffer.set(position, cellForValue(valueAt(position, rect, phase), paletteIndex));
    });
}

auto PlasmaRenderer::palettes() -> const std::vector<ColorSequence> & {
    static const auto cPalettes = std::vector<ColorSequence>{
        {
            Color{fg::BrightBlue, bg::Blue},
            Color{fg::BrightCyan, bg::Cyan},
            Color{fg::BrightMagenta, bg::Magenta},
            Color{fg::BrightRed, bg::Red},
            Color{fg::BrightYellow, bg::Yellow},
        },
        {
            Color{fg::BrightBlack, bg::Black},
            Color{fg::BrightBlue, bg::BrightBlack},
            Color{fg::BrightCyan, bg::Blue},
            Color{fg::BrightGreen, bg::Cyan},
            Color{fg::BrightWhite, bg::BrightBlue},
        },
        {
            Color{fg::BrightRed, bg::Red},
            Color{fg::BrightYellow, bg::Yellow},
            Color{fg::BrightGreen, bg::Green},
            Color{fg::BrightCyan, bg::Cyan},
            Color{fg::BrightWhite, bg::BrightMagenta},
        },
    };
    return cPalettes;
}

auto PlasmaRenderer::valueAt(const Position position, const Rectangle rect, const double phase) noexcept -> double {

    const auto width = std::max(Coordinate{1}, rect.width());
    const auto height = std::max(Coordinate{1}, rect.height());
    const auto x =
        static_cast<double>((position.x() - rect.x1()).toRawValue()) / static_cast<double>(width.toRawValue());
    const auto y =
        static_cast<double>((position.y() - rect.y1()).toRawValue()) / static_cast<double>(height.toRawValue());
    const auto centeredX = (x - 0.5) * 2.6;
    const auto centeredY = (y - 0.5) * 2.0;
    const auto radial = std::hypot(centeredX, centeredY);
    const auto value = std::sin(centeredX * 4.0 + phase * 1.5) + std::sin(centeredY * 6.0 - phase * 1.2) +
        std::sin((centeredX + centeredY) * 4.5 + phase * 0.8) + std::sin(radial * 9.0 - phase * 2.1);
    return std::clamp((value + 4.0) / 8.0, 0.0, 1.0);
}

auto PlasmaRenderer::cellForValue(const double normalizedValue, const std::size_t paletteIndex) -> Block {
    static constexpr auto cShades = std::array<el::Char, 5>{U' ', U'░', U'▒', U'▓', U'█'};

    const auto &paletteList = palettes();
    const auto &palette = paletteList[paletteIndex % paletteList.size()];
    const auto scaledValue = normalizedValue * static_cast<double>(palette.sequenceLength() - 1);
    const auto baseIndex = static_cast<std::size_t>(scaledValue);
    const auto nextIndex = std::min(baseIndex + 1, palette.sequenceLength() - 1);
    const auto shadeIndex = std::min(
        static_cast<std::size_t>((scaledValue - static_cast<double>(baseIndex)) * static_cast<double>(cShades.size())),
        cShades.size() - 1);
    const auto background = palette.color(baseIndex);
    const auto foreground = palette.color(nextIndex);
    return Block{cShades[shadeIndex], Color{foreground.fg(), background.bg()}};
}

}
