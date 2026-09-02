// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SystemInfoBackend.hpp"

#include "SystemInfoBackendStorage.hpp"

#include <mutex>
#include <utility>

namespace erbsland::system::impl {

auto systemInfoBackend() noexcept -> SystemInfoBackend & {
    auto &storage = SystemInfoBackendStorage::instance();
    const auto lock = std::scoped_lock{storage.mutex()};
    auto &backend = storage.backend();
    if (!backend) {
        try {
            backend = createSystemInfoBackend();
        } catch (...) {
            std::terminate();
        }
    }
    return *backend;
}

void setSystemInfoBackend(SystemInfoBackendPtr backend) noexcept {
    auto &storage = SystemInfoBackendStorage::instance();
    const auto lock = std::scoped_lock{storage.mutex()};
    try {
        storage.backend() = backend ? std::move(backend) : createSystemInfoBackend();
    } catch (...) {
        std::terminate();
    }
}

}
