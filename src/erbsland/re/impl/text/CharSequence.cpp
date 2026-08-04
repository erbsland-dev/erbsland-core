// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharSequence.hpp"

#include "../error/InternalError.hpp"

#include <algorithm>

namespace erbsland::re::impl {

auto CharSequence::operator==(const CharSequence &other) const noexcept -> bool {
    if (_hash == other._hash) {
        if (_extendedSequence != nullptr && _extendedSequence == other._extendedSequence) {
            return true;
        }
        return std::ranges::equal(sequence(), other.sequence());
    }
    return false;
}

auto CharSequence::operator!=(const CharSequence &other) const noexcept -> bool {
    return !(*this == other);
}

auto CharSequence::size() const noexcept -> std::size_t {
    return _extendedSequence != nullptr ? _extendedSequence->size() : _inlineSize;
}

auto CharSequence::sequence() const noexcept -> std::span<const text::Char> {
    if (_extendedSequence != nullptr) {
        return *_extendedSequence;
    }
    return {_inlineSequence.data(), _inlineSize};
}

auto CharSequence::hash() const noexcept -> std::size_t {
    return _hash;
}

auto CharSequence::begin() const noexcept -> ConstIterator {
    return sequence().data();
}

auto CharSequence::end() const noexcept -> ConstIterator {
    return sequence().data() + size();
}

void CharSequence::append(const text::Char character) {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(character.isValidUnicode(), "Cannot append an invalid Unicode character"_el);
    if (_extendedSequence != nullptr) {
        conditionalDetach();
        _extendedSequence->push_back(character);
    } else if (_inlineSize < cInlineCapacity) {
        _inlineSequence[_inlineSize++] = character;
    } else {
        _extendedSequence = std::make_shared<std::vector<text::Char>>();
        _extendedSequence->reserve(cInlineCapacity * 2U);
        _extendedSequence->insert(_extendedSequence->end(), _inlineSequence.begin(), _inlineSequence.end());
        _extendedSequence->push_back(character);
    }
    _hash = util::combineHash(_hash, std::hash<char32_t>{}(character.toRawValue()));
}

void CharSequence::conditionalDetach() {
    if (_extendedSequence.use_count() > 1) {
        _extendedSequence = std::make_shared<std::vector<text::Char>>(*_extendedSequence);
    }
}

}
