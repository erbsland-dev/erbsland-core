// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SecureEraseBackend.hpp"

#include "../../core/impl/WindowsApi.hpp"

namespace erbsland::mem::impl {

void secureEraseBackend(const std::span<std::byte> memory) noexcept {
    SecureZeroMemory(memory.data(), memory.size());
}

}
