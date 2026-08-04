// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SecureErase.hpp"

#include "SecureEraseBackend.hpp"

#include <atomic>

namespace erbsland::mem::impl {

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)

/// Get the lazily initialized test observer storage.
auto secureEraseObserverStorage() noexcept -> std::atomic<SecureEraseObserver> & {
    static auto observer = std::atomic<SecureEraseObserver>{nullptr};
    return observer;
}

void setSecureEraseObserver(const SecureEraseObserver observer) noexcept {
    secureEraseObserverStorage().store(observer, std::memory_order_release);
}

#endif

void secureErase(const std::span<std::byte> memory) noexcept {
    if (memory.empty()) {
        return;
    }
    secureEraseBackend(memory);
#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
    if (const auto observer = secureEraseObserverStorage().load(std::memory_order_acquire); observer != nullptr) {
        observer(memory);
    }
#endif
}

}
