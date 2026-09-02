// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SystemInfoBackend_fwd.hpp"

#include <mutex>

namespace erbsland::system::impl {

/// Process-wide storage for the replaceable system-information backend.
/// @tested{ProcessInfoTest SystemInfoTest}
class SystemInfoBackendStorage final {
public:
    /// Get the process-wide storage instance.
    [[nodiscard]] static auto instance() noexcept -> SystemInfoBackendStorage &;

    /// Destroy the storage after the backend is complete.
    ~SystemInfoBackendStorage();

    // defaults/deletions
    SystemInfoBackendStorage(const SystemInfoBackendStorage &) = delete;
    SystemInfoBackendStorage(SystemInfoBackendStorage &&) = delete;
    auto operator=(const SystemInfoBackendStorage &) -> SystemInfoBackendStorage & = delete;
    auto operator=(SystemInfoBackendStorage &&) -> SystemInfoBackendStorage & = delete;

public:
    /// Get the stored backend.
    [[nodiscard]] auto backend() noexcept -> SystemInfoBackendPtr & { return _backend; }
    /// Get the mutex protecting backend replacement and initialization.
    [[nodiscard]] auto mutex() noexcept -> std::mutex & { return _mutex; }

private:
    // defaults
    SystemInfoBackendStorage() = default;

private:
    SystemInfoBackendPtr _backend; ///< Replaceable backend.
    std::mutex _mutex;             ///< Protects backend replacement and initialization.
};

}
