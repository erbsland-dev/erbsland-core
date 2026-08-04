// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProtectedDataProvider.hpp"

#include "InternalProtectedDataProvider.hpp"

#if defined(__APPLE__)
#include "MacosProtectedDataProvider.hpp"
#elif defined(_WIN32)
#include "WindowsProtectedDataProvider.hpp"
#endif

namespace erbsland::cryptology::impl {

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
std::atomic<ProtectedDataProvider::TestPlatformFactory> ProtectedDataProvider::_testPlatformFactory{nullptr};
#endif

auto ProtectedDataProvider::createInternal() -> ProtectedDataProviderPtr {
    return std::make_unique<InternalProtectedDataProvider>();
}

auto ProtectedDataProvider::createPlatform() -> ProtectedDataProviderPtr {
#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
    if (const auto factory = _testPlatformFactory.load(std::memory_order_acquire); factory != nullptr) {
        return factory();
    }
#endif
#if defined(__APPLE__)
    return std::make_unique<MacosProtectedDataProvider>();
#elif defined(_WIN32)
    return std::make_unique<WindowsProtectedDataProvider>();
#else
    return {};
#endif
}

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
void ProtectedDataProvider::setTestPlatformFactory(const TestPlatformFactory factory) noexcept {
    _testPlatformFactory.store(factory, std::memory_order_release);
}
#endif

}
