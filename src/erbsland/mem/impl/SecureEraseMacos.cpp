// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SecureEraseBackend.hpp"

#include <string.h>

namespace erbsland::mem::impl {

void secureEraseBackend(const std::span<std::byte> memory) noexcept {
    static_cast<void>(memset_s(memory.data(), memory.size(), 0, memory.size()));
}

}
