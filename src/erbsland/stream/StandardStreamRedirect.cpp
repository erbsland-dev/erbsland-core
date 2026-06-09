// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreamRedirect.hpp"

#include "impl/StandardStreamRedirectData.hpp"

#include <utility>

namespace erbsland::stream {

StandardStreamRedirect::StandardStreamRedirect(std::shared_ptr<impl::StandardStreamRedirectData> data) noexcept :
    _data{std::move(data)} {
}

StandardStreamRedirect::~StandardStreamRedirect() {
    reset();
}

StandardStreamRedirect::StandardStreamRedirect(StandardStreamRedirect &&other) noexcept :
    _data{std::move(other._data)} {
}

auto StandardStreamRedirect::operator=(StandardStreamRedirect &&other) noexcept -> StandardStreamRedirect & {
    if (this != &other) {
        reset();
        _data = std::move(other._data);
    }
    return *this;
}

auto StandardStreamRedirect::isActive() const noexcept -> bool {
    return _data != nullptr && _data->isActive();
}

void StandardStreamRedirect::reset() noexcept {
    if (_data != nullptr) {
        _data->reset();
        _data.reset();
    }
}

}
