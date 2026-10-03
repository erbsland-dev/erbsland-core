// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DigitViewport.hpp"

#include <erbsland/CpCount.hpp>

#include <algorithm>

namespace demo {

auto DigitViewport::maximumFirstLine() const noexcept -> Coordinate {
    if (_lineCount <= _visibleLines) {
        return Coordinate{0};
    }
    return _lineCount - _visibleLines;
}

void DigitViewport::update(const el::CpCount digits, const Coordinate visibleLines) noexcept {
    const auto digitsValue = digits.toSizeT();
    constexpr auto digitsPerLine = cDigitsPerLine.toSizeT();

    _lineCount = Coordinate{digitsValue / digitsPerLine};
    if (digitsValue % digitsPerLine != 0) {
        ++_lineCount;
    }
    _visibleLines = std::max(visibleLines, Coordinate{1});
    if (_following) {
        _firstLine = maximumFirstLine();
    } else {
        _firstLine = std::min(_firstLine, maximumFirstLine());
    }
}

void DigitViewport::up(const Coordinate lines) noexcept {
    _following = false;
    _firstLine -= std::min(lines, _firstLine);
}

void DigitViewport::down(const Coordinate lines) noexcept {
    _following = false;
    _firstLine += std::min(lines, maximumFirstLine() - _firstLine);
}

void DigitViewport::home() noexcept {
    _following = false;
    _firstLine = Coordinate{0};
}

void DigitViewport::end() noexcept {
    _following = false;
    _firstLine = maximumFirstLine();
}

void DigitViewport::follow() noexcept {
    _following = true;
    _firstLine = maximumFirstLine();
}

}
