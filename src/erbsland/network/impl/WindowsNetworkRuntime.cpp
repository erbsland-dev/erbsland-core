// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsNetworkRuntime.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../system/PlatformError.hpp"
#include "../../system/WindowsErrorContext.hpp"
#include "../../text/Literals.hpp"

#include <winsock2.h>

#include <mutex>

namespace erbsland::network::impl {

using namespace text::literals;

WindowsNetworkRuntime::WindowsNetworkRuntime() {
    auto data = WSADATA{};
    const auto errorCode = ::WSAStartup(MAKEWORD(2, 2), &data);
    if (errorCode != 0) {
        throw system::PlatformError{
            "Winsock initialization failed."_el, system::WindowsErrorContext::fromErrorCode(errorCode)};
    }
    if (data.wVersion != MAKEWORD(2, 2)) {
        // Preserve the version error; there is no initialized runtime left to recover if cleanup fails.
        ::WSACleanup();
        throw system::PlatformError{
            "Winsock 2.2 is not available."_el, system::WindowsErrorContext::fromErrorCode(WSAVERNOTSUPPORTED)};
    }
}

WindowsNetworkRuntime::~WindowsNetworkRuntime() {
    // Process teardown cannot recover from or report a Winsock cleanup failure.
    ::WSACleanup();
}

auto WindowsNetworkRuntime::shared() -> std::shared_ptr<WindowsNetworkRuntime> {
    static auto weakRuntime = std::weak_ptr<WindowsNetworkRuntime>{};
    static auto mutex = std::mutex{};
    const auto lock = std::scoped_lock{mutex};
    if (const auto existing = weakRuntime.lock(); existing != nullptr) {
        return existing;
    }
    auto result = std::shared_ptr<WindowsNetworkRuntime>{new WindowsNetworkRuntime{}};
    weakRuntime = result;
    return result;
}

}
