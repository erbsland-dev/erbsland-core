// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProtectedDataAccess.hpp"

#include "ApplicationProtectedDataAccess.hpp"

namespace erbsland::cryptology::impl {

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
std::atomic<ProtectedDataAccess *> ProtectedDataAccess::_testAccess{nullptr};
#endif

auto ProtectedDataAccess::access() -> ProtectedDataAccess & {
#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
    if (auto *currentTestAccess = _testAccess.load(std::memory_order_acquire); currentTestAccess != nullptr) {
        return *currentTestAccess;
    }
#endif
    static auto applicationAccess = ApplicationProtectedDataAccess{};
    return applicationAccess;
}

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
void ProtectedDataAccess::setTestAccess(ProtectedDataAccess *access) noexcept {
    _testAccess.store(access, std::memory_order_release);
}
#endif

}
