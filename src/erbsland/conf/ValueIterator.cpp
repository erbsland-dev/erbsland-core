// Copyright (c) 2024 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueIterator.hpp"

#include "impl/value/Value.hpp"

namespace erbsland::conf {

auto ValueIterator::operator*() const noexcept -> reference {
    return *_current;
}

auto ValueIterator::operator->() const noexcept -> pointer {
    return *_current;
}

auto ValueIterator::operator++() noexcept -> ValueIterator & {
    ++_current;
    return *this;
}

auto ValueIterator::operator++(int) noexcept -> ValueIterator {
    return ValueIterator(_current++);
}

auto ValueIterator::operator==(const ValueIterator &other) const noexcept -> bool {
    return _current == other._current;
}

auto ValueIterator::operator!=(const ValueIterator &other) const noexcept -> bool {
    return _current != other._current;
}

auto ValueIterator::operator--() noexcept -> ValueIterator & {
    --_current;
    return *this;
}

auto ValueIterator::operator--(int) noexcept -> ValueIterator {
    return ValueIterator(_current--);
}

}
