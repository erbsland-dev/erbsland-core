// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteSpan.hpp"

#include <span>
#include <type_traits>

namespace erbsland::mem {

/// Secure erase the memory from a byte span.
/// The platform implementation prevents the erase from being optimized away.
/// @param span The writable bytes to erase.
/// @tested{SecureEraseTest}
void secureErase(ByteSpan span) noexcept;

/// Securely erase writable storage represented by native trivially-copyable objects.
/// This overload is intended for mathematical word state whose representation is not byte-addressed at the API level.
/// @tparam T The writable trivially-copyable element type.
/// @tparam Extent The span extent.
/// @param span The native object storage to erase.
/// @tested{SecureEraseTest}
template <typename T, std::size_t Extent>
    requires(!std::is_const_v<T> && std::is_trivially_copyable_v<T>)
void secureErase(const std::span<T, Extent> span) noexcept {
    secureErase(toByteSpan(std::as_writable_bytes(span)));
}

}
