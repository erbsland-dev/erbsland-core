// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeLocation.hpp"

#include "ColumnCount.hpp"
#include "CpLength.hpp"
#include "LineCount.hpp"

#include "../text/Literals.hpp"
#include "../text/StringFormat.hpp"

namespace erbsland::unit {

using namespace text::literals;

auto CodeLocation::isUndefined() const noexcept -> bool {
    return _line.isNoIndex() && _column.isNoIndex() && _position.isNoIndex();
}

void CodeLocation::nextLine() noexcept {
    _line = _line.isNoIndex() ? LineIndex::zero() : _line.advanced(LineCount::one());
    _column = ColumnIndex::zero();
    _position = _position.isNoIndex() ? CpIndex::zero() : _position.advanced(CpLength::one());
}

void CodeLocation::nextColumn() noexcept {
    _column = _column.isNoIndex() ? ColumnIndex::zero() : _column.advanced(ColumnCount::one());
    _position = _position.isNoIndex() ? CpIndex::zero() : _position.advanced(CpLength::one());
}

auto CodeLocation::toString() const noexcept -> text::String {
    try {
        static const auto lineFormat = text::StringFormat{"{}"_el};
        static const auto lineColumnFormat = text::StringFormat{"{}:{}"_el};
        if (_line.isNoIndex()) {
            return "undefined"_el;
        }
        if (_column.isNoIndex()) {
            return lineFormat.build(_line.toSizeT() + 1U);
        }
        return lineColumnFormat.build(_line.toSizeT() + 1U, _column.toSizeT() + 1U);
    } catch (...) {
        return {};
    }
}

}
