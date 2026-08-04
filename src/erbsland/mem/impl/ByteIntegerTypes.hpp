// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>
#include <cstdint>

namespace erbsland::mem::impl {

/// A fixed-width native integer supported by the byte reader and writer.
template <typename T>
concept NativeByteInteger =
    std::same_as<T, int8_t> || std::same_as<T, uint8_t> || std::same_as<T, int16_t> || std::same_as<T, uint16_t> ||
    std::same_as<T, int32_t> || std::same_as<T, uint32_t> || std::same_as<T, int64_t> || std::same_as<T, uint64_t>;

}
