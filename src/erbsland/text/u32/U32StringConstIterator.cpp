// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringConstIterator.hpp"

#include "U32StringView.hpp"

namespace erbsland::text {

struct U32StringConstIterator::Private {
    U32StringView view;  ///< The view accessed by this iterator
    unit::CpIndex index; ///< The current byte index within the storage
};

U32StringConstIterator::U32StringConstIterator() : _p{nullptr} {
}

U32StringConstIterator::U32StringConstIterator(const U32StringView &view, unit::CpIndex index) :
    _p{std::make_unique<Private>(view, index)} {
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
    if (_p->view.storageId() != other._p->view.storageId()) {
        return false;
    }
    return _p->index == other._p->index;
}

auto U32StringConstIterator::operator!=(const U32StringConstIterator &other) const noexcept -> bool {
    return !operator==(other);
}

auto U32StringConstIterator::isValid() const noexcept -> bool {
    return _p != nullptr && !_p->view.isEmpty() && !_p->index.isNoIndex();
}

auto U32StringConstIterator::operator*() const -> Char {
    if (!isValid()) {
        return Char::null();
    }
    return _p->view.charAt(_p->index);
}

auto U32StringConstIterator::operator++() -> U32StringConstIterator & {
    if (!isValid()) {
        return *this;
    }
    _p->view.advance(_p->index);
    return *this;
}

auto U32StringConstIterator::operator++(int) -> U32StringConstIterator {
    if (!isValid()) {
        return *this;
    }
    const auto result = *this;
    _p->view.advance(_p->index);
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
