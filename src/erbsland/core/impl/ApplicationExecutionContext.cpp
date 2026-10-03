// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationExecutionContext.hpp"

#include <exception>
#include <utility>

namespace erbsland::core::impl {

thread_local ApplicationExecutionContext *ApplicationExecutionContext::_current{};

ApplicationExecutionContext::ApplicationExecutionContext(ApplicationDataPtr data) :
    _data{std::move(data)}, _previous{_current} {
    if (_data == nullptr) {
        std::terminate();
    }
    _current = this;
}

ApplicationExecutionContext::~ApplicationExecutionContext() {
    if (_current != this) {
        std::terminate();
    }
    _current = _previous;
}

auto ApplicationExecutionContext::current() noexcept -> ApplicationDataPtr {
    if (_current != nullptr) {
        return _current->_data;
    }
    return {};
}

}
