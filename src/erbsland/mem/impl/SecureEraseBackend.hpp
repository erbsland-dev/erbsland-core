// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <span>

namespace erbsland::mem::impl {

/// Securely erase bytes through the selected native platform primitive.
/// @notest{Exercised through the shared secure-erasure tests on every supported platform.}
void secureEraseBackend(std::span<std::byte> memory) noexcept;

}
