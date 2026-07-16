// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharSequence.hpp"

#include "../error/InternalError.hpp"

namespace erbsland::re::impl {

CharSequence::CharSequence() : _sequence{std::make_shared<std::vector<text::Char>>()} {
    _sequence->reserve(16);
}

auto CharSequence::operator==(const CharSequence &other) const noexcept -> bool {
    if (_hash == other._hash) {
        if (_sequence == other._sequence) {
            return true;
        }
        return *_sequence == *other._sequence;
    }
    return false;
}

auto CharSequence::operator!=(const CharSequence &other) const noexcept -> bool {
    return !(*this == other);
}

auto CharSequence::size() const noexcept -> std::size_t {
    return _sequence->size();
}

auto CharSequence::sequence() const noexcept -> const SequencePtr & {
    return _sequence;
}

auto CharSequence::hash() const noexcept -> std::size_t {
    return _hash;
}

auto CharSequence::begin() const noexcept -> std::vector<text::Char>::const_iterator {
    return _sequence->begin();
}

auto CharSequence::end() const noexcept -> std::vector<text::Char>::const_iterator {
    return _sequence->end();
}

void CharSequence::append(const text::Char character) {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(character.isValidUnicode(), "Cannot append an invalid Unicode character"_el);
    conditionalDetach();
    _sequence->push_back(character);
    _hash = util::combineHash(_hash, std::hash<char32_t>{}(character.toRawValue()));
}

void CharSequence::conditionalDetach() {
    if (_sequence.use_count() > 1) {
        _sequence = std::make_shared<std::vector<text::Char>>(*_sequence);
    }
}

}
