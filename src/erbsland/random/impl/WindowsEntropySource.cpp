// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsEntropySource.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../err/RandomError.hpp"

#include <bcrypt.h>

#include <limits>

namespace erbsland::random::impl {

void WindowsEntropySource::fillBytes(const std::span<std::byte> destination) {
    if (destination.empty()) {
        return;
    }
    if (destination.size() > std::numeric_limits<ULONG>::max()) {
        throw err::RandomError{"Requested entropy block is too large"};
    }
    const auto status = ::BCryptGenRandom(
        nullptr,
        reinterpret_cast<PUCHAR>(destination.data()),
        static_cast<ULONG>(destination.size()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (status < 0) {
        throw err::RandomError{"System entropy source failed"};
    }
}

}
