// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <span>

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
#include <atomic>
#endif

namespace erbsland::mem::impl {

/// Erase memory using an optimizer-resistant platform primitive.
/// @param memory The memory to erase.
/// @notest{This internal platform abstraction is tested through secure shared storage.}
void secureErase(std::span<std::byte> memory) noexcept;

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)

/// An observer invoked after a secure-erasure operation.
using SecureEraseObserver = void (*)(std::span<const std::byte>) noexcept;

/// Install an observer for secure-erasure tests.
/// Only one observer is active process-wide. Passing `nullptr` removes it.
/// @param observer The observer to install.
/// @notest{This is test instrumentation.}
void setSecureEraseObserver(SecureEraseObserver observer) noexcept;

#endif

}
