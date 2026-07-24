// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SensitiveInput.hpp"

#include "impl/StandardStreamRegistry.hpp"

#include <exception>
#include <utility>

namespace erbsland::stream::io {

SensitiveInputToken::SensitiveInputToken(SensitiveInputToken &&other) noexcept :
    _id{std::exchange(other._id, 0U)}, _sourceLocation{other._sourceLocation} {
}

auto SensitiveInputToken::operator=(SensitiveInputToken &&other) noexcept -> SensitiveInputToken & {
    if (this != &other) {
        if (_id != 0U) {
            try {
                impl::standardStreamRegistry().stopSensitiveInput(_id);
            } catch (...) {
                std::terminate();
            }
        }
        _id = std::exchange(other._id, 0U);
        _sourceLocation = other._sourceLocation;
    }
    return *this;
}

auto startSensitiveInput(const std::source_location location) -> SensitiveInputToken {
    return SensitiveInputToken{impl::standardStreamRegistry().startSensitiveInput(location), location};
}

void stopSensitiveInput(SensitiveInputToken token) {
    impl::standardStreamRegistry().stopSensitiveInput(token._id);
    token._id = 0U;
}

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
