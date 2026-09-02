// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SystemInfoBackendStorage.hpp"

#include "SystemInfoBackend.hpp"

namespace erbsland::system::impl {

auto SystemInfoBackendStorage::instance() noexcept -> SystemInfoBackendStorage & {
    static auto value = SystemInfoBackendStorage{};
    return value;
}

SystemInfoBackendStorage::~SystemInfoBackendStorage() = default;

}
