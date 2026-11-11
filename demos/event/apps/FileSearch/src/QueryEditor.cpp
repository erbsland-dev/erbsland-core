// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "QueryEditor.hpp"

#include <erbsland/Char.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/U32String.hpp>

namespace demo {

using namespace el::text::literals;
using el::cterm::Key;

auto QueryEditor::edit(const Key &key) -> bool {
    if (key == Key::Left) {
        _cursor = previous();
    } else if (key == Key::Right) {
        _cursor = next();
    } else if (key == Key::Home) {
        _cursor = {};
    } else if (key == Key::End) {
        _cursor = el::CpIndex::end(_text.characterLength());
    } else if (key == Key::Backspace && !_cursor.isZero()) {
        const auto start = previous();
        _text.remove(el::CpRange{start, start.absoluteDistanceTo(_cursor)});
        _cursor = start;
        return true;
    } else if (key == Key::Delete && _cursor.isWithin(_text.characterLength())) {
        _text.remove(el::CpRange{_cursor, _cursor.absoluteDistanceTo(next())});
        return true;
    } else {
        auto input = el::String{};
        if (key == Key::Space) {
            input = " "_el;
        } else if (key.type() == Key::Character || key.type() == Key::Combined) {
            input = key.combinedCharacter().toString();
        }
        if (input.isEmpty()) {
            return false;
        }
        for (auto index = el::CpIndex{}; index.isWithin(input.characterLength()); ++index) {
            if (input.charAt(index).isControl()) {
                return false;
            }
        }
        _text.insert(_cursor, input);
        _cursor += input.characterLength();
        return true;
    }
    return false;
}

auto QueryEditor::previous() const noexcept -> el::CpIndex {
    auto index = _cursor;
    if (!index.isZero()) {
        --index;
        while (!index.isZero() && _text.charAt(index).displayWidth() == 0) {
            --index;
        }
    }
    return index;
}

auto QueryEditor::next() const noexcept -> el::CpIndex {
    auto index = _cursor;
    const auto end = el::CpIndex::end(_text.characterLength());
    if (index < end) {
        ++index;
        while (index < end && _text.charAt(index).displayWidth() == 0) {
            ++index;
        }
    }
    return index;
}

}
