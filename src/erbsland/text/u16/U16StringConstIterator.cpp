// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringConstIterator.hpp"

namespace erbsland::text {

U16StringConstIterator::U16StringConstIterator() = default;

U16StringConstIterator::U16StringConstIterator(const U16String &string, const unit::U16DataIndex index) :
    _string{string}, _index{index} {
}

auto U16StringConstIterator::operator==(const U16StringConstIterator &other) const noexcept -> bool {
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

auto U16StringConstIterator::operator!=(const U16StringConstIterator &other) const noexcept -> bool {
    return !operator==(other);
}

auto U16StringConstIterator::isValid() const noexcept -> bool {
    return !_string.isEmpty() && !_index.isNoIndex();
}

auto U16StringConstIterator::operator*() const -> Char {
    if (!isValid()) {
        return Char::null();
    }
    return _string.charAt(_index);
}

auto U16StringConstIterator::operator++() -> U16StringConstIterator & {
    if (!isValid()) {
        return *this;
    }
    _string.advance(_index);
    return *this;
}

auto U16StringConstIterator::operator++(int) -> U16StringConstIterator {
    if (!isValid()) {
        return *this;
    }
    const auto result = *this;
    _string.advance(_index);
    return result;
}

auto U16StringConstIterator::operator->() const -> const Char * {
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
