// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringConstIterator.hpp"

#include "U8String.hpp"

namespace erbsland::text {

struct U8StringConstIterator::Private {
    U8String string;       ///< The string accessed by this iterator
    unit::ByteIndex index; ///< The current byte index within the storage
};

U8StringConstIterator::U8StringConstIterator() : _p{nullptr} {
}

U8StringConstIterator::U8StringConstIterator(const U8String &string, unit::ByteIndex index) :
    _p{std::make_unique<Private>(string, index)} {
}

U8StringConstIterator::U8StringConstIterator(const U8StringConstIterator &other) :
    _p{other._p == nullptr ? nullptr : std::make_unique<Private>(*other._p)} {
}

U8StringConstIterator::U8StringConstIterator(U8StringConstIterator &&other) noexcept : _p{std::move(other._p)} {
}

U8StringConstIterator::~U8StringConstIterator() = default;

auto U8StringConstIterator::operator=(const U8StringConstIterator &other) -> U8StringConstIterator & {
    if (this == &other) {
        return *this;
    }
    _p = other._p == nullptr ? nullptr : std::make_unique<Private>(*other._p);
    return *this;
}

auto U8StringConstIterator::operator=(U8StringConstIterator &&other) noexcept -> U8StringConstIterator & {
    if (this == &other) {
        return *this;
    }
    _p = std::move(other._p);
    return *this;
}

auto U8StringConstIterator::operator==(const U8StringConstIterator &other) const noexcept -> bool {
    if (!isValid() && !other.isValid()) {
        return true;
    }
    if (!isValid() || !other.isValid()) {
        return false;
    }
    if (_p->string.storageId() != other._p->string.storageId()) {
        return false;
    }
    return _p->index == other._p->index;
}

auto U8StringConstIterator::operator!=(const U8StringConstIterator &other) const noexcept -> bool {
    return !operator==(other);
}

auto U8StringConstIterator::isValid() const noexcept -> bool {
    return _p != nullptr && !_p->string.isEmpty() && !_p->index.isNoIndex();
}

auto U8StringConstIterator::operator*() const -> Char {
    if (!isValid()) {
        return Char::null();
    }
    const auto character = _p->string.charAt(_p->index);
    return character.isSignal() ? Char::null() : character;
}

auto U8StringConstIterator::operator++() -> U8StringConstIterator & {
    if (!isValid()) {
        return *this;
    }
    _p->string.advance(_p->index);
    return *this;
}

auto U8StringConstIterator::operator++(int) -> U8StringConstIterator {
    if (!isValid()) {
        return *this;
    }
    const auto result = *this;
    _p->string.advance(_p->index);
    return result;
}

auto U8StringConstIterator::operator->() const -> const Char * {
    if (!isValid()) {
        return nullptr;
    }
    if (_p->index.isNoIndex()) {
        return nullptr;
    }
    _currentChar = operator*();
    return &_currentChar;
}

}
