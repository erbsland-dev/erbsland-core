// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU8StringBuffer_fwd.hpp"

#include "../u8/impl/U8StringData.hpp"
#include "../u8/U8String.hpp"
#include "../u8/U8StringEditor.hpp"

#include "../../mem/ByteSpan.hpp"
#include "../../unit/ByteCount.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <span>
#include <utility>

namespace erbsland::text::impl {

/// Owns uncommitted UTF-8 string storage for low-level native APIs.
/// @warning Do not use this class in user code!
/// @tested{UnsafeU8StringEditorAccessTest}
class UnsafeU8StringBuffer {
public:
    /// Create a buffer with the given full data size, including the null byte.
    explicit UnsafeU8StringBuffer(const std::size_t dataSize, const bool sensitive = false) :
        _data{createDataForDataSize(dataSize, sensitive)} {}
    /// Create a buffer with the given usable capacity, excluding the null byte.
    explicit UnsafeU8StringBuffer(const unit::ByteCount capacity, const bool sensitive = false) :
        UnsafeU8StringBuffer{dataSizeForCapacity(capacity), sensitive} {}
    /// Create a buffer by copying raw standard bytes without validation or BOM handling.
    /// @param bytes Exact payload, including any null bytes; no terminator is required.
    /// @param sensitive Whether to protect the copied storage as sensitive data.
    explicit UnsafeU8StringBuffer(const std::span<const std::byte> bytes, const bool sensitive = false) :
        UnsafeU8StringBuffer{unit::ByteCount::fromSizeT(bytes.size()), sensitive} {
        if (!bytes.empty()) {
            std::memcpy(data(), bytes.data(), bytes.size());
        }
    }
    /// Create a buffer by copying raw Core bytes without validation or BOM handling.
    /// @param bytes Exact payload, including any null bytes; no terminator is required.
    /// @param sensitive Whether to protect the copied storage as sensitive data.
    explicit UnsafeU8StringBuffer(const mem::ConstByteSpan bytes, const bool sensitive = false) :
        UnsafeU8StringBuffer{std::as_bytes(bytes), sensitive} {}
    /// Create a buffer by copying raw character bytes without validation or BOM handling.
    /// @param bytes Exact payload, including any null bytes; no terminator is required.
    /// @param sensitive Whether to protect the copied storage as sensitive data.
    explicit UnsafeU8StringBuffer(const std::span<const char> bytes, const bool sensitive = false) :
        UnsafeU8StringBuffer{std::as_bytes(bytes), sensitive} {}

    // defaults
    ~UnsafeU8StringBuffer() = default;
    UnsafeU8StringBuffer(const UnsafeU8StringBuffer &) = delete;
    UnsafeU8StringBuffer(UnsafeU8StringBuffer &&) noexcept = default;
    auto operator=(const UnsafeU8StringBuffer &) = delete;
    auto operator=(UnsafeU8StringBuffer &&) noexcept -> UnsafeU8StringBuffer & = default;

public:
    /// Test if the uncommitted allocation is marked as sensitive.
    [[nodiscard]] auto isSensitive() const noexcept -> bool {
        return !_data.isNull() && _data.constGet()->isSensitive();
    }
    /// Mark the uncommitted allocation as sensitive.
    void markAsSensitive() noexcept {
        if (!_data.isNull()) {
            _data.get()->setSensitive();
        }
    }
    /// Access the writable buffer data.
    [[nodiscard]] auto data() noexcept -> mem::UnsafeCharPtr {
        if (_data.isNull()) {
            return nullptr;
        }
        return _data.get()->data();
    }
    /// Access the buffer data.
    [[nodiscard]] auto data() const noexcept -> mem::UnsafeConstCharPtr {
        if (_data.isNull()) {
            return nullptr;
        }
        return _data.constGet()->data();
    }
    /// Access the full buffer size, including the null byte.
    [[nodiscard]] auto dataSize() const noexcept -> std::size_t {
        if (_data.isNull()) {
            return 0;
        }
        return _data.constGet()->capacity();
    }
    /// Access the usable buffer capacity, excluding the null byte.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteCount {
        const auto size = dataSize();
        if (size == 0U) {
            return unit::ByteCount::zero();
        }
        return unit::ByteCount::fromSizeT(size - 1U);
    }
    /// Create a UTF-8 string from the buffer and release the buffer.
    [[nodiscard]] auto take(unit::ByteCount length = unit::ByteCount::infinite()) -> U8StringEditor {
        return U8StringEditor{takeStorage(length)};
    }
    /// Create a read-only UTF-8 string from the buffer and release the buffer.
    [[nodiscard]] auto takeString(unit::ByteCount length = unit::ByteCount::infinite()) -> U8String {
        return U8String{takeStorage(length)};
    }

public:
    /// Test if a capacity would exceed the maximum capacity.
    [[nodiscard]] static auto wouldExceedCapacity(const std::size_t capacityValue) noexcept -> bool {
        return !U8StringData::canAllocateWithCapacity(capacityValue);
    }

private:
    /// Create exact buffer storage for a full data size, including the null byte.
    [[nodiscard]] static auto createDataForDataSize(const std::size_t dataSize, const bool sensitive)
        -> U8StringDataPtr {
        if (dataSize == 0U) {
            return {};
        }
        if (!U8StringData::canAllocateWithCapacity(dataSize)) {
            std::terminate();
        }
        const auto size = static_cast<U8StringData::SizeType>(dataSize);
        const auto flags = sensitive ? U8StringData::cSensitiveFlag : std::uint8_t{};
        auto *data = U8StringData::create(size, size, flags);
        data->data()[dataSize - 1U] = '\0';
        return U8StringDataPtr{data};
    }
    /// Convert usable capacity to full buffer size.
    [[nodiscard]] static auto dataSizeForCapacity(unit::ByteCount capacity) -> std::size_t {
        if (capacity.isInfinite()) {
            std::terminate();
        }
        const auto size = capacity.toSizeTOrThrow();
        if (size == std::numeric_limits<std::size_t>::max()) {
            std::terminate();
        }
        return size + 1U;
    }
    /// Validate the final string length for take().
    [[nodiscard]] auto checkedFinalLength(unit::ByteCount length) const -> unit::ByteCount {
        if (length.isInfinite()) {
            return capacity();
        }
        if (length > capacity()) {
            std::terminate();
        }
        return length;
    }
    /// Finalize and detach shared string storage.
    [[nodiscard]] auto takeStorage(unit::ByteCount length) -> U8StringSharedStorage {
        const auto finalLength = checkedFinalLength(length);
        if (finalLength.isZero() && !isSensitive()) {
            _data.reset();
            return {};
        }
        auto data = std::move(_data);
        const auto size = finalLength.toSizeT();
        data.get()->setSize(static_cast<U8StringData::SizeType>(size + 1U));
        data.get()->data()[size] = '\0';
        return U8StringSharedStorage{std::move(data), unit::ByteRange::fromSizeT(size)};
    }

private:
    U8StringDataPtr _data; ///< The uncommitted string data.
};

}
