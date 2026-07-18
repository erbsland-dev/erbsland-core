// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeU8StringBuffer_fwd.hpp"

#include "../u8/impl/U8StringData.hpp"
#include "../u8/U8StringEditor.hpp"

#include "../../unit/ByteLength.hpp"

#include <cstddef>
#include <exception>
#include <limits>
#include <utility>

namespace erbsland::text::impl {

/// Owns uncommitted UTF-8 string storage for low-level native APIs.
/// @warning Do not use this class in user code!
/// @tested{UnsafeU8StringEditorAccessTest}
class UnsafeU8StringBuffer {
public:
    /// Create a buffer with the given full data size, including the null byte.
    explicit UnsafeU8StringBuffer(std::size_t dataSize) : _data{createDataForDataSize(dataSize)} {}
    /// Create a buffer with the given usable capacity, excluding the null byte.
    explicit UnsafeU8StringBuffer(unit::ByteLength capacity) : UnsafeU8StringBuffer{dataSizeForCapacity(capacity)} {}

    // defaults
    ~UnsafeU8StringBuffer() = default;
    UnsafeU8StringBuffer(const UnsafeU8StringBuffer &) = delete;
    UnsafeU8StringBuffer(UnsafeU8StringBuffer &&) noexcept = default;
    auto operator=(const UnsafeU8StringBuffer &) = delete;
    auto operator=(UnsafeU8StringBuffer &&) noexcept -> UnsafeU8StringBuffer & = default;

public:
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
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength {
        const auto size = dataSize();
        if (size == 0U) {
            return unit::ByteLength::zero();
        }
        return unit::ByteLength::fromSizeT(size - 1U);
    }
    /// Create a UTF-8 string from the buffer and release the buffer.
    [[nodiscard]] auto take(unit::ByteLength length = unit::ByteLength::infinite()) -> U8StringEditor {
        const auto finalLength = checkedFinalLength(length);
        if (finalLength.isZero()) {
            _data.reset();
            return {};
        }
        auto data = std::move(_data);
        const auto size = finalLength.toSizeT();
        data.get()->setSize(static_cast<U8StringData::SizeType>(size + 1U));
        data.get()->data()[size] = '\0';
        return U8StringEditor{U8StringSharedStorage{std::move(data), unit::ByteRange::fromSizeT(size)}};
    }

private:
    /// Create exact buffer storage for a full data size, including the null byte.
    [[nodiscard]] static auto createDataForDataSize(std::size_t dataSize) -> U8StringDataPtr {
        if (dataSize == 0U) {
            return {};
        }
        if (!U8StringData::canAllocateWithCapacity(dataSize)) {
            std::terminate();
        }
        const auto size = static_cast<U8StringData::SizeType>(dataSize);
        auto *data = U8StringData::create(size, size);
        data->data()[dataSize - 1U] = '\0';
        return U8StringDataPtr{data};
    }
    /// Convert usable capacity to full buffer size.
    [[nodiscard]] static auto dataSizeForCapacity(unit::ByteLength capacity) -> std::size_t {
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
    [[nodiscard]] auto checkedFinalLength(unit::ByteLength length) const -> unit::ByteLength {
        if (length.isInfinite()) {
            return capacity();
        }
        if (length > capacity()) {
            std::terminate();
        }
        return length;
    }

private:
    U8StringDataPtr _data; ///< The uncommitted string data.
};

}
