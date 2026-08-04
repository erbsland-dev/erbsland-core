// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"
#include "../u16/impl/U16StringStorage.hpp"
#include "../u32/impl/U32StringStorage.hpp"
#include "../u8/impl/U8StringStorage.hpp"

#include <cstdint>

namespace erbsland::text::impl {

/// Identify the kind of storage backing a string.
enum class StringStorageKind : uint8_t {
    Empty,   ///< The string has no backing storage.
    Shared,  ///< The string uses shared, mutable backing storage.
    Literal, ///< The string refers to literal backing storage.
};

/// Get the storage kind of shared UTF-8 string storage.
[[nodiscard]] auto storageKind(const U8StringSharedStorage &storage) noexcept -> StringStorageKind;
/// Get the storage kind of shared UTF-16 string storage.
[[nodiscard]] auto storageKind(const U16StringSharedStorage &storage) noexcept -> StringStorageKind;
/// Get the storage kind of shared UTF-32 string storage.
[[nodiscard]] auto storageKind(const U32StringSharedStorage &storage) noexcept -> StringStorageKind;
/// Get the storage kind of UTF-8 string storage.
[[nodiscard]] auto storageKind(const U8StringStorage &storage) noexcept -> StringStorageKind;
/// Get the storage kind of UTF-16 string storage.
[[nodiscard]] auto storageKind(const U16StringStorage &storage) noexcept -> StringStorageKind;
/// Get the storage kind of UTF-32 string storage.
[[nodiscard]] auto storageKind(const U32StringStorage &storage) noexcept -> StringStorageKind;
/// Convert a string storage kind into its text name.
[[nodiscard]] auto toString(StringStorageKind kind) noexcept -> String;

}
