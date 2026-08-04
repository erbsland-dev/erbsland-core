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

}
