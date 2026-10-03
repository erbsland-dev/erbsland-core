// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DynamicStringPatternData.hpp"

namespace erbsland::text::impl {

auto DynamicStringPatternData::parsePosixGlob(StringCharReader &reader) -> StringPatternDataPtr {
    auto data = std::make_shared<DynamicStringPatternData>();
    data->_kind = StringPatternKind::PosixGlob;
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == U'*') {
            data->appendStar();
        } else if (character == U'?') {
            data->appendOneChar();
        } else if (character == U'[') {
            data->appendPosixGlobSet(reader);
        } else {
            data->appendCharacter(character);
        }
    }
    return data;
}

void DynamicStringPatternData::appendPosixGlobSet(StringCharReader &reader) {
    const auto start = reader.save();
    const auto offset = _ranges.size();
    const auto negated = reader.readIf(Char{U'!'});
    auto first = true;
    while (!reader.isAtEnd()) {
        // A leading closing bracket is a literal set member, not the delimiter.
        if (!first && reader.readIf(Char{U']'})) {
            const auto total = checkedRangeCount(_ranges.size());
            _elements.push_back(
                StringPatternElement{
                    .kind = negated ? StringPatternElementKind::NegatedSet : StringPatternElementKind::Set,
                    .rangeOffset = checkedRangeCount(offset),
                    .rangeCount = checkedRangeCount(total - offset)});
            return;
        }
        appendPosixGlobRange(reader);
        first = false;
    }
    // An unmatched opening bracket is literal; parse its following text normally.
    _ranges.resize(offset);
    reader.restore(start);
    appendCharacter(Char{U'['});
}

void DynamicStringPatternData::appendPosixGlobRange(StringCharReader &reader) {
    const auto first = reader.read();
    if (!reader.readIf(Char{U'-'})) {
        _ranges.emplace_back(first);
        return;
    }
    if (reader.isAtEnd() || reader.peek() == U']') {
        _ranges.emplace_back(first);
        _ranges.emplace_back(Char{U'-'});
        return;
    }
    const auto last = reader.read();
    // CharRange orders its endpoints; descending glob ranges must instead match nothing.
    if (first <= last) {
        _ranges.emplace_back(first, last);
    }
}

}
