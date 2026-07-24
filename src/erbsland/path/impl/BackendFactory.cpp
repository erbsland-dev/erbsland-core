// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BackendFactory.hpp"

#include <cassert>
#include <mutex>

namespace erbsland::path::impl {

/// Shared backend instance for this process.
PathBackendPtr gPathBackend = {};

auto pathBackend() noexcept -> PathBackend & {
    static std::once_flag flag;
    std::call_once(flag, []() -> void {
        if (gPathBackend == nullptr) {
            gPathBackend = createPathBackend();
            if (gPathBackend == nullptr) {
                std::terminate();
            }
        }
    });
    return *gPathBackend;
}

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
void setPathBackend(PathBackendPtr &&backend) noexcept {
    if (backend == nullptr) {
        gPathBackend = createPathBackend();
    } else {
        gPathBackend = std::move(backend);
    }
}
#endif

}
