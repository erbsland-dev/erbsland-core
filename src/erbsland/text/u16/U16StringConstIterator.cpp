// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringConstIterator.hpp"

#include "U16StringView.hpp"

namespace erbsland::text {

struct U16StringConstIterator::Private {
    U16StringView view;       ///< The view accessed by this iterator
    unit::U16DataIndex index; ///< The current byte index within the storage
};

U16StringConstIterator::U16StringConstIterator() : _p{nullptr} {
}

U16StringConstIterator::U16StringConstIterator(const U16StringView &view, unit::U16DataIndex index) :
    _p{std::make_unique<Private>(view, index)} {
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
    if (_p->view.storageId() != other._p->view.storageId()) {
        return false;
    }
    return _p->index == other._p->index;
}

auto U16StringConstIterator::operator!=(const U16StringConstIterator &other) const noexcept -> bool {
    return !operator==(other);
}

auto U16StringConstIterator::isValid() const noexcept -> bool {
    return _p != nullptr && !_p->view.isEmpty() && !_p->index.isNoIndex();
}

auto U16StringConstIterator::operator*() const -> Char {
    if (!isValid()) {
        return Char::null();
    }
    return _p->view.charAt(_p->index);
}

auto U16StringConstIterator::operator++() -> U16StringConstIterator & {
    if (!isValid()) {
        return *this;
    }
    _p->view.advance(_p->index);
    return *this;
}

auto U16StringConstIterator::operator++(int) -> U16StringConstIterator {
    if (!isValid()) {
        return *this;
    }
    const auto result = *this;
    _p->view.advance(_p->index);
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
