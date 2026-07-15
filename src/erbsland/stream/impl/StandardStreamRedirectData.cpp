// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreamRedirectData.hpp"

#include "StandardStreamRegistry.hpp"

#include <utility>

namespace erbsland::stream::impl {

StandardStreamRedirectData::StandardStreamRedirectData(
    const StandardStreamSlot slot,
    TextInputStreamPtr previousInput,
    TextOutputStreamPtr previousOutput,
    TextOutputStreamPtr previousError) :
    _slot{slot},
    _previousInput{std::move(previousInput)},
    _previousOutput{std::move(previousOutput)},
    _previousError{std::move(previousError)} {
}

auto StandardStreamRedirectData::isActive() const noexcept -> bool {
    auto lock = std::scoped_lock{_mutex};
    return _active;
}

void StandardStreamRedirectData::reset() noexcept {
    auto lock = std::scoped_lock{_mutex};
    if (!_active) {
        return;
    }
    standardStreamRegistry().restore(
        _slot, std::move(_previousInput), std::move(_previousOutput), std::move(_previousError));
    _active = false;
}

}
