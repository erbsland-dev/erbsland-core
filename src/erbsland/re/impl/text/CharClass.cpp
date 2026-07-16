// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharClass.hpp"

#include "../error/InternalError.hpp"

#include "../../../util/HashHelper.hpp"

#include <algorithm>
#include <memory>
#include <ranges>

namespace erbsland::re::impl {

void CharClass::normalizeRanges() noexcept {
    if (_data->ranges->empty()) {
        return;
    }
    std::ranges::sort(*_data->ranges);
    RangeVector merged;
    merged.reserve(_data->ranges->size());
    auto currentFirst = _data->ranges->front().first();
    auto currentLast = _data->ranges->front().last();
    for (std::size_t i = 1; i < _data->ranges->size(); ++i) {
        const auto &range = (*_data->ranges)[i];
        const auto nextFirst = range.first();
        if (CharRange::canMerge(currentLast, nextFirst)) {
            // Overlap or adjacency → extend
            if (range.last() > currentLast) {
                currentLast = range.last();
            }
        } else {
            merged.emplace_back(currentFirst, currentLast);
            currentFirst = range.first();
            currentLast = range.last();
        }
    }
    merged.emplace_back(currentFirst, currentLast);
    _data->ranges->swap(merged);
}

void CharClass::buildHash() noexcept {
    std::size_t hash = 0;
    for (const auto &range : *_data->ranges) {
        hash = util::combineHash(hash, std::hash<CharRange>{}(range));
    }
    _data->hash = hash;
}

void CharClass::conditionalDetach() {
    if (_data.use_count() > 1) {
        _data = std::make_shared<Data>(*_data);
    }
}

auto CharClass::operator==(const CharClass &other) const -> bool {
    if (_data->hash == other._data->hash) {
        if (_data == other._data) {
            return true;
        }
        return *_data->ranges == *other._data->ranges;
    }
    return false;
}

auto CharClass::operator!=(const CharClass &other) const -> bool {
    return !(*this == other);
}

void CharClass::add(text::Char character) {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(character.isValidUnicode(), "Cannot add an invalid Unicode character"_el);
    conditionalDetach();
    _data->ranges->emplace_back(character, character);
    _data->hash = 0;
    _data->readyForUse = false;
}

void CharClass::add(text::Char first, text::Char last) {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        first.isValidUnicode() && last.isValidUnicode(), "Cannot add an invalid Unicode character range"_el);
    conditionalDetach();
    _data->ranges->emplace_back(first, last);
    _data->hash = 0;
    _data->readyForUse = false;
}

auto CharClass::size() const noexcept -> std::size_t {
    return _data->ranges->size();
}

auto CharClass::isSingleChar() const noexcept -> bool {
    return size() == 1 && ranges().front().isSingleChar();
}

void CharClass::prepareForUse() {
    if (!_data->readyForUse) {
        conditionalDetach();
        for (const auto &range : *_data->ranges) {
            ERBSLAND_CORE_RE_REQUIRE_SAFETY(
                range.first().isValidUnicode() && range.last().isValidUnicode(),
                "Cannot prepare a character class with invalid Unicode characters"_el);
        }
        normalizeRanges();
        buildHash();
        _data->readyForUse = true;
    }
}

auto CharClass::matches(const text::Char character) const -> bool {
    const auto &data = *_data;
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(data.readyForUse, "CharClass::matches called before normalization"_el);
    const auto &ranges = *data.ranges;
    bool doesMatch = false;
    if (!ranges.empty()) {
        // Find the first range whose first >= character
        const auto rangeIt = std::lower_bound(
            ranges.begin(), ranges.end(), character, [](const CharRange &range, const text::Char &character) noexcept {
                return range.first() < character;
            });
        if (rangeIt != ranges.end() && rangeIt->first() == character) {
            doesMatch = true; // exact match on the start of a range
        } else if (rangeIt != ranges.begin()) {
            const auto &range = *(rangeIt - 1);
            doesMatch = character <= range.last();
        }
    }
    return doesMatch;
}

auto CharClass::toString() const -> text::String {
    const auto &data = *_data;
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(data.readyForUse, "CharClass::toString called before normalization"_el);
    text::String result;
    for (const auto &range : *data.ranges) {
        result.append(range.toString());
    }
    return result;
}

}
