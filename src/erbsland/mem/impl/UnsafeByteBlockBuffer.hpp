// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteBlockData.hpp"
#include "UnsafeByteBlockBuffer_fwd.hpp"

#include "../ByteBlockEditor.hpp"

#include "../../unit/ByteLength.hpp"

#include <cstddef>
#include <exception>
#include <span>
#include <utility>

namespace erbsland::mem::impl {

/// Owns uncommitted byte block storage for low-level native APIs.
/// @warning Do not use this class in user code!
/// @tested{ByteStreamTest}
class UnsafeByteBlockBuffer {
public:
    /// Create a buffer with the given usable capacity.
    explicit UnsafeByteBlockBuffer(unit::ByteLength capacity) : _block{capacity} {
        if (!_block._data.isNull()) {
            _block._data.get()->setSize(0U);
        }
    }

    // defaults
    ~UnsafeByteBlockBuffer() = default;
    UnsafeByteBlockBuffer(const UnsafeByteBlockBuffer &) = delete;
    UnsafeByteBlockBuffer(UnsafeByteBlockBuffer &&) noexcept = default;
    auto operator=(const UnsafeByteBlockBuffer &) -> UnsafeByteBlockBuffer & = delete;
    auto operator=(UnsafeByteBlockBuffer &&) noexcept -> UnsafeByteBlockBuffer & = default;

public:
    /// Access the writable buffer span.
    [[nodiscard]] auto data() noexcept -> std::span<Byte> {
        if (_block._data.isNull()) {
            return {};
        }
        return std::span<Byte>{_block._data.get()->data(), _block._data.get()->capacity()};
    }
    /// Access the usable buffer capacity.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength { return _block.capacity(); }
    /// Create a byte block from the buffer and release the buffer.
    [[nodiscard]] auto take(unit::ByteLength length = unit::ByteLength::infinite()) -> ByteBlockEditor {
        const auto finalLength = checkedFinalLength(length);
        if (finalLength.isZero()) {
            _block.reset();
            return {};
        }
        auto result = std::move(_block);
        result._data.get()->setSize(static_cast<ByteBlockData::SizeType>(finalLength.toSizeT()));
        return result;
    }

private:
    /// Validate the final byte length for take().
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
    ByteBlockEditor _block; ///< The uncommitted byte block storage.
};

}
