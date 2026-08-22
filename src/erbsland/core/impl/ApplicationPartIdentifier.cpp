// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationPartIdentifier.hpp"

namespace erbsland::core::impl {

auto ApplicationPartIdentifier::cachedNumber(const uint64_t managerToken) const noexcept -> std::size_t {
    const auto lock = std::scoped_lock{_cacheMutex};
    return _cachedManagerToken == managerToken ? _cachedNumber : 0;
}

void ApplicationPartIdentifier::setCachedNumber(const uint64_t managerToken, const std::size_t number) noexcept {
    const auto lock = std::scoped_lock{_cacheMutex};
    _cachedManagerToken = managerToken;
    _cachedNumber = number;
}

}
