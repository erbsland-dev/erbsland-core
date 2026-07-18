// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringConstIterator.hpp"

#include "U16String.hpp"

namespace erbsland::text {

struct U16StringConstIterator::Private {
    U16String string;         ///< The string accessed by this iterator
    unit::U16DataIndex index; ///< The current byte index within the storage
};

U16StringConstIterator::U16StringConstIterator() : _p{nullptr} {
}

U16StringConstIterator::U16StringConstIterator(const U16String &string, unit::U16DataIndex index) :
    _p{std::make_unique<Private>(string, index)} {
}

U16StringConstIterator::U16StringConstIterator(const U16StringConstIterator &other) :
    _p{other._p == nullptr ? nullptr : std::make_unique<Private>(*other._p)} {
}

U16StringConstIterator::U16StringConstIterator(U16StringConstIterator &&other) noexcept : _p{std::move(other._p)} {
}

U16StringConstIterator::~U16StringConstIterator() = default;

auto U16StringConstIterator::operator=(const U16StringConstIterator &other) -> U16StringConstIterator & {
    if (this == &other) {
        return *this;
    }
    _p = other._p == nullptr ? nullptr : std::make_unique<Private>(*other._p);
    return *this;
}

auto U16StringConstIterator::operator=(U16StringConstIterator &&other) noexcept -> U16StringConstIterator & {
    if (this == &other) {
        return *this;
    }
    _p = std::move(other._p);
    return *this;
}

auto U16StringConstIterator::operator==(const U16StringConstIterator &other) const noexcept -> bool {
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

auto U16StringConstIterator::operator!=(const U16StringConstIterator &other) const noexcept -> bool {
    return !operator==(other);
}

auto U16StringConstIterator::isValid() const noexcept -> bool {
    return _p != nullptr && !_p->string.isEmpty() && !_p->index.isNoIndex();
}

auto U16StringConstIterator::operator*() const -> Char {
    if (!isValid()) {
        return Char::null();
    }
    return _p->string.charAt(_p->index);
}

auto U16StringConstIterator::operator++() -> U16StringConstIterator & {
    if (!isValid()) {
        return *this;
    }
    _p->string.advance(_p->index);
    return *this;
}

auto U16StringConstIterator::operator++(int) -> U16StringConstIterator {
    if (!isValid()) {
        return *this;
    }
    const auto result = *this;
    _p->string.advance(_p->index);
    return result;
}

auto U16StringConstIterator::operator->() const -> const Char * {
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
