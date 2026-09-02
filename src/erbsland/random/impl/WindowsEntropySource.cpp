// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsEntropySource.hpp"

#include "../RandomError.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../system/impl/WindowsErrorContext.hpp"
#include "../../system/PlatformError.hpp"
#include "../../text/Literals.hpp"

#include <bcrypt.h>

#include <limits>

namespace erbsland::random::impl {

using namespace text::literals;

void WindowsEntropySource::fillBytes(const std::span<std::byte> destination) {
    if (destination.empty()) {
        return;
    }
    if (destination.size() > std::numeric_limits<ULONG>::max()) {
        throw system::PlatformError{"Requested entropy block is too large"_el};
    }
    const auto status = ::BCryptGenRandom(
        nullptr,
        reinterpret_cast<PUCHAR>(destination.data()),
        static_cast<ULONG>(destination.size()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (status < 0) {
        throw system::PlatformError{
            "System entropy source failed"_el, system::impl::WindowsErrorContext::fromLastError()};
    }
}

}
