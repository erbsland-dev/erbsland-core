// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringConstIterator.hpp"

namespace erbsland::text {

U8StringConstIterator::U8StringConstIterator() = default;

U8StringConstIterator::U8StringConstIterator(const U8String &string, const unit::ByteIndex index) :
    _string{string}, _index{index} {
}

auto U8StringConstIterator::operator==(const U8StringConstIterator &other) const noexcept -> bool {
    if (!isValid() && !other.isValid()) {
        return true;
    }
    if (!isValid() || !other.isValid()) {
        return false;
    }
    if (_string.storageId() != other._string.storageId()) {
        return false;
    }
    return _index == other._index;
}

auto U8StringConstIterator::operator!=(const U8StringConstIterator &other) const noexcept -> bool {
    return !operator==(other);
}

auto U8StringConstIterator::isValid() const noexcept -> bool {
    return !_string.isEmpty() && !_index.isNoIndex();
}

auto U8StringConstIterator::operator*() const -> Char {
    if (!isValid()) {
        return Char::null();
    }
    const auto character = _string.charAt(_index);
    return character.isSignal() ? Char::null() : character;
}

auto U8StringConstIterator::operator++() -> U8StringConstIterator & {
    if (!isValid()) {
        return *this;
    }
    _string.advance(_index);
    return *this;
}

auto U8StringConstIterator::operator++(int) -> U8StringConstIterator {
    if (!isValid()) {
        return *this;
    }
    const auto result = *this;
    _string.advance(_index);
    return result;
}

auto U8StringConstIterator::operator->() const -> const Char * {
    if (!isValid()) {
        return nullptr;
    }
    if (_index.isNoIndex()) {
        return nullptr;
    }
    _currentChar = operator*();
    return &_currentChar;
}

}
