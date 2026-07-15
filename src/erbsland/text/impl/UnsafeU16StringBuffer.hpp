// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../u16/impl/U16StringData.hpp"
#include "../u16/U16String.hpp"

#include "../../unit/U16DataLength.hpp"

#include <cstddef>
#include <exception>
#include <limits>
#include <utility>

namespace erbsland::text::impl {

/// Owns uncommitted UTF-16 string storage for low-level native APIs.
/// @warning Do not use this class in user code!
/// @tested{UnsafeU16StringAccessTest}
class UnsafeU16StringBuffer {
public:
    /// Create a buffer with the given full data size, including the null code unit.
    explicit UnsafeU16StringBuffer(std::size_t dataSize);
    /// Create a buffer with the given usable capacity, excluding the null code unit.
    explicit UnsafeU16StringBuffer(unit::U16DataLength capacity);

    // defaults
    ~UnsafeU16StringBuffer() = default;
    UnsafeU16StringBuffer(const UnsafeU16StringBuffer &) = delete;
    UnsafeU16StringBuffer(UnsafeU16StringBuffer &&) noexcept = default;
    auto operator=(const UnsafeU16StringBuffer &) = delete;
    auto operator=(UnsafeU16StringBuffer &&) noexcept -> UnsafeU16StringBuffer & = default;

public:
    /// Access the writable buffer data.
    [[nodiscard]] auto data() noexcept -> char16_t *;
    /// Access the buffer data.
    [[nodiscard]] auto data() const noexcept -> const char16_t *;
#ifdef ERBSLAND_WCHAR_16BIT
    /// Access the writable buffer data as wide char.
    [[nodiscard]] auto dataAsWide() noexcept -> wchar_t *;
    /// Access the buffer data as wide char.
    [[nodiscard]] auto dataAsWide() const noexcept -> const wchar_t *;
#endif
    /// Access the full buffer size, including the null code unit.
    [[nodiscard]] auto dataSize() const noexcept -> std::size_t;
    /// Access the usable buffer capacity, excluding the null code unit.
    [[nodiscard]] auto capacity() const noexcept -> unit::U16DataLength;
    /// Create a UTF-16 string from the buffer and release the buffer.
    /// @param length The length of the string to create, excluding the null code unit.
    /// @return The created UTF-16 string.
    [[nodiscard]] auto take(unit::U16DataLength length = unit::U16DataLength::infinite()) -> U16String;
    /// @overload
    [[nodiscard]] auto take(std::size_t length) -> U16String;
    /// Create a UTF-8 string from the buffer and release the buffer.
    /// This is a convenience overload of the take() function that converts the buffer from UTF-16 to UTF-8.
    /// @param length The length of the string to create, excluding the null code unit.
    /// @return The created UTF-16 string.
    [[nodiscard]] auto takeAsUtf8(unit::U16DataLength length = unit::U16DataLength::infinite()) -> U8String;
    /// @overload
    [[nodiscard]] auto takeAsUtf8(std::size_t length) -> U8String;

private:
    /// Create exact buffer storage for a full data size, including the null code unit.
    [[nodiscard]] static auto createDataForDataSize(std::size_t dataSize) -> U16StringDataPtr;
    /// Convert usable capacity to full buffer size.
    [[nodiscard]] static auto dataSizeForCapacity(unit::U16DataLength capacity) -> std::size_t;
    /// Validate the final string length for take().
    [[nodiscard]] auto checkedFinalLength(unit::U16DataLength length) const -> unit::U16DataLength;

private:
    U16StringDataPtr _data; ///< The uncommitted string data.
};

}
