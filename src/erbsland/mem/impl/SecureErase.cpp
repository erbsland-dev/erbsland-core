// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SecureErase.hpp"

#include "SecureEraseBackend.hpp"

#include <atomic>

namespace erbsland::mem::impl {

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)

namespace {
std::atomic<SecureEraseObserver> gSecureEraseObserver{nullptr};
}

void setSecureEraseObserver(const SecureEraseObserver observer) noexcept {
    gSecureEraseObserver.store(observer, std::memory_order_release);
}

#endif

void secureErase(const std::span<std::byte> memory) noexcept {
    if (memory.empty()) {
        return;
    }
    secureEraseBackend(memory);
#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
    if (const auto observer = gSecureEraseObserver.load(std::memory_order_acquire); observer != nullptr) {
        observer(memory);
    }
#endif
}

}
