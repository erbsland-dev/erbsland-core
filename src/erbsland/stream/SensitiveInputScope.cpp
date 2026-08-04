// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SensitiveInputScope.hpp"

#include <exception>
#include <utility>

namespace erbsland::stream::io {

SensitiveInputScope::SensitiveInputScope(const std::source_location location) : _token{startSensitiveInput(location)} {
}

SensitiveInputScope::~SensitiveInputScope() {
    reset();
}

SensitiveInputScope::SensitiveInputScope(SensitiveInputScope &&other) noexcept : _token{std::move(other._token)} {
    other._token.reset();
}

auto SensitiveInputScope::operator=(SensitiveInputScope &&other) noexcept -> SensitiveInputScope & {
    if (this != &other) {
        reset();
        _token = std::move(other._token);
        other._token.reset();
    }
    return *this;
}

void SensitiveInputScope::reset() noexcept {
    if (!_token.has_value()) {
        return;
    }
    try {
        stopSensitiveInput(std::move(*_token));
        _token.reset();
    } catch (...) {
        std::terminate();
    }
}

}
