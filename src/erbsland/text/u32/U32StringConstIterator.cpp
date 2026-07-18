// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringConstIterator.hpp"

#include "U32String.hpp"

namespace erbsland::text {

struct U32StringConstIterator::Private {
    U32String string;    ///< The string accessed by this iterator
    unit::CpIndex index; ///< The current byte index within the storage
};

U32StringConstIterator::U32StringConstIterator() : _p{nullptr} {
}

U32StringConstIterator::U32StringConstIterator(const U32String &string, unit::CpIndex index) :
    _p{std::make_unique<Private>(string, index)} {
}

U32StringConstIterator::U32StringConstIterator(const U32StringConstIterator &other) :
    _p{other._p == nullptr ? nullptr : std::make_unique<Private>(*other._p)} {
}

U32StringConstIterator::U32StringConstIterator(U32StringConstIterator &&other) noexcept : _p{std::move(other._p)} {
}

U32StringConstIterator::~U32StringConstIterator() = default;

auto U32StringConstIterator::operator=(const U32StringConstIterator &other) -> U32StringConstIterator & {
    if (this == &other) {
        return *this;
    }
    _p = other._p == nullptr ? nullptr : std::make_unique<Private>(*other._p);
    return *this;
}

auto U32StringConstIterator::operator=(U32StringConstIterator &&other) noexcept -> U32StringConstIterator & {
    if (this == &other) {
        return *this;
    }
    _p = std::move(other._p);
    return *this;
}

auto U32StringConstIterator::operator==(const U32StringConstIterator &other) const noexcept -> bool {
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

auto U32StringConstIterator::operator!=(const U32StringConstIterator &other) const noexcept -> bool {
    return !operator==(other);
}

auto U32StringConstIterator::isValid() const noexcept -> bool {
    return _p != nullptr && !_p->string.isEmpty() && !_p->index.isNoIndex();
}

auto U32StringConstIterator::operator*() const -> Char {
    if (!isValid()) {
        return Char::null();
    }
    return _p->string.charAt(_p->index);
}

auto U32StringConstIterator::operator++() -> U32StringConstIterator & {
    if (!isValid()) {
        return *this;
    }
    _p->string.advance(_p->index);
    return *this;
}

auto U32StringConstIterator::operator++(int) -> U32StringConstIterator {
    if (!isValid()) {
        return *this;
    }
    const auto result = *this;
    _p->string.advance(_p->index);
    return result;
}

auto U32StringConstIterator::operator->() const -> const Char * {
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
