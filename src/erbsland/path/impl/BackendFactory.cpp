// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BackendFactory.hpp"

#include "PathBackend.hpp"

#include <cassert>
#include <mutex>

namespace erbsland::path::impl {

/// Get the lazily initialized shared backend storage.
auto pathBackendStorage() noexcept -> PathBackendPtr & {
    static auto backend = PathBackendPtr{};
    return backend;
}

auto pathBackend() noexcept -> PathBackend & {
    static std::once_flag flag;
    std::call_once(flag, []() -> void {
        auto &backend = pathBackendStorage();
        if (backend == nullptr) {
            backend = createPathBackend();
            if (backend == nullptr) {
                std::terminate();
            }
        }
    });
    return *pathBackendStorage();
}

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
void setPathBackend(PathBackendPtr &&backend) noexcept {
    auto &storage = pathBackendStorage();
    if (backend == nullptr) {
        storage = createPathBackend();
    } else {
        storage = std::move(backend);
    }
}
#endif

}
