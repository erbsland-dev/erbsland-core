// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringConstIterator.hpp"

namespace erbsland::text {

U32StringConstIterator::U32StringConstIterator() = default;

U32StringConstIterator::U32StringConstIterator(const U32String &string, const unit::CpIndex index) :
    _string{string}, _index{index} {
}

auto U32StringConstIterator::operator==(const U32StringConstIterator &other) const noexcept -> bool {
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

auto U32StringConstIterator::operator!=(const U32StringConstIterator &other) const noexcept -> bool {
    return !operator==(other);
}

auto U32StringConstIterator::isValid() const noexcept -> bool {
    return !_string.isEmpty() && !_index.isNoIndex();
}

auto U32StringConstIterator::operator*() const -> Char {
    if (!isValid()) {
        return Char::null();
    }
    return _string.charAt(_index);
}

auto U32StringConstIterator::operator++() -> U32StringConstIterator & {
    if (!isValid()) {
        return *this;
    }
    _string.advance(_index);
    return *this;
}

auto U32StringConstIterator::operator++(int) -> U32StringConstIterator {
    if (!isValid()) {
        return *this;
    }
    const auto result = *this;
    _string.advance(_index);
    return result;
}

auto U32StringConstIterator::operator->() const -> const Char * {
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
